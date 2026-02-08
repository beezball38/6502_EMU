# 6502 Emulator - Architecture & Dependency Diagram

## Visual Dependency Graph (Mermaid)

```mermaid
flowchart TB
    subgraph stdlib["Standard Library"]
        stdint["&lt;stdint.h&gt;"]
        stdbool["&lt;stdbool.h&gt;"]
        stddef["&lt;stddef.h&gt;"]
        stdio["&lt;stdio.h&gt;"]
        stdlib_h["&lt;stdlib.h&gt;"]
        string["&lt;string.h&gt;"]
    end

    subgraph external["External Libraries"]
        SDL["&lt;SDL.h&gt;"]
        unity["unity.h"]
    end

    subgraph layer1["Layer 1: Foundation"]
        cpu_defs["cpu_defs.h<br/>─────────────<br/>• byte_t, word_t<br/>• Addressing modes<br/>• Opcodes (X-Macros)<br/>• Status flags"]
    end

    subgraph layer2["Layer 2: Core Components"]
        cpu_h["cpu.h<br/>─────────────<br/>• CPU6502 struct<br/>• Registers (A,X,Y)<br/>• PC, SP, STATUS"]
        ppu_h["ppu.h<br/>─────────────<br/>• PPU struct<br/>• OAM, VRAM<br/>• Framebuffer"]
        ines_h["ines.h<br/>─────────────<br/>• NESCartridge<br/>• iNES header<br/>• Mapper info"]
    end

    subgraph layer3["Layer 3: System Integration"]
        bus_h["bus.h<br/>─────────────<br/>• Bus struct<br/>• Memory mapping<br/>• Read/Write routing"]
    end

    subgraph layer4["Layer 4: Interface"]
        debugger_h["debugger.h<br/>─────────────<br/>• Debugger struct<br/>• SDL window<br/>• Breakpoints"]
    end

    subgraph impl["Implementation Files"]
        cpu_c["cpu.c<br/>─────────────<br/>• 6502 instructions<br/>• Addressing modes<br/>• Cycle counting"]
        ppu_c["ppu.c<br/>─────────────<br/>• Scanline render<br/>• Sprite eval<br/>• VRAM access"]
        bus_c["bus.c<br/>─────────────<br/>• RAM (2KB)<br/>• PRG ROM<br/>• OAM DMA"]
        ines_c["ines.c<br/>─────────────<br/>• ROM loading<br/>• Header parsing<br/>• Mapper setup"]
        debugger_c["debugger.c<br/>─────────────<br/>• SDL rendering<br/>• Memory views<br/>• Step/Run/Break"]
    end

    subgraph tests["Test Suite"]
        cpu_tests["cpu_tests.c<br/>─────────────<br/>• Instruction tests<br/>• Flag tests<br/>• Addressing tests"]
        ppu_tests["ppu_tests.c<br/>─────────────<br/>• Register tests<br/>• Rendering tests<br/>• Timing tests"]
    end

    subgraph utils["Utilities"]
        cpu_trace["cpu_trace.c<br/>─────────────<br/>• ROM execution<br/>• Trace logging<br/>• CLI interface"]
    end

    %% Header Dependencies
    stdint --> cpu_defs
    cpu_defs --> cpu_h
    cpu_defs --> ppu_h
    cpu_defs --> ines_h
    cpu_h --> bus_h
    ppu_h --> bus_h
    cpu_h --> debugger_h
    bus_h --> debugger_h
    SDL --> debugger_h

    %% Implementation Dependencies
    cpu_h --> cpu_c
    bus_h --> cpu_c
    bus_h --> bus_c
    ppu_h --> ppu_c
    ines_h --> ines_c
    debugger_h --> debugger_c
    ines_h --> debugger_c

    %% Test Dependencies
    cpu_h --> cpu_tests
    bus_h --> cpu_tests
    unity --> cpu_tests
    ppu_h --> ppu_tests
    bus_h --> ppu_tests
    cpu_defs --> ppu_tests
    unity --> ppu_tests

    %% Utility Dependencies
    cpu_h --> cpu_trace
    bus_h --> cpu_trace
    ines_h --> cpu_trace

    %% Styling - high contrast with white text
    classDef foundation fill:#1565c0,stroke:#0d47a1,color:#fff
    classDef core fill:#e65100,stroke:#bf360c,color:#fff
    classDef integration fill:#6a1b9a,stroke:#4a148c,color:#fff
    classDef interface fill:#2e7d32,stroke:#1b5e20,color:#fff
    classDef impl fill:#c62828,stroke:#b71c1c,color:#fff
    classDef test fill:#f9a825,stroke:#f57f17,color:#000
    classDef util fill:#37474f,stroke:#263238,color:#fff
    classDef external fill:#616161,stroke:#424242,color:#fff

    class cpu_defs foundation
    class cpu_h,ppu_h,ines_h core
    class bus_h integration
    class debugger_h interface
    class cpu_c,ppu_c,bus_c,ines_c,debugger_c impl
    class cpu_tests,ppu_tests test
    class cpu_trace util
    class SDL,unity external
```

## Simplified Dependency Tree (ASCII)

```
┌─────────────────────────────────────────────────────────────────────┐
│                        STANDARD LIBRARY                              │
│    <stdint.h>  <stdbool.h>  <stddef.h>  <stdio.h>  <string.h>       │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     LAYER 1: FOUNDATION                              │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                      cpu_defs.h                              │    │
│  │  • Type definitions (byte_t, word_t, offset_t)               │    │
│  │  • Addressing mode enums (13 modes)                          │    │
│  │  • Opcode definitions via X-Macros                           │    │
│  │  • CPU status flag definitions                               │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                    │              │              │
                    ▼              ▼              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 2: CORE COMPONENTS                          │
│                                                                      │
│  ┌───────────────┐    ┌───────────────┐    ┌───────────────┐        │
│  │    cpu.h      │    │    ppu.h      │    │   ines.h      │        │
│  │───────────────│    │───────────────│    │───────────────│        │
│  │ CPU6502       │    │ PPU           │    │ NESCartridge  │        │
│  │ • A,X,Y regs  │    │ • OAM (256B)  │    │ • PRG ROM     │        │
│  │ • PC, SP      │    │ • VRAM        │    │ • CHR ROM     │        │
│  │ • STATUS      │    │ • Framebuffer │    │ • Mapper      │        │
│  │ • Cycles      │    │ • Scanline    │    │ • Mirroring   │        │
│  └───────────────┘    └───────────────┘    └───────────────┘        │
└─────────────────────────────────────────────────────────────────────┘
          │                    │                      │
          ▼                    ▼                      │
┌─────────────────────────────────────────────────────────────────────┐
│                   LAYER 3: SYSTEM INTEGRATION                        │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                         bus.h                                │    │
│  │  • System bus connecting CPU ↔ Memory ↔ PPU                  │    │
│  │  • Memory map: $0000-$07FF (RAM), $2000-$2007 (PPU regs)    │    │
│  │  • PRG ROM: $8000-$FFFF                                      │    │
│  │  • OAM DMA transfers                                         │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
          │                                           │
          ▼                                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 4: INTERFACE / TOOLS                        │
│                                                                      │
│  ┌─────────────────────────┐         ┌─────────────────────────┐    │
│  │     debugger.h          │         │     cpu_trace.c         │    │
│  │─────────────────────────│         │─────────────────────────│    │
│  │ • SDL2 window/renderer  │         │ • CLI trace utility     │    │
│  │ • Breakpoint system     │         │ • ROM execution         │    │
│  │ • Memory viewer         │         │ • Instruction logging   │    │
│  │ • CPU/PPU state display │         └─────────────────────────┘    │
│  │ • Pattern table viewer  │                                        │
│  └─────────────────────────┘                                        │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                        IMPLEMENTATION FILES                          │
│                                                                      │
│  cpu.c ──────► Implements 6502 instruction set (256 opcodes)        │
│                All addressing modes, cycle-accurate timing           │
│                                                                      │
│  ppu.c ──────► PPU rendering pipeline, scanline-based               │
│                Background/sprite rendering, VRAM management          │
│                                                                      │
│  bus.c ──────► Memory routing, address decoding                     │
│                RAM mirroring, PRG ROM banking                        │
│                                                                      │
│  ines.c ─────► iNES/NES 2.0 ROM file parser                         │
│                Mapper detection, ROM data extraction                 │
│                                                                      │
│  debugger.c ─► SDL2 GUI, rendering, input handling                  │
│                Disassembly, memory inspection, breakpoints           │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                          TEST SUITE                                  │
│                                                                      │
│  cpu_tests.c ► Unity framework tests for all CPU instructions       │
│                Addressing mode verification, flag behavior           │
│                                                                      │
│  ppu_tests.c ► Unity framework tests for PPU functionality          │
│                Register tests, timing verification                   │
└─────────────────────────────────────────────────────────────────────┘
```

## Include Dependency Matrix

| File          | cpu_defs.h | cpu.h | ppu.h | bus.h | ines.h | debugger.h | SDL.h | unity.h |
|---------------|:----------:|:-----:|:-----:|:-----:|:------:|:----------:|:-----:|:-------:|
| cpu_defs.h    | -          |       |       |       |        |            |       |         |
| cpu.h         | ✓          | -     |       |       |        |            |       |         |
| ppu.h         | ✓          |       | -     |       |        |            |       |         |
| ines.h        | ✓          |       |       |       | -      |            |       |         |
| bus.h         | ✓          | ✓     | ✓     | -     |        |            |       |         |
| debugger.h    |            | ✓     |       | ✓     |        | -          | ✓     |         |
| cpu.c         |            | ✓     |       | ✓     |        |            |       |         |
| ppu.c         |            |       | ✓     |       |        |            |       |         |
| bus.c         |            |       |       | ✓     |        |            |       |         |
| ines.c        |            |       |       |       | ✓      |            |       |         |
| debugger.c    |            |       |       |       | ✓      | ✓          |       |         |
| cpu_tests.c   |            | ✓     |       | ✓     |        |            |       | ✓       |
| ppu_tests.c   | ✓          |       | ✓     | ✓     |        |            |       | ✓       |
| cpu_trace.c   |            | ✓     |       | ✓     | ✓      |            |       |         |

## Data Flow Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                              NES SYSTEM                                   │
│                                                                          │
│   ┌─────────┐                                         ┌─────────┐        │
│   │  ROM    │                                         │ Display │        │
│   │ (.nes)  │                                         │ Output  │        │
│   └────┬────┘                                         └────▲────┘        │
│        │                                                   │             │
│        ▼                                                   │             │
│   ┌─────────┐         ┌─────────────────────┐         ┌────┴────┐        │
│   │  ines   │────────►│        BUS          │────────►│   PPU   │        │
│   │ loader  │         │                     │◄────────│         │        │
│   └─────────┘         │  ┌───────────────┐  │         │ • VRAM  │        │
│                       │  │   RAM (2KB)   │  │         │ • OAM   │        │
│                       │  │ $0000-$07FF   │  │         │ • Tiles │        │
│                       │  └───────────────┘  │         └─────────┘        │
│                       │                     │              ▲             │
│                       │  ┌───────────────┐  │              │             │
│   ┌─────────┐         │  │  PRG ROM      │  │         NMI/IRQ            │
│   │   CPU   │◄───────►│  │ $8000-$FFFF   │  │              │             │
│   │  6502   │         │  └───────────────┘  │              │             │
│   │         │         │                     │              │             │
│   │ • A,X,Y │         │  ┌───────────────┐  │              │             │
│   │ • PC,SP │         │  │  PPU Regs     │  │──────────────┘             │
│   │ • FLAGS │         │  │ $2000-$2007   │  │                            │
│   └─────────┘         └─────────────────────┘                            │
│        ▲                                                                 │
│        │                                                                 │
│   ┌────┴────┐                                                            │
│   │Debugger │  (SDL2 interface for development/testing)                  │
│   └─────────┘                                                            │
└──────────────────────────────────────────────────────────────────────────┘
```

## Build Dependency Order

For compilation, files should be built in this order:

1. **cpu_defs.h** - No dependencies (header only)
2. **cpu.h** - Depends on cpu_defs.h
3. **ppu.h** - Depends on cpu_defs.h
4. **ines.h** - Depends on cpu_defs.h
5. **bus.h** - Depends on cpu.h, ppu.h
6. **debugger.h** - Depends on cpu.h, bus.h, SDL.h
7. **ines.c → ines.o**
8. **ppu.c → ppu.o**
9. **cpu.c → cpu.o**
10. **bus.c → bus.o**
11. **debugger.c → debugger.o** (requires SDL2)
12. **Link all objects** → final executable
