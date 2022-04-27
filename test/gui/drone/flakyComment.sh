#!/usr/bin/env bash

# $1 - GUI_TEST_REPORT_DIR


# If there are flaky tests file in $1/failed_tests.txt then create a comment listing each lines in the file
if [[ -f $1/failed_tests.txt ]]; then
    echo "creating comment for flaky tests"
    echo "The following tests were flaky:" >> $1/flaky.file
    while read line; do
        echo "- $line" >> $1/flaky.file
    done < $1/failed_tests.txt
fi