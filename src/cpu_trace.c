#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include "nes.h"
#include "ines.h"
#include "gamecart.h"

#define MEMORY_SIZE (64 * 1024)
#define DEFAULT_MAX_INSTRUCTIONS 10000
#define NESTEST_START_PC 0xC000
#define NESTEST_INITIAL_SP 0xFD
#define NESTEST_INITIAL_STATUS 0x24
#define NESTEST_OFFICIAL_OPCODES_END 5003
#define TRACE_LINE_SIZE 128

#define ROMS_DIR "roms/"
#define LOGS_DIR "logs/"
#define NESTEST_ROM_PATH ROMS_DIR "nestest.nes"
#define NESTEST_LOG_PATH LOGS_DIR "nestest.log"
#define NESTEST_TRACE_OUTPUT LOGS_DIR "nestest_cpu_trace.log"
#define NESTEST_ERROR_LOG LOGS_DIR "nestest_errors.log"

typedef struct options_t {
    const char *rom_path;
    const char *compare_path;
    const char *output_path;
    int max_instructions;
    int start_pc;
    bool nestest_mode;
    bool official_only;
    bool quiet;
    bool step;
} options_t;

typedef struct {
    word_t pc;
    byte_t a;
    byte_t x;
    byte_t y;
    byte_t p;
    byte_t sp;
} log_entry_t;

static bool file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

static struct termios orig_termios;
static bool termios_saved = false;

static void restore_terminal(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

static void enable_raw_mode(void) {
    if (!termios_saved) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        termios_saved = true;
        atexit(restore_terminal);
    }

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static char read_step_input(void) {
    enable_raw_mode();
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        restore_terminal();
        return 'q';
    }
    restore_terminal();

    if (c == '\n' || c == '\r' || c == ' ') {
        return 's';
    } else if (c == 'c' || c == 'C') {
        return 'c';
    } else if (c == 'q' || c == 'Q' || c == 3) {
        return 'q';
    }
    return 's';
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

    if (optind < argc) {
        opts->rom_path = argv[optind];
        if (optind + 1 < argc) {
            fprintf(stderr, "Error: Unexpected argument: %s\n", argv[optind + 1]);
            return false;
        }
    }

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

static void format_log_line(cpu_s *cpu, char *buffer, size_t size) {
    bus_s *bus = cpu->bus;
    cpu_instruction_s *instr = get_instruction(cpu, bus_read(bus, cpu->PC));

    byte_t bytes[3] = {0};
    bytes[0] = bus_read(bus, cpu->PC);
    if (instr->length > 1) bytes[1] = bus_read(bus, cpu->PC + 1);
    if (instr->length > 2) bytes[2] = bus_read(bus, cpu->PC + 2);

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

static bool parse_log_line(const char *line, log_entry_t *entry) {
    unsigned int pc, a, x, y, p, sp;

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

    gamecart_s cart;
    if (!gamecart_load(opts.rom_path, &cart)) {
        fprintf(stderr, "Failed to load ROM: %s\n", opts.rom_path);
        return 1;
    }

    if (!opts.quiet) {
        ines_print_info(&cart.rom);
    }

    nes_console_s *nes = nes_get_instance();
    nes_init(nes);
    cpu_s *cpu = nes->cpu;
    bus_s *bus = nes->bus;

    nes_attach_cart(nes, &cart);

    if (opts.nestest_mode) {
        printf("Test official opcodes only? (y/n): ");
        fflush(stdout);
        char response = getchar();
        while (getchar() != '\n');
        opts.official_only = (response == 'y' || response == 'Y');

        cpu->PC = NESTEST_START_PC;
        cpu->SP = NESTEST_INITIAL_SP;
        cpu->STATUS = NESTEST_INITIAL_STATUS;
        if (!opts.quiet) {
            printf("\nNestest mode: PC=$%04X, SP=$%02X, P=$%02X\n",
                   cpu->PC, cpu->SP, cpu->STATUS);
            printf("Testing: %s opcodes\n",
                   opts.official_only ? "official only" : "all (official + unofficial)");
        }
    } else if (opts.start_pc >= 0) {
        cpu->PC = (word_t)opts.start_pc;
        if (!opts.quiet) {
            printf("\nStarting at PC=$%04X (custom)\n", cpu->PC);
        }
    } else {
        word_t reset_vector = bus_read(bus, 0xFFFC) | (bus_read(bus, 0xFFFD) << 8);
        cpu->PC = reset_vector;
        if (!opts.quiet) {
            printf("\nStarting at PC=$%04X (reset vector)\n", cpu->PC);
        }
    }

    FILE *output_file = stdout;
    if (opts.output_path) {
        output_file = fopen(opts.output_path, "w");
        if (!output_file) {
            fprintf(stderr, "Failed to open output file: %s\n", opts.output_path);
            gamecart_free(&cart);
            return 1;
        }
    }

    FILE *compare_file = NULL;
    if (opts.compare_path) {
        compare_file = fopen(opts.compare_path, "r");
        if (!compare_file) {
            fprintf(stderr, "Warning: Could not open comparison log: %s\n", opts.compare_path);
            fprintf(stderr, "Running without comparison.\n\n");
        }
    }

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

    FILE *error_log = NULL;
    if (compare_file) {
        error_log = fopen(NESTEST_ERROR_LOG, "w");
        if (!error_log) {
            fprintf(stderr, "Warning: Could not open error log: %s\n", NESTEST_ERROR_LOG);
        }
    }

    char line_buffer[256];
    char cpu_log[256];
    int instruction_count = 0;
    int mismatches = 0;
    int first_mismatch_line = 0;
    bool running = true;
    bool stepping = opts.step;
    int exit_code = 0;

    if (stepping) {
        printf("Step mode: Enter=step, c=continue, q=quit\n\n");
    }

    while (running && instruction_count < opts.max_instructions) {
        format_log_line(cpu, cpu_log, sizeof(cpu_log));

        if (trace_buffer && trace_count < trace_capacity) {
            trace_buffer[trace_count] = strdup(cpu_log);
            trace_count++;
        }

        if (compare_file && fgets(line_buffer, sizeof(line_buffer), compare_file)) {
            size_t len = strlen(line_buffer);
            if (len > 0 && line_buffer[len-1] == '\n') {
                line_buffer[len-1] = '\0';
            }

            log_entry_t expected;
            if (parse_log_line(line_buffer, &expected)) {
                if (error_log && !compare_state(cpu, &expected, instruction_count + 1,
                                                 error_log, cpu_log, line_buffer)) {
                    mismatches++;

                    if (first_mismatch_line == 0) {
                        first_mismatch_line = instruction_count + 1;
                    }

        if (opts.official_only && first_mismatch_line <= NESTEST_OFFICIAL_OPCODES_END) {
                        break;
                    }

                    if (!opts.official_only && first_mismatch_line > NESTEST_OFFICIAL_OPCODES_END) {
                        running = false;
                        break;
                    }
                }
            }
        }

        if (!opts.quiet && !compare_file) {
            fprintf(output_file, "%s\n", cpu_log);
            if (output_file != stdout) {
                fflush(output_file);
            }
        }

        if (stepping) {
            printf("%s\n", cpu_log);
            printf("[%d] ", instruction_count + 1);
            fflush(stdout);

            char input = read_step_input();
            printf("\r                    \r");
            if (input == 'c') {
                stepping = false;
                printf("Continuing...\n");
            } else if (input == 'q') {
                printf("Quit.\n");
                running = false;
                break;
            }
        }

        run_instruction(cpu);
        instruction_count++;

        if (opts.nestest_mode) {
            if (opts.official_only && instruction_count >= NESTEST_OFFICIAL_OPCODES_END) {
                running = false;
            }

            if (!opts.official_only && instruction_count > 8991) {
                running = false;
            }

            if (bus_read(bus, cpu->PC) == 0x00 && cpu->PC < 0xC000) {
                if (!opts.quiet) {
                    printf("\nHit BRK at $%04X, stopping.\n", cpu->PC);
                }
                running = false;
            }
        }
    }

    if (error_log) {
        fclose(error_log);
    }

    if (!opts.quiet) {
        printf("\n=== Results ===\n");
        printf("Instructions executed: %d\n", instruction_count);
    }

    if (opts.nestest_mode) {
        if (compare_file) {
            if (opts.official_only) {
                if (mismatches == 0) {
                    printf("\nPASSED: All official opcodes correct\n");
                } else {
                    printf("\nFAILED: Official opcode mismatch at line %d\n", first_mismatch_line);
                    printf("See %s for details\n", NESTEST_ERROR_LOG);
                    exit_code = 1;
                }
            } else {
                if (mismatches == 0) {
                    printf("\nPASSED: All opcodes (official + unofficial) correct\n");
                } else if (first_mismatch_line > NESTEST_OFFICIAL_OPCODES_END) {
                    printf("\nPASSED: All official opcodes correct\n");
                    printf("FAILED: Unofficial opcode mismatch at line %d\n", first_mismatch_line);
                    printf("See %s for details\n", NESTEST_ERROR_LOG);
                    exit_code = 0;
                } else {
                    printf("\nFAILED: Official opcode mismatch at line %d\n", first_mismatch_line);
                    printf("See %s for details\n", NESTEST_ERROR_LOG);
                    exit_code = 1;
                }
            }

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
