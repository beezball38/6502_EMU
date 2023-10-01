# 6502 EMULATOR
# =============
## Description
Emulator for the 6502 microprocessor written in C
## Usage
make && ./bin/cpu_emulator
## TODO
- [ ] Add support for more opcodes
- [ ] Add testing files
- [ ] Add support for multiple stack ranges (currently only 0x0100-0x01FF) to conform to some NES games that use $0100-$019F for VRAM buffers
```