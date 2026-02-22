/**
 * -a              Append to output file instead of truncating
 * -c <command>    Execute command instead of interactive shell
 * -q              Quiet mode (no start/done messages)
 * -e              Return child's exit status
 * -f              Flush output after each write (BONUS)
 * -h              Print help and exit
 * [file]          Output file (positional argument)
 */

#include "ft_script.h"
#include <getopt.h>  /* getopt, optarg, optind */

void init_options(script_options *options)
{
    options->output_file = NULL; // (will use DEFAULT_TYPESCRIPT)
    options->command = NULL; // (interactive shell)
    options->append_mode = 0; // (truncate)
    options->quiet_mode = 0; // (print messages)
    options->return_exit_status = 0; // (always exit 0)
    options->flush_mode = 0; // (buffered output)
}

void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [options] [file]\n", program_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Make a typescript of terminal session.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a              Append to output file instead of overwriting\n");
    fprintf(stderr, "  -c <command>    Execute command instead of interactive shell\n");
    fprintf(stderr, "  -q              Quiet mode (suppress start/done messages)\n");
    fprintf(stderr, "  -e              Return exit status of child process\n");
    fprintf(stderr, "  -f              Flush output after each write (BONUS)\n");
    fprintf(stderr, "  -h              Display this help and exit\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "If no file is specified, output is written to 'typescript'.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s                      # Record to 'typescript'\n", program_name);
    fprintf(stderr, "  %s -a session.log       # Append to 'session.log'\n", program_name);
    fprintf(stderr, "  %s -c 'ls -la' out.txt  # Run command and log to 'out.txt'\n", program_name);
}

int parse_arguments(int argc, char *argv[], script_options *options)
{
    int opt;

    while ((opt = getopt(argc, argv, ":ac:qefh")) != -1) {
        switch (opt) {
            case 'a':
                /*
                 * -a: Append mode
                 */
                options->append_mode = 1;
                break;

            case 'c':
                /*
                 * -c <command>: Execute command
                 */
                options->command = optarg;
                break;

            case 'q':
                /*
                 * -q: Quiet mode
                 */
                options->quiet_mode = 1;
                break;

            case 'e':
                /*
                 * -e: Return child exit status
                 */
                options->return_exit_status = 1;
                break;

            case 'f':
                /*
                 * -f: Flush mode (BONUS)
                 */
                options->flush_mode = 1;
                break;

            case 'h':
                /*
                 * -h: Help
                 */
                print_usage(argv[0]);
                return -1;  /* Caller should exit (not an error, but stop processing) */

            case ':':
                /*
                 * Missing argument for option
                 * optopt contains the option character
                 */
                fprintf(stderr, "Error: Option -%c requires an argument\n", optopt);
                print_usage(argv[0]);
                return -1;

            case '?':
                /*
                 * Unknown option
                 * optopt contains the option character (if printable)
                 */
                if (optopt != 0 && optopt != '?') {
                    fprintf(stderr, "Error: Unknown option -%c\n", optopt);
                } else {
                    fprintf(stderr, "Error: Unknown option\n");
                }
                print_usage(argv[0]);
                return -1;

            default:
                fprintf(stderr, "Error: Unexpected getopt return value\n");
                return -1;
        }
    }

    /*
     * After getopt() finishes, optind points to first non-option argument
     */
    if (optind < argc) {
        options->output_file = argv[optind];
        if (optind + 1 < argc) {
            fprintf(stderr, "Error: Too many arguments\n");
            print_usage(argv[0]);
            return -1;
        }
    }
    return 0;
}
