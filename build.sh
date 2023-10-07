#!/bin/bash
make clean;
make && ./bin/tests/test_instructions && ./bin/cpu_emulator;
