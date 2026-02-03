
// ===============================
// CPU Trace Tool for NES Emulator
// ===============================
//
// Outputs execution logs for any ROM, optionally compares against a reference log (e.g., nestest).
//
// Usage: cpu_trace <rom.nes> [options]
//        cpu_trace --nestest   (auto-finds roms/nestest.nes and logs/nestest.log)
//
// This tool is essential for debugging and validating CPU emulation accuracy.
//
// For the official nestest log format and test ROM, see:
//   https://www.nesdev.org/wiki/Emulator_tests
//   https://www.nesdev.org/wiki/CPU_tests
//
// Example log line format (matches nestest):
//   C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD CYC:7
//
// ===============
// CPU State Trace
// ===============
//
// The trace log records the following for each instruction:
//   - Program Counter (PC)
//   - Opcode bytes
//   - Mnemonic
//   - Registers: A, X, Y, P (status), SP (stack pointer)
//   - Cycle count
//
// This format is required for compatibility with nestest.log and other reference logs.
//
// For details on the 6502 CPU state and instruction set, see:
//   https://www.nesdev.org/wiki/CPU_registers
//   https://www.nesdev.org/wiki/CPU_instructions
//
// =====================
// Stack and Status Reg
// =====================
//
// The stack is located at $0100-$01FF:
//
//   +--------+  $01FF (top)
//   |  ...   |
//   +--------+
//   |  ...   |
//   +--------+  $0100 (bottom)
//
// The stack pointer (SP) points to the next free byte (offset from $0100).
//
// Status register (P): NV-BDIZC
//   N = Negative, V = Overflow, - = Unused, B = Break, D = Decimal, I = IRQ Disable, Z = Zero, C = Carry
//
// See:
//   https://www.nesdev.org/wiki/Status_flags
//   https://www.nesdev.org/wiki/Stack
//
// =====================
// Log Comparison
// =====================
//
// When --compare is used, the tool checks each instruction against a reference log (e.g., nestest.log).
// Mismatches are reported with details for debugging.
//
// For more on test ROMs and log comparison:
//   https://www.nesdev.org/wiki/Emulator_tests
//
// =====================
//
// (For further technical details, see nesdev.org)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include "cpu.h"
#include "bus.h"
#include "ines.h"
#include "gamecart.h"

#define MEMORY_SIZE (64 * 1024)
#define DEFAULT_MAX_INSTRUCTIONS 10000
#define NESTEST_START_PC 0xC000
#define NESTEST_INITIAL_SP 0xFD
#define NESTEST_INITIAL_STATUS 0x24
#define NESTEST_OFFICIAL_OPCODES_END 5003  // Unofficial opcodes start after this line
#define TRACE_LINE_SIZE 128

// Directory structure
#define ROMS_DIR "roms/"
#define LOGS_DIR "logs/"
#define NESTEST_ROM_PATH ROMS_DIR "nestest.nes"
#define NESTEST_LOG_PATH LOGS_DIR "nestest.log"
#define NESTEST_TRACE_OUTPUT LOGS_DIR "nestest_cpu_trace.log"
#define NESTEST_ERROR_LOG LOGS_DIR "nestest_errors.log"

// Command line options
typedef struct options_t {
    const char *rom_path;
    const char *compare_path;
    const char *output_path;
    int max_instructions;
    int start_pc;           // -1 means use reset vector
    bool nestest_mode;
    bool official_only;     // Only test official opcodes in nestest mode
    bool quiet;
    bool step;              // Step mode: press Enter to step, 'c' to continue
} options_t;

// Log entry for comparison
typedef struct {
    word_t pc;
    byte_t a;
    byte_t x;
    byte_t y;
    byte_t p;
    byte_t sp;
} log_entry_t;

// Check if a file exists
static bool file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// Terminal state for step mode
static struct termios orig_termios;
static bool termios_saved = false;

// Restore terminal to original state
static void restore_terminal(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

// Enable raw mode for single character input
static void enable_raw_mode(void) {
    if (!termios_saved) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        termios_saved = true;
        atexit(restore_terminal);
    }

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    raw.c_cc[VMIN] = 1;               // Read 1 byte at a time
    raw.c_cc[VTIME] = 0;              // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Read a single character in step mode
// Returns: 's' to step, 'c' to continue, 'q' to quit
static char read_step_input(void) {
    enable_raw_mode();
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        restore_terminal();
        return 'q';
    }
    restore_terminal();

    if (c == '\n' || c == '\r' || c == ' ') {
        return 's';  // Step
    } else if (c == 'c' || c == 'C') {
        return 'c';  // Continue
    } else if (c == 'q' || c == 'Q' || c == 3) {  // 3 = Ctrl+C
        return 'q';  // Quit
    }
    return 's';  // Default to step for any other key
}

static void print_usage(const char *program_name) {
    printf("CPU trace tool - outputs execution logs for any ROM\n\n");
    printf("Usage: %s <rom.nes> [options]\n", program_name);
    printf("       %s --nestest   (auto-finds nestest files)\n\n", program_name);
    printf("Options:\n");
    printf("  -c, --compare <log>   Compare against reference log\n");
    printf("  -n, --max <count>     Max instructions to execute (default: %d)\n", DEFAULT_MAX_INSTRUCTIONS);
    printf("  --pc <addr>           Override start PC (hex, e.g. C000)\n");
    printf("  --nestest             Use nestest automation mode (uses %s and %s)\n", NESTEST_ROM_PATH, NESTEST_LOG_PATH);
    printf("  -o, --output <file>   Write trace to file instead of stdout\n");
    printf("  -q, --quiet           Suppress trace output (useful with --compare)\n");
    printf("  -s, --step            Step mode: Enter=step, c=continue, q=quit\n");
    printf("\nExamples:\n");
    printf("  %s roms/game.nes\n", program_name);
    printf("  %s roms/game.nes --pc 8000\n", program_name);
    printf("  %s --nestest\n", program_name);
    printf("  %s roms/game.nes -o logs/trace.log\n", program_name);
    printf("  %s roms/game.nes -s    (step through execution)\n", program_name);
}

static bool parse_args(int argc, char *argv[], options_t *opts) {
    static struct option long_options[] = {
        {"compare", required_argument, NULL, 'c'},
        {"max",     required_argument, NULL, 'n'},
        {"pc",      required_argument, NULL, 'p'},
        {"nestest", no_argument,       NULL, 'N'},
        {"output",  required_argument, NULL, 'o'},
        {"quiet",   no_argument,       NULL, 'q'},
        {"step",    no_argument,       NULL, 's'},
        {"help",    no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    // Initialize defaults
    opts->rom_path = NULL;
    opts->compare_path = NULL;
    opts->output_path = NULL;
    opts->max_instructions = DEFAULT_MAX_INSTRUCTIONS;
    opts->start_pc = -1;
    opts->nestest_mode = false;
    opts->official_only = false;
    opts->quiet = false;
    opts->step = false;

    int opt;
    while ((opt = getopt_long(argc, argv, "c:n:o:qsh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                opts->compare_path = optarg;
                break;
            case 'n':
                opts->max_instructions = atoi(optarg);
                if (opts->max_instructions <= 0) {
                    fprintf(stderr, "Error: Invalid max instruction count\n");
                    return false;
                }
                break;
            case 'p': {
                unsigned int pc;
                if (sscanf(optarg, "%x", &pc) != 1) {
                    fprintf(stderr, "Error: Invalid PC address (expected hex)\n");
                    return false;
                }
                opts->start_pc = (int)pc;
                break;
            }
            case 'N':
                opts->nestest_mode = true;
                break;
            case 'o':
                opts->output_path = optarg;
                break;
            case 'q':
                opts->quiet = true;
                break;
            case 's':
                opts->step = true;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            case '?':
                return false;
        }
    }

    // Handle positional argument (ROM path)
    if (optind < argc) {
        opts->rom_path = argv[optind];
        if (optind + 1 < argc) {
            fprintf(stderr, "Error: Unexpected argument: %s\n", argv[optind + 1]);
            return false;
        }
    }

    // Handle --nestest mode: auto-find ROM and log files
    if (opts->nestest_mode) {
        if (opts->rom_path == NULL) {
            opts->rom_path = NESTEST_ROM_PATH;
        }
        if (opts->compare_path == NULL) {
            opts->compare_path = NESTEST_LOG_PATH;
        }

        if (!file_exists(opts->rom_path)) {
            fprintf(stderr, "Error: nestest ROM not found: %s\n", opts->rom_path);
            return false;
        }
        if (!file_exists(opts->compare_path)) {
            fprintf(stderr, "Error: nestest log not found: %s\n", opts->compare_path);
            return false;
        }
    } else if (opts->rom_path == NULL) {
        fprintf(stderr, "Error: ROM path required\n\n");
        print_usage(argv[0]);
        return false;
    }

    return true;
}

// Format current CPU state as nestest log line
static void format_log_line(cpu_s *cpu, char *buffer, size_t size) {
    cpu_instruction_s *instr = get_instruction(cpu, bus_read(cpu->bus, cpu->PC));

    // Read instruction bytes
    byte_t bytes[3] = {0};
    bytes[0] = bus_read(cpu->bus, cpu->PC);
    if (instr->length > 1) bytes[1] = bus_read(cpu->bus, cpu->PC + 1);
    if (instr->length > 2) bytes[2] = bus_read(cpu->bus, cpu->PC + 2);

    // Format: PC  BYTES  MNEMONIC  A:XX X:XX Y:XX P:XX SP:XX CYC:XXXX
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
             "%04X  %s  %-4s  A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%zu",
             cpu->PC, byte_str, instr->name ? instr->name : "???",
             cpu->A, cpu->X, cpu->Y, cpu->STATUS, cpu->SP, cpu->cycles);
}

// Parse a reference log line to extract CPU state
static bool parse_log_line(const char *line, log_entry_t *entry) {
    // Format: C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD ...
    unsigned int pc, a, x, y, p, sp;

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

// Compare CPU state against expected log entry (writes mismatches to error_log)
static bool compare_state(cpu_s *cpu, const log_entry_t *expected, int line_num,
                          FILE *error_log, const char *cpu_log, const char *expected_line) {
    bool match = true;

    if (cpu->PC != expected->pc) {
        fprintf(error_log, "Line %d: PC mismatch - expected %04X, got %04X\n",
                line_num, expected->pc, cpu->PC);
        match = false;
    }
    if (cpu->A != expected->a) {
        fprintf(error_log, "Line %d: A mismatch - expected %02X, got %02X\n",
                line_num, expected->a, cpu->A);
        match = false;
    }
    if (cpu->X != expected->x) {
        fprintf(error_log, "Line %d: X mismatch - expected %02X, got %02X\n",
                line_num, expected->x, cpu->X);
        match = false;
    }
    if (cpu->Y != expected->y) {
        fprintf(error_log, "Line %d: Y mismatch - expected %02X, got %02X\n",
                line_num, expected->y, cpu->Y);
        match = false;
    }
    if (cpu->STATUS != expected->p) {
        fprintf(error_log, "Line %d: P mismatch - expected %02X, got %02X\n",
                line_num, expected->p, cpu->STATUS);
        match = false;
    }
    if (cpu->SP != expected->sp) {
        fprintf(error_log, "Line %d: SP mismatch - expected %02X, got %02X\n",
                line_num, expected->sp, cpu->SP);
        match = false;
    }

    if (!match) {
        fprintf(error_log, "CPU:      %s\n", cpu_log);
        fprintf(error_log, "Expected: %s\n", expected_line);
    }

    return match;
}

int main(int argc, char *argv[]) {
    options_t opts;
    if (!parse_args(argc, argv, &opts)) {
        return 1;
    }

    // Load cartridge (stack-allocated)
    gamecart_s cart;
    if (!gamecart_load(opts.rom_path, &cart)) {
        fprintf(stderr, "Failed to load ROM: %s\n", opts.rom_path);
        return 1;
    }

    if (!opts.quiet) {
        ines_print_info(&cart.rom);
    }

    // Initialize bus (stack-allocated) and attach components
    bus_s bus;
    bus_init(&bus);

    // Create PPU and CPU on the stack and attach to bus
    ppu_s ppu;
    ppu_init(&ppu);
    bus.ppu = &ppu;

    cpu_s cpu;
    cpu_init(&cpu, &bus);
    bus.cpu = &cpu;

    // Attach cartridge to bus (after PPU is attached so CHR ROM can be loaded)
    bus_attach_cart(&bus, &cart);

    // Set initial CPU state
    if (opts.nestest_mode) {
        // Ask about official opcodes only
        printf("Test official opcodes only? (y/n): ");
        fflush(stdout);
        char response = getchar();
        // Clear any remaining input
        while (getchar() != '\n');
        opts.official_only = (response == 'y' || response == 'Y');

        // nestest automation mode
        cpu.PC = NESTEST_START_PC;
        cpu.SP = NESTEST_INITIAL_SP;
        cpu.STATUS = NESTEST_INITIAL_STATUS;
        if (!opts.quiet) {
            printf("\nNestest mode: PC=$%04X, SP=$%02X, P=$%02X\n",
                   cpu.PC, cpu.SP, cpu.STATUS);
            printf("Testing: %s opcodes\n",
                   opts.official_only ? "official only" : "all (official + unofficial)");
        }
    } else if (opts.start_pc >= 0) {
        // Custom start PC
        cpu.PC = (word_t)opts.start_pc;
        if (!opts.quiet) {
            printf("\nStarting at PC=$%04X (custom)\n", cpu.PC);
        }
    } else {
        // Use reset vector
        word_t reset_vector = bus_read(&bus, 0xFFFC) | (bus_read(&bus, 0xFFFD) << 8);
        cpu.PC = reset_vector;
        if (!opts.quiet) {
            printf("\nStarting at PC=$%04X (reset vector)\n", cpu.PC);
        }
    }

    // Open output file if specified
    FILE *output_file = stdout;
    if (opts.output_path) {
        output_file = fopen(opts.output_path, "w");
        if (!output_file) {
            fprintf(stderr, "Failed to open output file: %s\n", opts.output_path);
            gamecart_free(&cart);
            return 1;
        }
    }

    // Open comparison log file if specified
    FILE *compare_file = NULL;
    if (opts.compare_path) {
        compare_file = fopen(opts.compare_path, "r");
        if (!compare_file) {
            fprintf(stderr, "Warning: Could not open comparison log: %s\n", opts.compare_path);
            fprintf(stderr, "Running without comparison.\n\n");
        }
    }

    // Allocate trace buffer for nestest comparison mode
    char **trace_buffer = NULL;
    int trace_capacity = 0;
    int trace_count = 0;

    if (opts.nestest_mode && compare_file) {
        trace_capacity = opts.max_instructions;
        trace_buffer = malloc(trace_capacity * sizeof(char *));
        if (!trace_buffer) {
            fprintf(stderr, "Failed to allocate trace buffer\n");
            if (compare_file) fclose(compare_file);
            if (output_file != stdout) fclose(output_file);
            gamecart_free(&cart);
            return 1;
        }
    }

    // Open error log file for mismatches
    FILE *error_log = NULL;
    if (compare_file) {
        error_log = fopen(NESTEST_ERROR_LOG, "w");
        if (!error_log) {
            fprintf(stderr, "Warning: Could not open error log: %s\n", NESTEST_ERROR_LOG);
        }
    }

    // Run instructions
    char line_buffer[256];
    char cpu_log[256];
    int instruction_count = 0;
    int mismatches = 0;
    int first_mismatch_line = 0;  // Track where first mismatch occurred
    bool running = true;
    bool stepping = opts.step;    // Track if we're in step mode
    int exit_code = 0;

    if (stepping) {
        printf("Step mode: Enter=step, c=continue, q=quit\n\n");
    }

    while (running && instruction_count < opts.max_instructions) {
        // Format current state before execution
        format_log_line(&cpu, cpu_log, sizeof(cpu_log));

        // Store trace line in buffer if in nestest comparison mode
        if (trace_buffer && trace_count < trace_capacity) {
            trace_buffer[trace_count] = strdup(cpu_log);
            trace_count++;
        }

        // Compare against log file if available
        if (compare_file && fgets(line_buffer, sizeof(line_buffer), compare_file)) {
            // Strip newline from line_buffer for cleaner logging
            size_t len = strlen(line_buffer);
            if (len > 0 && line_buffer[len-1] == '\n') {
                line_buffer[len-1] = '\0';
            }

            log_entry_t expected;
            if (parse_log_line(line_buffer, &expected)) {
                if (error_log && !compare_state(&cpu, &expected, instruction_count + 1,
                                                 error_log, cpu_log, line_buffer)) {
                    mismatches++;

                    if (first_mismatch_line == 0) {
                        first_mismatch_line = instruction_count + 1;
                    }

                    // Stop if testing official only and we hit a mismatch in official section
        if (opts.official_only && first_mismatch_line <= NESTEST_OFFICIAL_OPCODES_END) {
                        break;
                    }

                    // Stop after first mismatch when testing unofficial (they crash the emulator)
                    if (!opts.official_only && first_mismatch_line > NESTEST_OFFICIAL_OPCODES_END) {
                        running = false;
                        break;
                    }
                }
            }
        }

        // Output trace line (unless quiet mode)
        if (!opts.quiet && !compare_file) {
            fprintf(output_file, "%s\n", cpu_log);
            if (output_file != stdout) {
                fflush(output_file);  // Ensure line is written to file before stepping
            }
        }

        // Handle step mode
        if (stepping) {
            // Always print trace line to stdout in step mode
            printf("%s\n", cpu_log);
            printf("[%d] ", instruction_count + 1);
            fflush(stdout);

            char input = read_step_input();
            printf("\r                    \r");  // Clear the prompt
            if (input == 'c') {
                stepping = false;
                printf("Continuing...\n");
            } else if (input == 'q') {
                printf("Quit.\n");
                running = false;
                break;
            }
            // 's' (step) just continues to next iteration
        }

        // Execute instruction
        run_instruction(&cpu);
        instruction_count++;

        // Check for end conditions in nestest mode
        if (opts.nestest_mode) {
            // Stop at official opcodes boundary if only testing official
            if (opts.official_only && instruction_count >= NESTEST_OFFICIAL_OPCODES_END) {
                running = false;
            }

            // nestest writes results to $02 and $03
            // Full test ends around instruction 8991
            if (!opts.official_only && instruction_count > 8991) {
                running = false;
            }

            // Stop if we hit BRK at certain addresses (error conditions)
            if (bus_read(&bus, cpu.PC) == 0x00 && cpu.PC < 0xC000) {
                if (!opts.quiet) {
                    printf("\nHit BRK at $%04X, stopping.\n", cpu.PC);
                }
                running = false;
            }
        }
    }

    // Close error log
    if (error_log) {
        fclose(error_log);
    }

    // Print results
    if (!opts.quiet) {
        printf("\n=== Results ===\n");
        printf("Instructions executed: %d\n", instruction_count);
    }

    // Check nestest result codes and report pass/fail
    if (opts.nestest_mode) {
        if (compare_file) {
            if (opts.official_only) {
                // Testing official opcodes only
                if (mismatches == 0) {
                    printf("\nPASSED: All official opcodes correct\n");
                } else {
                    printf("\nFAILED: Official opcode mismatch at line %d\n", first_mismatch_line);
                    printf("See %s for details\n", NESTEST_ERROR_LOG);
                    exit_code = 1;
                }
            } else {
                // Testing all opcodes
                if (mismatches == 0) {
                    printf("\nPASSED: All opcodes (official + unofficial) correct\n");
                } else if (first_mismatch_line > NESTEST_OFFICIAL_OPCODES_END) {
                    printf("\nPASSED: All official opcodes correct\n");
                    printf("FAILED: Unofficial opcode mismatch at line %d\n", first_mismatch_line);
                    printf("See %s for details\n", NESTEST_ERROR_LOG);
                    // Don't fail exit code for unofficial opcodes
                    exit_code = 0;
                } else {
                    printf("\nFAILED: Official opcode mismatch at line %d\n", first_mismatch_line);
                    printf("See %s for details\n", NESTEST_ERROR_LOG);
                    exit_code = 1;
                }
            }

            // Write trace to file
            if (trace_buffer) {
                FILE *trace_file = fopen(NESTEST_TRACE_OUTPUT, "w");
                if (trace_file) {
                    for (int i = 0; i < trace_count; i++) {
                        fprintf(trace_file, "%s\n", trace_buffer[i]);
                    }
                    fclose(trace_file);
                }
            }

            fclose(compare_file);
        }
    } else if (compare_file) {
        if (mismatches == 0) {
            printf("\nPASSED: No mismatches\n");
        } else {
            printf("\nFAILED: %d mismatches (first at line %d)\n", mismatches, first_mismatch_line);
            printf("See %s for details\n", NESTEST_ERROR_LOG);
            exit_code = 1;
        }
        fclose(compare_file);
    }

    // Cleanup
    if (trace_buffer) {
        for (int i = 0; i < trace_count; i++) {
            free(trace_buffer[i]);
        }
        free(trace_buffer);
    }
    if (output_file != stdout) {
        fclose(output_file);
    }
    gamecart_free(&cart);
    return exit_code;
}
