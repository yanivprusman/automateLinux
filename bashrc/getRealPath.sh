#!/bin/bash
path="$1"
path="${path#/}"
# Use PWD to get the current working directory
readlink -f "${PWD}/${path}"