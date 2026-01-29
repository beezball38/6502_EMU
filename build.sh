#!/bin/bash

case "$1" in
    clean)
        rm -rf build
        echo "Clean complete"
        ;;
    test)
        mkdir -p build && cd build && cmake .. && make && ./bin/tests/emulator_tests
        ;;
    *)
        mkdir -p build && cd build && cmake .. && make
        ;;
esac
