// nestest.nes automation mode runner
// Compares CPU execution against reference log
//
// Usage: ./nestest_runner <path_to_nestest.nes> [path_to_nestest.log]
//
// Reference log format:
// C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD PPU:  0,  0 CYC:7
//
// Download nestest.nes from: https://nickmass.com/images/nestest.nes
// Reference log: https://www.qmtpro.com/~nes/misc/nestest.log

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "ines.h"

#define MEMORY_SIZE (64 * 1024)
#define NESTEST_START_PC 0xC000
#define MAX_INSTRUCTIONS 10000

// Log entry for comparison
typedef struct {
    word_t pc;
    byte_t a;
    byte_t x;
    byte_t y;
    byte_t p;
    byte_t sp;
} log_entry_t;

// Format current CPU state as nestest log line
static void format_log_line(cpu_s *cpu, char *buffer, size_t size) {
    cpu_instruction_s *instr = get_instruction(cpu, cpu->memory[cpu->PC]);

    // Read instruction bytes
    byte_t bytes[3] = {0};
    bytes[0] = cpu->memory[cpu->PC];
    if (instr->length > 1) bytes[1] = cpu->memory[cpu->PC + 1];
    if (instr->length > 2) bytes[2] = cpu->memory[cpu->PC + 2];

    // Format: PC  BYTES  MNEMONIC  A:XX X:XX Y:XX P:XX SP:XX
    char byte_str[12];
    switch (instr->length) {
        case 1:
            snprintf(byte_str, sizeof(byte_str), "%02X      ", bytes[0]);
            break;
        case 2:
            snprintf(byte_str, sizeof(byte_str), "%02X %02X   ", bytes[0], bytes[1]);
            break;
        case 3:
            snprintf(byte_str, sizeof(byte_str), "%02X %02X %02X", bytes[0], bytes[1], bytes[2]);
            break;
        default:
            snprintf(byte_str, sizeof(byte_str), "??      ");
            break;
    }

    snprintf(buffer, size,
             "%04X  %s  %-4s  A:%02X X:%02X Y:%02X P:%02X SP:%02X",
             cpu->PC, byte_str, instr->name ? instr->name : "???",
             cpu->A, cpu->X, cpu->Y, cpu->STATUS, cpu->SP);
}

// Parse a nestest.log line to extract CPU state
static bool parse_log_line(const char *line, log_entry_t *entry) {
    // Format: C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD ...
    word_t pc;
    byte_t a, x, y, p, sp;

    // Find register values by looking for "A:" pattern
    const char *a_pos = strstr(line, "A:");
    if (!a_pos) return false;

    if (sscanf(line, "%04X", &pc) != 1) return false;
    if (sscanf(a_pos, "A:%02X X:%02X Y:%02X P:%02X SP:%02X", &a, &x, &y, &p, &sp) != 5) {
        return false;
    }

    entry->pc = (word_t)pc;
    entry->a = (byte_t)a;
    entry->x = (byte_t)x;
    entry->y = (byte_t)y;
    entry->p = (byte_t)p;
    entry->sp = (byte_t)sp;

    return true;
}

// Compare CPU state against expected log entry
static bool compare_state(cpu_s *cpu, const log_entry_t *expected, int line_num) {
    bool match = true;

    if (cpu->PC != expected->pc) {
        printf("Line %d: PC mismatch - expected %04X, got %04X\n",
               line_num, expected->pc, cpu->PC);
        match = false;
    }
    if (cpu->A != expected->a) {
        printf("Line %d: A mismatch - expected %02X, got %02X\n",
               line_num, expected->a, cpu->A);
        match = false;
    }
    if (cpu->X != expected->x) {
        printf("Line %d: X mismatch - expected %02X, got %02X\n",
               line_num, expected->x, cpu->X);
        match = false;
    }
    if (cpu->Y != expected->y) {
        printf("Line %d: Y mismatch - expected %02X, got %02X\n",
               line_num, expected->y, cpu->Y);
        match = false;
    }
    if (cpu->STATUS != expected->p) {
        printf("Line %d: P mismatch - expected %02X, got %02X\n",
               line_num, expected->p, cpu->STATUS);
        match = false;
    }
    if (cpu->SP != expected->sp) {
        printf("Line %d: SP mismatch - expected %02X, got %02X\n",
               line_num, expected->sp, cpu->SP);
        match = false;
    }

    return match;
}

// Run a single instruction (fetch, decode, execute)
static void run_instruction(cpu_s *cpu) {
    cpu->current_opcode = cpu->memory[cpu->PC];
    cpu->instruction_pending = true;
    cpu->pc_changed = false;

    cpu_instruction_s *instr = get_current_instruction(cpu);

    // Fetch operand
    if (instr->data_fetch) {
        instr->data_fetch(cpu);
    }

    // Execute
    if (instr->execute) {
        instr->execute(cpu);
    }

    // Advance PC if instruction didn't modify it
    if (!cpu->pc_changed) {
        cpu->PC += instr->length;
    }

    cpu->instruction_pending = false;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <nestest.nes> [nestest.log]\n", argv[0]);
        printf("\nDownload nestest.nes from: https://nickmass.com/images/nestest.nes\n");
        printf("Reference log: https://www.qmtpro.com/~nes/misc/nestest.log\n");
        return 1;
    }

    const char *rom_path = argv[1];
    const char *log_path = argc > 2 ? argv[2] : NULL;

    // Load ROM
    ines_rom_t rom;
    if (!ines_load(rom_path, &rom)) {
        printf("Failed to load ROM: %s\n", rom_path);
        return 1;
    }

    ines_print_info(&rom);

    // Check mapper
    if (rom.mapper != 0) {
        printf("Warning: nestest uses mapper 0, this ROM uses mapper %d\n", rom.mapper);
    }

    // Initialize memory and CPU
    byte_t *memory = malloc(MEMORY_SIZE);
    if (!memory) {
        printf("Failed to allocate memory\n");
        ines_free(&rom);
        return 1;
    }

    cpu_s cpu;
    cpu_init(&cpu, memory);
    init_instruction_table(&cpu);

    // Load PRG ROM into memory
    ines_load_prg_into_memory(&rom, memory);

    // Set PC to $C000 for automation mode (not reset vector)
    cpu.PC = NESTEST_START_PC;

    // nestest expects specific initial state
    cpu.SP = 0xFD;
    cpu.STATUS = 0x24;  // IRQ disabled, unused bit set

    printf("\nStarting nestest at $%04X\n\n", cpu.PC);

    // Open log file if provided
    FILE *log_file = NULL;
    if (log_path) {
        log_file = fopen(log_path, "r");
        if (!log_file) {
            printf("Warning: Could not open log file: %s\n", log_path);
            printf("Running without comparison.\n\n");
        }
    }

    // Run instructions
    char line_buffer[256];
    char cpu_log[256];
    int instruction_count = 0;
    int mismatches = 0;
    bool running = true;

    while (running && instruction_count < MAX_INSTRUCTIONS) {
        // Format current state before execution
        format_log_line(&cpu, cpu_log, sizeof(cpu_log));

        // Compare against log file if available
        if (log_file && fgets(line_buffer, sizeof(line_buffer), log_file)) {
            log_entry_t expected;
            if (parse_log_line(line_buffer, &expected)) {
                if (!compare_state(&cpu, &expected, instruction_count + 1)) {
                    printf("CPU: %s\n", cpu_log);
                    printf("Expected: %s", line_buffer);
                    mismatches++;

                    if (mismatches >= 10) {
                        printf("\nToo many mismatches, stopping.\n");
                        break;
                    }
                }
            }
        } else if (!log_file) {
            // No log file, just print execution
            printf("%s\n", cpu_log);
        }

        // Execute instruction
        run_instruction(&cpu);
        instruction_count++;

        // Check for end conditions
        // nestest writes results to $02 and $03
        // $02 = 0x00 means all tests passed
        // Official test ends around instruction 8991
        if (instruction_count > 8991) {
            running = false;
        }

        // Also stop if we hit BRK at certain addresses (error conditions)
        if (cpu.memory[cpu.PC] == 0x00 && cpu.PC < 0xC000) {
            printf("\nHit BRK at $%04X, stopping.\n", cpu.PC);
            running = false;
        }
    }

    // Print results
    printf("\n=== Results ===\n");
    printf("Instructions executed: %d\n", instruction_count);
    printf("Final PC: $%04X\n", cpu.PC);
    printf("Error code at $02: $%02X\n", memory[0x0002]);
    printf("Error code at $03: $%02X\n", memory[0x0003]);

    if (memory[0x0002] == 0x00 && memory[0x0003] == 0x00) {
        printf("\n*** ALL TESTS PASSED ***\n");
    } else {
        printf("\n*** TESTS FAILED ***\n");
        printf("Check nestest.txt for error code meanings.\n");
    }

    if (log_file) {
        printf("Mismatches with log: %d\n", mismatches);
        fclose(log_file);
    }

    // Cleanup
    free(memory);
    ines_free(&rom);

    return (memory[0x0002] == 0x00 && memory[0x0003] == 0x00) ? 0 : 1;
}
