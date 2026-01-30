# 6502 Emulator

A 6502 microprocessor emulator written in C. The goal is to build an NES emulator and a general-purpose CPU for emulating other systems that used this processor.

## Status

- All 151 legal opcodes implemented and tested
- Illegal/undocumented opcodes log a warning and continue (stubs for future implementation)
- Passes nestest.nes for all legal opcode tests (~5000 instructions)

## Building

```bash
./build    # Build the project
./test     # Build and run tests (unit tests + nestest)
./clean    # Clean build artifacts
```

## Usage

```bash
./bin/cpu_trace <rom.nes>           # Trace ROM execution
./bin/cpu_trace --nestest           # Run nestest validation
./bin/emulator_main                 # Run emulator
```
