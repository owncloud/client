#!/usr/bin/env bash
# Regenerates conan.lock for all four platforms in conan/profiles/. This is the
# only supported way to change conan.lock.
#
# Usage:
#   OC_RECIPES_DIR=<checkout> conan/update-lock.sh [PATTERN...]
#   OC_RECIPES_DIR=<checkout> conan/update-lock.sh --check [--base GIT-REF]
#
#   OC_RECIPES_DIR  A checkout of https://github.com/owncloud/conan-center-index
#                   at the commit in conan/recipes-commit, with a clean tree.
#                   It is used as a local-recipes-index remote, the only
#                   remote: the script runs in a throwaway CONAN_HOME with all
#                   other remotes removed, so it never contacts conancenter.
#   PATTERN         Packages to bump, e.g. 'openssl/*'. Their entries (and
#                   their build-requires entries) are dropped from the
#                   existing conan.lock before resolving, so they move to what
#                   the recipes offer now; everything else stays locked.
#   --check         Write nothing; exit 1 if conan.lock is not what this
#                   script would produce (the CI drift check). It applies the
#                   three rules described below and reports every violation.
#   --base GIT-REF  With --check: the commit whose conan.lock the change
#                   starts from, read with `git show GIT-REF:conan.lock`.
#                   Default: origin/master. CI passes the merge request's
#                   target branch. No conan.lock at GIT-REF counts as empty.
#
# How it works: once per profile it runs
#   conan lock create . -pr:a <profile> --lockfile=conan.lock --lockfile-clean --update
# starting from the existing conan.lock (if any), so only what changed moves
# and unused entries are dropped. -pr:a sets the host and the build profile,
# so the build-context tool_requires are those of the target platform, and all
# four platforms can be locked on any machine. The four results are merged
# with `conan lock merge`.
#
# --update: without it, Conan resolves a version range from the local cache
# whenever the cache has a match, and asks the remote only otherwise. The
# profile runs share one throwaway cache, so an unlocked range could resolve
# to an older version that an earlier run happened to export. With --update a
# range always resolves to the newest match in the pinned recipes, which keeps
# the result deterministic. Locked entries are unaffected: Conan resolves
# them from the lockfile before it looks at ranges.
#
# The --check rules. Entries are compared per kind (requires,
# build_requires, python_requires, config_requires) by their full
# name/version#revision, timestamp ignored. A fresh lock is resolved like
# conan.lock (per profile with --update, then merged) but without any
# existing lockfile.
#  1. Fixed point: regenerating from conan.lock as above gives conan.lock
#     again. This catches missing requirements and unused entries.
#  2. Provenance: an entry that is in conan.lock but not in the base lock must
#     be in the fresh lock. So an entry that a change adds is what the pinned
#     recipes resolve to today, not a version picked by hand. Entries
#     inherited from the base are allowed as they are, such as versions kept
#     after a recipe-commit bump.
#  3. No hand removals: an entry that is in both the base lock and the fresh
#     lock must be in conan.lock. This catches an entry deleted by hand when
#     the package then falls back to another version that is already locked,
#     which rules 1 and 2 cannot see.
#  A bump with PATTERN passes: `conan lock upgrade` resolves the bumped
#  packages to the newest version in range, as the fresh lock does, and the
#  version it replaces is not in the fresh lock. So does the first lock,
#  resolved fresh against an empty base.
#
# Bumping: verified against Conan 2.34.0 (`conan lock upgrade -h` and
# conan/cli/commands/lock.py). `conan lock upgrade` (marked experimental)
# takes --update-requires / --update-build-requires PATTERN: it removes the
# matching entries from the given lockfile and then resolves the graph like
# `conan lock create`, honouring --lockfile-clean. With PATTERNs it is used
# instead of `conan lock create`. (`conan lock update` is not the right tool:
# it replaces an entry with an explicitly given reference and resolves
# nothing.)
#
# Normalization: with a local-recipes-index remote, Conan stamps each recipe
# revision with the time it exported it into the cache, so a fresh CONAN_HOME
# yields new timestamps on every run. The timestamps are stripped
# ("name/version#revision"), which Conan accepts in a lockfile; the revision
# alone identifies the recipe. --check compares the parsed JSON with
# timestamps stripped and every list sorted.
#
# Needs: conan (the version pinned in conanfile.py), git, python3 (for --check).

set -euo pipefail

# Empty arrays are expanded as ${a[@]+"${a[@]}"}: bash 3.2 (macOS /bin/bash)
# treats "${a[@]}" of an empty array as unbound under set -u.

die() { echo "update-lock: error: $*" >&2; exit 2; }

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

profiles=(linux-x86_64 windows-x86_64 macos-armv8 macos-x86_64)
remote=oc-recipes

check=0
base_ref=
patterns=()
while [ $# -gt 0 ]; do
    case "$1" in
        --check) check=1 ;;
        --base) [ $# -ge 2 ] || die "--base needs a git ref"; base_ref="$2"; shift ;;
        --base=*) base_ref="${1#--base=}" ;;
        -h|--help) sed -n '2,/^$/s/^# \{0,1\}//p' "${BASH_SOURCE[0]}"; exit 0 ;;
        -*) die "unknown option: $1" ;;
        *) patterns+=("$1") ;;
    esac
    shift
done
if [ "$check" = 1 ] && [ "${#patterns[@]}" -gt 0 ]; then
    die "--check takes no package patterns"
fi
if [ "$check" = 0 ] && [ -n "$base_ref" ]; then
    die "--base is only valid with --check"
fi
[ -n "$base_ref" ] || base_ref=origin/master

command -v conan >/dev/null || die "conan not found on PATH"
command -v git >/dev/null || die "git not found on PATH"
if [ "$check" = 1 ]; then
    command -v python3 >/dev/null || die "python3 not found on PATH (needed for --check)"
fi

[ -n "${OC_RECIPES_DIR:-}" ] || die "OC_RECIPES_DIR is not set; point it at a checkout of
  https://github.com/owncloud/conan-center-index at commit $(cat conan/recipes-commit)"
[ -d "$OC_RECIPES_DIR/recipes" ] || die "OC_RECIPES_DIR=$OC_RECIPES_DIR has no recipes/ folder"
recipes_dir="$(cd "$OC_RECIPES_DIR" && pwd)"

pinned="$(tr -d '[:space:]' < conan/recipes-commit)"
actual="$(git -C "$recipes_dir" rev-parse HEAD 2>/dev/null)" \
    || die "OC_RECIPES_DIR=$recipes_dir is not a git checkout"
[ "$actual" = "$pinned" ] \
    || die "OC_RECIPES_DIR is at $actual, conan/recipes-commit pins $pinned"
[ -z "$(git -C "$recipes_dir" status --porcelain)" ] \
    || die "OC_RECIPES_DIR=$recipes_dir has local changes; recipe revisions would not match the pin"

for p in "${profiles[@]}"; do
    [ -f "conan/profiles/$p" ] || die "missing profile conan/profiles/$p"
done

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

if [ "$check" = 1 ]; then
    git rev-parse --verify --quiet "$base_ref^{commit}" >/dev/null \
        || die "--base $base_ref is not a commit in this repository; fetch it or pass another --base"
    if git cat-file -e "$base_ref:./conan.lock" 2>/dev/null; then
        git show "$base_ref:./conan.lock" > "$work/base.lock"
    else
        echo '{}' > "$work/base.lock"
    fi
fi
export CONAN_HOME="$work/conan-home"

conan remote remove '*' >/dev/null
conan remote add "$remote" "$recipes_dir" --type=local-recipes-index
[ "$(conan remote list | cut -d: -f1)" = "$remote" ] \
    || die "unexpected Conan remotes in the throwaway CONAN_HOME"

base=()
if [ -f conan.lock ]; then
    base=(--lockfile=conan.lock)
else
    [ "$check" = 0 ] || die "no conan.lock to check"
    # No lockfile yet: there is nothing to keep, so a full resolve is a bump of everything.
    patterns=()
fi

merge_args=()
for p in "${profiles[@]}"; do
    out="$work/$p.lock"
    echo "update-lock: resolving $p" >&2
    if [ "${#patterns[@]}" -gt 0 ]; then
        bump=()
        for pat in "${patterns[@]}"; do
            bump+=("--update-requires=$pat" "--update-build-requires=$pat")
        done
        conan lock upgrade . -pr:a "conan/profiles/$p" ${base[@]+"${base[@]}"} --lockfile-clean \
            --update "${bump[@]}" -r "$remote" --lockfile-out="$out" -verror
    else
        conan lock create . -pr:a "conan/profiles/$p" ${base[@]+"${base[@]}"} --lockfile-clean \
            --update -r "$remote" --lockfile-out="$out" -verror
    fi
    merge_args+=("--lockfile=$out")
done

conan lock merge "${merge_args[@]}" --lockfile-out="$work/merged.lock" -verror
# Drop the export timestamps: "ref#rev%1794407837.72" -> "ref#rev".
sed -E 's/%[0-9]+(\.[0-9]+)?"/"/' "$work/merged.lock" > "$work/conan.lock"

if [ "$check" = 1 ]; then
    # The fresh lock for rules 2 and 3. An empty --lockfile= keeps Conan from
    # using ./conan.lock on its own. --update matters even more here: the
    # runs above have just filled the cache with the locked versions.
    fresh_args=()
    for p in "${profiles[@]}"; do
        out="$work/fresh-$p.lock"
        echo "update-lock: resolving $p without a lockfile" >&2
        conan lock create . -pr:a "conan/profiles/$p" --lockfile= --update \
            -r "$remote" --lockfile-out="$out" -verror
        fresh_args+=("--lockfile=$out")
    done
    conan lock merge "${fresh_args[@]}" --lockfile-out="$work/fresh.lock" -verror

    python3 -I - conan.lock "$work/conan.lock" "$work/base.lock" "$work/fresh.lock" "$base_ref" <<'EOF'
import difflib, json, re, sys

KINDS = ("requires", "build_requires", "python_requires", "config_requires")

def load(path):
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    for key, value in data.items():
        if isinstance(value, list):
            data[key] = sorted(re.sub(r"%[0-9.]+$", "", v) if isinstance(v, str) else v
                               for v in value)
    return data

def normalized(data):
    return json.dumps(data, indent=4, sort_keys=True).splitlines()

committed, regenerated, base, fresh = (load(p) for p in sys.argv[1:5])
base_ref = sys.argv[5]
failed = False

# Rule 1: conan.lock is a fixed point of the script.
if normalized(committed) != normalized(regenerated):
    sys.stdout.writelines(line + "\n" for line in difflib.unified_diff(
        normalized(committed), normalized(regenerated),
        "conan.lock (committed)", "conan.lock (regenerated)", lineterm=""))
    sys.stdout.flush()
    print("update-lock: conan.lock is out of date; run conan/update-lock.sh and commit the result",
          file=sys.stderr)
    failed = True

# Rule 2: every entry that is new against the base is what a fresh resolution gives.
stale = [(kind, ref) for kind in KINDS for ref in committed.get(kind, [])
         if ref not in base.get(kind, []) and ref not in fresh.get(kind, [])]
if stale:
    print("update-lock: conan.lock has entries that are neither in the base lock (%s) nor in a "
          "fresh resolution; run conan/update-lock.sh PATTERN for them and commit the result"
          % base_ref, file=sys.stderr)
    for kind, ref in stale:
        print("  %s %s" % (kind, ref))
    sys.stdout.flush()
    failed = True

# Rule 3: an entry that the base and a fresh resolution both have was not removed by hand.
removed = [(kind, ref) for kind in KINDS for ref in base.get(kind, [])
           if ref in fresh.get(kind, []) and ref not in committed.get(kind, [])]
if removed:
    print("update-lock: conan.lock lacks entries that the base lock (%s) and a fresh resolution "
          "both have (removed by hand); run conan/update-lock.sh PATTERN for them and commit the result"
          % base_ref, file=sys.stderr)
    for kind, ref in removed:
        print("  %s %s" % (kind, ref))
    sys.stdout.flush()
    failed = True

if failed:
    sys.exit(1)
print("update-lock: conan.lock is up to date", file=sys.stderr)
EOF
else
    cp "$work/conan.lock" conan.lock
    echo "update-lock: wrote conan.lock" >&2
fi
