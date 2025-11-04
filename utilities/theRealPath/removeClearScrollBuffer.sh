#!/bin/bash
sed -i 's/\x1b\[3J//g' "$1"
