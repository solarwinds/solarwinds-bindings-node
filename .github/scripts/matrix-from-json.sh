#!/bin/bash

path_to_file="$1"

JSON=$(cat "$path_to_file")
JSON="${JSON//'%'/'%25'}"
JSON="${JSON//$'\n'/'%0A'}"
JSON="${JSON//$'\r'/'%0D'}"

# return a value
echo "::set-output name=matrix::$JSON"
