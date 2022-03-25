#! /bin/bash

set -eo pipefail

show_usage() {
    echo "Usage: ${0} <core dumps...>"
}

# check number of params
if [[ "${1}" == "" ]]; then
    echo "Warning: no arguments passed"
    show_usage
    exit 0
fi

# the coredump pattern we expect is /tmp/core-%t-%p-%s-%E
# %t: timestamp
# %u: process ID
# %s: signal number
# %E: encoded executable name
# see man 5 core for more information
# the order ensures easy parsing, as the first three elements are numeric and cannot contain additional - signs
for coredump in "${@}"; do
    filename="$(basename "${coredump}")"

    if [[ ! "${filename}" =~ ^core-[0-9]+-[0-9]+-[0-9]+- ]]; then
        echo "Error: invalid filename: ${coredump} does not match expected pattern, skipping"
        continue
    fi

    timestamp="$(date "+%Y/%m/%d %H:%M:%S" -d @"$(cut -d- -f2 <<<"${filename}")")"
    procid="$(cut -d- -f3 <<<"${filename}")"
    signal="$(cut -d- -f4 <<<"${filename}")"
    executable="$(cut -d- -f5- <<<"${filename}" | tr '!' '/')"

    echo "=== ${timestamp} process ${procid} crashed with signal ${signal} (executable: ${executable}) ==="

    if [[ ! -f "${executable}" ]]; then
        echo "Error: could not find executable ${executable}, skipping"
        continue
    fi

    gdb -n --batch -ex bt "${executable}" "${coredump}"
done
