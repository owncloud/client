#!/usr/bin/env bash

# $1 - template directory
#
# Generates a template file for notification

COMMIT_SHA_SHORT=${DRONE_COMMIT:0:8}
SERVERS=("oc10" "ocis")
BUILD_STATUS=":white_check_mark:**Success**"
TEST_LOGS=""

for server in "${SERVERS[@]}"; do
    LOG_URL_PATH="${CACHE_ENDPOINT}/${CACHE_BUCKET}/${DRONE_REPO}/${DRONE_BUILD_NUMBER}/${server}/guiReportUpload"
    CURL="curl --write-out %{http_code} --silent --output /dev/null"
    GUI_REPORT="${LOG_URL_PATH}/minimal-report.html"

    LOGS="\n${server}: [GUI Test Report]"
    GUI_STATUS_CODE=$($CURL "$GUI_REPORT")
    if [[ "$GUI_STATUS_CODE" == "200" ]]; then
        LOGS+="($GUI_REPORT)"
    else
        LOGS+=" Not Found"
    fi
    if [ "${DRONE_BUILD_STATUS}" == "failure" ]; then
        BUILD_STATUS=":x:**Failure**"
    fi
    TEST_LOGS+="${LOGS}"
done

echo -e "$BUILD_STATUS [${DRONE_REPO}#${COMMIT_SHA_SHORT}](${DRONE_BUILD_LINK}) (${DRONE_BRANCH}) by **${DRONE_COMMIT_AUTHOR}** $TEST_LOGS" >"$1"/template.md
