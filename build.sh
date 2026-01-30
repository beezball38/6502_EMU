#!/usr/bin/env bash

case "$1" in
    clean)
        rm -rf build bin
        echo "Clean complete"
        ;;
    test)
        mkdir -p build && cd build && cmake .. && make && cd .. && \
        echo "=== Running unit tests ===" && \
        ./bin/emulator_tests && \
        echo "" && \
        echo "=== Running nestest (legal opcodes) ===" && \
        ./bin/cpu_trace --nestest -q -n 5003
        ;;
    *)
        mkdir -p build && cd build && cmake .. && make
        ;;
esac
