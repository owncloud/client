#!/usr/bin/env bash
# Regenerates conan.lock for all four platforms in conan/profiles/. This is the
# only supported way to change conan.lock.
#
# Usage:
#   OC_RECIPES_DIR=<checkout> conan/update-lock.sh [PATTERN...]
#   OC_RECIPES_DIR=<checkout> conan/update-lock.sh --check
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
#   --check         Write nothing; exit 1 if the regenerated lockfile differs
#                   from the committed conan.lock (the CI drift check).
#
# How it works: once per profile it runs
#   conan lock create . -pr:a <profile> --lockfile=conan.lock --lockfile-clean
# starting from the existing conan.lock (if any), so only what changed moves
# and unused entries are dropped. -pr:a sets the host and the build profile,
# so the build-context tool_requires are those of the target platform, and all
# four platforms can be locked on any machine. The four results are merged
# with `conan lock merge`.
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
patterns=()
for arg in "$@"; do
    case "$arg" in
        --check) check=1 ;;
        -h|--help) sed -n '2,/^$/s/^# \{0,1\}//p' "${BASH_SOURCE[0]}"; exit 0 ;;
        -*) die "unknown option: $arg" ;;
        *) patterns+=("$arg") ;;
    esac
done
if [ "$check" = 1 ] && [ "${#patterns[@]}" -gt 0 ]; then
    die "--check takes no package patterns"
fi

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
            "${bump[@]}" -r "$remote" --lockfile-out="$out" -verror
    else
        conan lock create . -pr:a "conan/profiles/$p" ${base[@]+"${base[@]}"} --lockfile-clean \
            -r "$remote" --lockfile-out="$out" -verror
    fi
    merge_args+=("--lockfile=$out")
done

conan lock merge "${merge_args[@]}" --lockfile-out="$work/merged.lock" -verror
# Drop the export timestamps: "ref#rev%1794407837.72" -> "ref#rev".
sed -E 's/%[0-9]+(\.[0-9]+)?"/"/' "$work/merged.lock" > "$work/conan.lock"

if [ "$check" = 1 ]; then
    python3 -I - conan.lock "$work/conan.lock" <<'EOF'
import difflib, json, re, sys

def normalized(path):
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    for key, value in data.items():
        if isinstance(value, list):
            data[key] = sorted(re.sub(r"%[0-9.]+$", "", v) if isinstance(v, str) else v
                               for v in value)
    return json.dumps(data, indent=4, sort_keys=True).splitlines()

committed, regenerated = normalized(sys.argv[1]), normalized(sys.argv[2])
if committed != regenerated:
    sys.stdout.writelines(line + "\n" for line in difflib.unified_diff(
        committed, regenerated, "conan.lock (committed)", "conan.lock (regenerated)", lineterm=""))
    print("update-lock: conan.lock is out of date; run conan/update-lock.sh and commit the result",
          file=sys.stderr)
    sys.exit(1)
print("update-lock: conan.lock is up to date", file=sys.stderr)
EOF
else
    cp "$work/conan.lock" conan.lock
    echo "update-lock: wrote conan.lock" >&2
fi
