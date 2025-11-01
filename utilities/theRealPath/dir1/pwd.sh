#!/bin/bash
# echo $PWD
echo "$(cd "$(dirname "$0")" && pwd)"
