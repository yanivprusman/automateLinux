#!/bin/bash
. terminal/theRealPath.sh
echo "--- Running from sourced_test_script.sh ---"
# Pass the debug flag to theRealPath
(theRealPath -debug go.sh) 2>&1
