#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Runs the GUI tests with the in-repo harness in the owncloudci/squish image, without Squish.
# It sets up what the image's startup scripts did for the Squish runner: an X display,
# a D-Bus session bus and an unlocked gnome-keyring (the client stores its credentials there).
#
# Environment:
#   GUI_TEST_PARAMETERS  harness arguments (the tokens Squish used to get), split on whitespace
#   AUT_NAME, AUT_DIR    the application under test, passed as --aut AUT_NAME=AUT_DIR
#   VNC_PW               the keyring password; the image sets it
#   DISPLAY              defaults to :1

set -euo pipefail

: "${GUI_TEST_PARAMETERS:?GUI_TEST_PARAMETERS must hold the harness arguments}"
: "${AUT_NAME:?AUT_NAME must name the application under test}"
: "${AUT_DIR:?AUT_DIR must be the directory of the application under test}"
: "${VNC_PW:?VNC_PW must hold the keyring password}"

export DISPLAY="${DISPLAY:-:1}"
export USER="${USER:-headless}"
export STARTUPDIR="${STARTUPDIR:-/dockerstartup}"
# where the image's VNC session saves its DBUS_SESSION_BUS_ADDRESS (common.sh)
DBUS_ENV_FILE=/tmp/dbus_env

wait_for() { # what timeout_seconds command...
    local what=$1 timeout=$2 start=$SECONDS
    shift 2
    until "$@"; do
        if ((SECONDS - start >= timeout)); then
            echo "[ERROR] Timeout waiting for ${what}" >&2
            exit 1
        fi
        sleep 1
    done
}

display_ready() {
    [[ -S "/tmp/.X11-unix/X${DISPLAY#:}" ]]
}

# after wait_for_keyring in the image's startup scripts
keyring_locked() {
    busctl --user get-property org.freedesktop.secrets /org/freedesktop/secrets/collection/login \
        org.freedesktop.Secret.Collection Locked 2>/dev/null || true
}

keyring_unlocked() {
    [[ "$(keyring_locked)" == *false ]] && return 0
    echo "[INFO] Waiting for the keyring to unlock..."
    echo -n "${VNC_PW}" | gnome-keyring-daemon -r --unlock >/dev/null
    return 1
}

run_harness() {
    wait_for "the keyring" 30 keyring_unlocked
    echo "[INFO] Keyring is ready and unlocked"

    # core dumps for the stack traces of crashed clients
    ulimit -c unlimited || echo "[WARN] Cannot lift the core dump size limit" >&2

    # plain word splitting, no globbing: GUI_TEST_PARAMETERS holds ~@skip and a ';' that stay literal
    set -f
    # shellcheck disable=SC2086
    exec python3.10 -m harness --aut "${AUT_NAME}=${AUT_DIR}" ${GUI_TEST_PARAMETERS}
}

if [[ "${1:-}" == --in-session ]]; then
    # inside dbus-run-session: start the keyring on this bus, as the image's VNC session does
    gnome-keyring-daemon --start --components=secrets >/dev/null
    run_harness
fi

if command -v Xvfb >/dev/null; then
    echo "[INFO] Starting Xvfb on ${DISPLAY}"
    Xvfb "${DISPLAY}" -screen 0 1920x1080x24 &
    wait_for "Xvfb" 30 display_ready
    exec dbus-run-session -- bash "$0" --in-session
fi

# The image has no Xvfb: start its VNC desktop, which also starts D-Bus and the keyring
if [[ ! -x "${STARTUPDIR}/vnc_startup.sh" ]]; then
    echo "[ERROR] Neither Xvfb nor ${STARTUPDIR}/vnc_startup.sh found" >&2
    exit 1
fi
echo "[INFO] Xvfb not found, starting ${STARTUPDIR}/vnc_startup.sh"
rm -f "${DBUS_ENV_FILE}"
"${STARTUPDIR}/vnc_startup.sh" &
# the desktop session writes the file once D-Bus and the keyring are up; a first, short-lived VNC
# server that vnc_startup.sh starts to prepare its configuration never writes it
wait_for "the D-Bus session" 180 test -f "${DBUS_ENV_FILE}"
wait_for "the VNC display" 60 display_ready
# shellcheck source=/dev/null
source "${DBUS_ENV_FILE}"
export DBUS_SESSION_BUS_ADDRESS
run_harness
