# NES Emulator

An NES emulator written in C, featuring a cycle-accurate 6502 CPU implementation.

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

## Tools

Development utilities for working with NES ROMs.

```bash
# Setup (one time)
cd tools && python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt

# Dump CHR ROM tiles to PNG
source tools/venv/bin/activate
python3 tools/scripts/dump_chr.py roms/game.nes         # -> tools/dumps/game_chr.png
python3 tools/scripts/dump_chr.py roms/game.nes -s 2    # 2x scale
python3 tools/scripts/dump_chr.py roms/game.nes --info  # Print ROM info only
```
