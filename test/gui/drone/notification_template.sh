#!/usr/bin/env bash

# args:
#
# $1 - template directory
# $2 - report type (custom)
#
# Generates a template file for notification

COMMIT_SHA_SHORT=${DRONE_COMMIT:0:8}
SERVERS=("oc10" "ocis")
BUILD_STATUS=":white_check_mark:**Success**"
TEST_LOGS=""

if [ "${DRONE_BUILD_STATUS}" == "failure" ]; then
    BUILD_STATUS=":x:**Failure**"
fi

for server in "${SERVERS[@]}"; do
    LOG_URL_PATH="${CACHE_ENDPOINT}/${CACHE_BUCKET}/${DRONE_REPO}/${DRONE_BUILD_NUMBER}/${server}/guiReportUpload"
    CURL="curl --write-out %{http_code} --silent --output /dev/null"

    LOGS=""

    if [[ "$2" == "custom" ]]; then
        GUI_CUSTOM_REPORT="${LOG_URL_PATH}/minimal-report.html"
        STATUS_CODE=$($CURL "$GUI_CUSTOM_REPORT")
        if [[ "$STATUS_CODE" == "200" ]]; then
            LOGS+=": [GUI Test Report]($GUI_CUSTOM_REPORT)"
        fi
    else
        if [ "${DRONE_BUILD_STATUS}" == "failure" ]; then
            GUI_LOG="${LOG_URL_PATH}/index.html"
            SERVER_LOG="${LOG_URL_PATH}/serverlog.log"
            STACKTRACE="${LOG_URL_PATH}/stacktrace.log"

            GUI_STATUS_CODE=$($CURL "$GUI_LOG")
            SERVER_STATUS_CODE=$($CURL "$SERVER_LOG")
            STACKTRACE_STATUS_CODE=$($CURL "$STACKTRACE")

            if [[ "$GUI_STATUS_CODE" == "200" ]]; then
                LOGS+=": [Report]($GUI_LOG)"
            fi
            if [[ "$SERVER_STATUS_CODE" == "200" ]]; then
                LOGS+="\n> [Server log]($SERVER_LOG)  " # 2 spaces at the end act as line-break
            fi
            if [[ "$STACKTRACE_STATUS_CODE" == "200" ]]; then
                LOGS+="\n> [Stacktrace]($STACKTRACE)"
            fi
        fi
    fi

    if [[ -n "${LOGS}" ]]; then
        LOGS="\n${server}${LOGS}"
        TEST_LOGS+="${LOGS}"
    fi
done

echo -e "$BUILD_STATUS [${DRONE_REPO}#${COMMIT_SHA_SHORT}](${DRONE_BUILD_LINK}) (${DRONE_BRANCH}) by **${DRONE_COMMIT_AUTHOR}** $TEST_LOGS" #>"$1"/template.md
