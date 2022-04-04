#!/usr/bin/env bash

# $1 is the file containing list of all failed tests

# since we always rerun the test at least once, if there is 1 entry of the same test then it is a flaky test
# at the end of the script, the file $1 will contain only the flaky tests

echo "Creating a list of flaky tests..."

sort $1 | uniq --count | while read line; do
    if [[ $line == *"1"* ]]; then
        echo ${line:1} >> $1.temp
    fi
done

mv $1.temp $1
rm $1.temp

if [[ $(cat $1 | wc -l) -eq 0 ]]; then
    echo "No flaky tests found"
    rm $1
fi