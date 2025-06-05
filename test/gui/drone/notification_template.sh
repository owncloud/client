#!/usr/bin/env bash

# $1 - template directory
#
# Generates a template file for notification

COMMIT_SHA_SHORT=${DRONE_COMMIT:0:8}
SERVERS=("oc10" "ocis")
BUILD_STATUS="✅ Success"
TEST_LOGS=""
BRANCH_NAME="${DRONE_BRANCH}"
ROOMID="!rnWsCVUmDHDJbiSPMM:matrix.org"

if [ "${DRONE_BUILD_STATUS}" == "failure" ]; then
    BUILD_STATUS="❌️ Failure"
fi

for server in "${SERVERS[@]}"; do
    LOG_URL_PATH="${CACHE_ENDPOINT}/${CACHE_BUCKET}/${DRONE_REPO}/${DRONE_BUILD_NUMBER}/${server}/guiReportUpload"
    CURL="curl --write-out %{http_code} --silent --output /dev/null"

    LOGS=""
    GUI_LOG="${LOG_URL_PATH}/index.html"

    GUI_STATUS_CODE=$($CURL "$GUI_LOG")

    if [[ "$GUI_STATUS_CODE" == "200" ]]; then
        LOGS+=': <a href='"${GUI_LOG}"'>Squish Report</a>'
    fi

    if [ "${DRONE_BUILD_STATUS}" == "failure" ]; then
        SERVER_LOG="${LOG_URL_PATH}/serverlog.log"
        STACKTRACE="${LOG_URL_PATH}/stacktrace.log"

        SERVER_STATUS_CODE=$($CURL "$SERVER_LOG")
        STACKTRACE_STATUS_CODE=$($CURL "$STACKTRACE")

        if [[ "$SERVER_STATUS_CODE" == "200" ]]; then
            LOGS+='<br> <a href='"${SERVER_LOG}"'>Server Log</a> <br>'
        fi
        if [[ "$STACKTRACE_STATUS_CODE" == "200" ]]; then
            LOGS+='<br> <a href='"${STACKTRACE}"'>Stacktrace</a>'
        fi
    fi

    if [[ -n "${LOGS}" ]]; then
        LOGS="${server}${LOGS}"
        TEST_LOGS+="${LOGS}"
    fi
done

if [ "${DRONE_BUILD_EVENT}" == "tag" ]; then
    BRANCH_NAME="Tag: \`${DRONE_TAG}\`"
fi

# helper functions
log_error() {
  echo -e "\e[31m$1\e[0m"
}

log_info() {
  echo -e "\e[37m$1\e[0m"
}

log_success() {
  echo -e "\e[32m$1\e[0m"
}

message_html='<b>'$BUILD_STATUS'</b> <a href="'${DRONE_BUILD_LINK}'">'${DRONE_REPO}'#'$COMMIT_SHA_SHORT'</a> ('${BRANCH_NAME}') by <b>'${DRONE_COMMIT_AUTHOR}'</b> <br> <b>'"${TEST_LOGS}"'</b>'
message_html=$(echo "$message_html" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g')

log_info "Sending report to the element chat..."

response=$(curl -s -o /dev/null -X PUT -w "%{http_code}" 'https://matrix.org/_matrix/client/v3/rooms/'$ROOMID'/send/m.room.message/'$(date +%s) \
  -H "Authorization: Bearer "$MATRIX_TOKEN \
  -H 'Content-Type: application/json' \
  -d '{
    "msgtype": "m.text",
    "body": "'"$message_html"'",
    "format": "org.matrix.custom.html",
    "formatted_body": "'"$message_html"'"
  }')

if [[ "$response" != "200" ]]; then
  log_error "❌ Error: Failed to send notification to element. Expected status code 200, but got $response."
  exit 1
fi

log_success "✅ Notification successfully sent to Element chat (ownCloud Infinite Scale Alerts)"
