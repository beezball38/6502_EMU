# 6502 Emulator

A 6502 microprocessor emulator written in C. The goal is to build an NES emulator and a general-purpose CPU for emulating other systems that used this processor.

## Status

- All 151 legal opcodes implemented and tested
- Illegal/undocumented opcodes log a warning and continue (stubs for future implementation)
- Passes nestest.nes for all legal opcode tests (~5000 instructions)

## Building

```bash
./build.sh          # Build
./build.sh test     # Build and run tests (unit tests + nestest)
./build.sh clean    # Clean build directory
```

## Usage

```bash
./build/bin/cpu_trace <rom.nes>           # Trace ROM execution
./build/bin/cpu_trace --nestest           # Run nestest validation
./build/bin/emulator_main <rom.nes>       # Run emulator
```
