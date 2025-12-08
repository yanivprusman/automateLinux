#!/bin/bash
set -e
if [[ " $@ " =~ " -rebuild " ]]; then
    rm -rf build
fi
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" 
cd ..
