/**
 * @file ft_script.h
 * @brief Educational implementation of the Unix script command (C98 standard)
 *
 * This header defines the core data structures and function interfaces for
 * ft_script, which creates a typescript of terminal sessions using
 * pseudo-terminals (PTY).
 *
 * STANDARD: C98 (ISO/IEC 9899:1990)
 * PLATFORM: Linux
 *
 * ARCHITECTURE OVERVIEW:
 * ======================
 *
 * The script command works by creating a pseudo-terminal (PTY) which consists
 * of two endpoints:
 *
 * 1. MASTER side: Controlled by the parent process (our ft_script program)
 * 2. SLAVE side: Used by the child process (the shell or command being run)
 *
 * The PTY acts like a bidirectional pipe but with terminal semantics - it
 * handles line discipline, terminal control codes, and signals.
 *
 * PROCESS STRUCTURE:
 * ==================
 *
 *    - User Terminal\n
 *       |\n
 *       | (stdin/stdout)\n
 *       v\n
 *   [Parent Process]\n
 *    - Reads from user terminal stdin
 *    - Writes to PTY master (sends to child)
 *    - Reads from PTY master (receives from child)
 *    - Writes to user terminal stdout
 *    - Logs output to typescript file
 *       |\n
 *       | (PTY master/slave)\n
 *         v\n
 *   [Child Process]\n
 *    - stdin/stdout/stderr connected to PTY slave
 *    - Runs shell or specified command
 *    - Behaves as if connected to a real terminal
 *
 * KEY SYSTEM CALLS USED:
 * ======================
 *
 * PTY Creation:
 * - posix_openpt(3) or open(2) with /dev/ptmx - Creates PTY master
 * - grantpt(3) - Changes ownership of slave to calling user
 * - unlockpt(3) - Unlocks the slave PTY
 * - ptsname(3) - Gets the name of the slave PTY device
 *
 * Process Management:
 * - fork(2) - Creates child process
 * - setsid(2) - Creates new session (child becomes session leader)
 * - execve(2) - Replaces child process with shell
 *
 * Terminal Control:
 * - tcgetattr(2) - Gets terminal attributes
 * - tcsetattr(2) - Sets terminal attributes
 * - ioctl(2) - Controls terminal device (TIOCSWINSZ for window size)
 *
 * I/O Multiplexing:
 * - select(2) - Waits for multiple file descriptors to become ready
 *
 * Signal Handling:
 * - signal(2) or sigaction(2) - Sets up signal handlers
 * - SIGCHLD - Notifies when child process terminates
 * - SIGWINCH - Notifies when terminal window size changes
 *
 */

#ifndef FT_SCRIPT_H
#define FT_SCRIPT_H

/* ========================================================================== */
/* STANDARD C98 INCLUDES                                                      */
/* ========================================================================== */

#include <stdio.h>      /* FILE, printf, fprintf, perror, fopen, fclose */
#include <stdlib.h>     /* EXIT_SUCCESS, EXIT_FAILURE, malloc, free, getenv */
#include <string.h>     /* strlen, strcpy, strcmp, strerror */
#include <time.h>       /* time, ctime (explicitly allowed by project) */

/* ========================================================================== */
/* POSIX/UNIX SYSTEM INCLUDES                                                 */
/* ========================================================================== */

#include <unistd.h>     /* read, write, close, fork, execve, setsid, isatty */
#include <fcntl.h>      /* open, O_RDWR, O_CREAT, O_APPEND, O_WRONLY */
#include <sys/types.h>  /* pid_t, size_t */
#include <sys/stat.h>   /* mode_t, file permissions */
#include <sys/wait.h>   /* wait, waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED */
#include <sys/ioctl.h>  /* ioctl, TIOCSWINSZ, TIOCGWINSZ */
#include <sys/select.h> /* select, fd_set, FD_ZERO, FD_SET, FD_ISSET */
#include <termios.h>    /* struct termios, tcgetattr, tcsetattr, TCSANOW */
#include <signal.h>     /* signal, SIGCHLD, SIGWINCH, SIG_DFL */
#include <errno.h>      /* errno, EINTR, EAGAIN */

/* PTY-specific includes */
#define _XOPEN_SOURCE 600  /* For posix_openpt, ptsname */
#include <stdlib.h>     /* posix_openpt, grantpt, unlockpt, ptsname */

/* ========================================================================== */
/* CONSTANTS AND MACROS                                                       */
/* ========================================================================== */

/**
 * Default output file when no filename is specified
 */
#define DEFAULT_TYPESCRIPT "typescript"

/**
 * Default shell when SHELL environment variable is not set
 */
#define DEFAULT_SHELL "/bin/sh"

/**
 * Buffer size for I/O operations (8KB is a good balance)
 * - Too small: excessive system calls
 * - Too large: increased latency, memory usage
 */
#define BUFFER_SIZE 8192

/**
 * Maximum length for command string (-c option)
 */
#define MAX_COMMAND_LENGTH 4096

/**
 * Exit status when child process is terminated by signal
 * Follows bash convention: 128 + signal_number
 */
#define EXIT_STATUS_SIGNAL_BASE 128

/* ========================================================================== */
/* TYPE DEFINITIONS                                                           */
/* ========================================================================== */

/**
 * @struct script_options
 * @brief Configuration options for the script command
 *
 * This structure holds all command-line options and their state.
 * It encapsulates configuration to make it easy to pass around and test.
 *
 * Ownership: Created on stack in main(), passed by pointer to functions
 * Lifetime: Exists for entire program execution
 */
typedef struct script_options {
    /**
     * Output file path
     * - NULL: use DEFAULT_TYPESCRIPT
     * - Non-NULL: caller-provided filename
     * Ownership: Points to argv string, do NOT free
     */
    const char *output_file;

    /**
     * Command to execute (for -c option)
     * - NULL: run interactive shell
     * - Non-NULL: execute this command
     * Ownership: Points to argv string, do NOT free
     */
    const char *command;

    /**
     * Append mode flag (-a option)
     * - 0: truncate existing file (default)
     * - 1: append to existing file
     */
    int append_mode;

    /**
     * Quiet mode flag (-q option)
     * - 0: print start/done messages (default)
     * - 1: suppress messages
     */
    int quiet_mode;

    /**
     * Return child exit status (-e option)
     * - 0: always exit with 0 (default)
     * - 1: exit with child's exit status
     */
    int return_exit_status;

    /**
     * Flush output after each write (-f option)
     * - 0: use buffered output (default)
     * - 1: flush after each write
     *
     * Note: This is a BONUS feature. When enabled, fflush() is called
     * after each write to the output file, ensuring real-time updates.
     * This is useful for monitoring scripts in progress but has a
     * performance impact.
     */
    int flush_mode;

} script_options;

/**
 * @struct script_state
 * @brief Runtime state of the script session
 *
 * This structure holds all the runtime state needed for the I/O loop.
 * Separating state from options makes the design cleaner and easier to test.
 *
 * Ownership: Created on stack or heap, cleaned up explicitly
 * Lifetime: From PTY creation to child process termination
 */
typedef struct script_state {
    /**
     * PTY master file descriptor
     * - Used by parent to communicate with child
     * - Must be closed when session ends
     * Ownership: We own this, must close it
     */
    int master_fd;

    /**
     * Output file descriptor (typescript file)
     * - Where we log the session output
     * - Must be closed when session ends
     * Ownership: We own this, must close it
     */
    int output_fd;

    /**
     * Child process ID
     * - Used to wait for child and get exit status
     * - Set by fork(), used by wait()
     */
    pid_t child_pid;

    /**
     * Child exit status
     * - Raw status from wait()
     * - Use WIFEXITED/WEXITSTATUS/WIFSIGNALED to interpret
     */
    int child_status;

    /**
     * Original terminal attributes (stdin)
     * - Saved at start, restored at end
     * - Required to put terminal into raw mode and restore it
     * Ownership: We own this data
     */
    struct termios original_termios;

    /**
     * Flag indicating if terminal is in raw mode
     * - 0: normal mode
     * - 1: raw mode (must restore on exit)
     */
    int terminal_modified;

} script_state;

/* ========================================================================== */
/* FUNCTION DECLARATIONS - MAIN PROGRAM FLOW                                  */
/* ========================================================================== */

/**
 * Parses command-line arguments and populates options structure
 *
 * Uses getopt(3) to parse command-line options in a standard way.
 * This function follows the POSIX conventions for option parsing.
 *
 * @param argc Argument count from main()
 * @param argv Argument vector from main()
 * @param options Output parameter for parsed options (must be non-NULL)
 *
 * Precondition: options must point to valid script_options structure
 * Postcondition: options is populated with parsed values
 *
 * @return 0 on success, -1 on error (invalid option or missing argument)
 *
 * Side effects: Prints error messages to stderr on parse errors
 */
int parse_arguments(int argc, char *argv[], script_options *options);

/**
 * Prints usage information to stderr
 *
 * @param program_name Name of the program (usually argv[0])
 */
void print_usage(const char *program_name);

/**
 * Initializes script_options with default values
 *
 * Call this before parse_arguments() to ensure all fields have valid defaults.
 *
 * @param options Options structure to initialize (must be non-NULL)
 */
void init_options(script_options *options);

/* ========================================================================== */
/* FUNCTION DECLARATIONS - PTY MANAGEMENT                                     */
/* ========================================================================== */

/**
 * Creates and opens a pseudo-terminal (PTY) master
 *
 * Uses the POSIX PTY interface:
 * 1. posix_openpt(O_RDWR | O_NOCTTY) - opens PTY master
 * 2. grantpt() - changes slave ownership to calling user
 * 3. unlockpt() - unlocks the slave so it can be opened
 *
 * O_NOCTTY: Don't make this our controlling terminal
 *
 * @return File descriptor of PTY master on success, -1 on error
 *
 * Ownership: Caller must close returned file descriptor
 *
 * Side effects: Prints error message to stderr on failure
 */
int create_pty_master(void);

/**
 * Gets the pathname of the PTY slave device
 *
 * @param master_fd File descriptor of PTY master
 * @return Slave device path (e.g., "/dev/pts/0") on success, NULL on error
 *
 * Ownership: Returned pointer is to static storage, do NOT free
 *
 * Side effects: Prints error message to stderr on failure
 */
const char *get_pty_slave_name(int master_fd);

/* ========================================================================== */
/* FUNCTION DECLARATIONS - PROCESS MANAGEMENT                                 */
/* ========================================================================== */

/**
 * Forks and sets up child process to run in PTY slave
 *
 * Child process:
 * 1. Creates new session with setsid() - becomes session leader
 * 2. Opens PTY slave - becomes controlling terminal
 * 3. Redirects stdin/stdout/stderr to PTY slave
 * 4. Closes master_fd (not needed in child)
 * 5. Executes shell or command
 *
 * Parent process:
 * 1. Closes slave_fd (not needed in parent)
 * 2. Returns child PID
 *
 * @param master_fd PTY master file descriptor
 * @param slave_name Path to PTY slave device (e.g., "/dev/pts/0")
 * @param options Script options (determines what command to run)
 *
 * @return Child PID in parent, does not return in child (calls execve)
 *         Returns -1 on fork failure
 *
 * Precondition: master_fd is valid PTY master, slave_name is non-NULL
 *
 * Side effects:
 * - Child: Replaces process image with shell (does not return)
 * - Parent: Returns normally
 * - Prints error messages to stderr on failure
 */
pid_t fork_child(int master_fd, const char *slave_name,
                 const script_options *options);

/*
 * execute_shell - removed; logic inlined into fork_child.
 * int execute_shell(const script_options *options);
 */

/* ========================================================================== */
/* FUNCTION DECLARATIONS - TERMINAL CONTROL                                   */
/* ========================================================================== */

/**
 * Puts terminal into raw mode for character-by-character I/O
 *
 * Raw mode disables:
 * - Line buffering (ICANON)
 * - Echo (ECHO)
 * - Signal generation (ISIG)
 * - CR/NL translation (ICRNL, ONLCR)
 * - Flow control (IXON)
 *
 * This allows immediate character forwarding without waiting for newline.
 *
 * @param state Script state (stores original termios for restoration)
 *
 * @return 0 on success, -1 on error
 *
 * Precondition: stdin is a terminal (checked with isatty)
 * Postcondition: state->original_termios contains saved attributes
 *                state->terminal_modified is set to 1
 *
 * Side effects: Modifies stdin terminal attributes
 */
int setup_terminal_raw_mode(script_state *state);

/**
 * Restores terminal to original state (before raw mode)
 *
 * Should be called before program exit to avoid leaving terminal
 * in an unusable state.
 *
 * @param state Script state (contains original termios)
 *
 * Precondition: setup_terminal_raw_mode() was called successfully
 */
void restore_terminal_mode(const script_state *state);

/**
 * Copies window size from stdin to PTY master
 *
 * This ensures the child process has the correct terminal dimensions.
 * Uses TIOCGWINSZ to get stdin's window size and TIOCSWINSZ to set
 * the PTY's window size.
 *
 * @param master_fd PTY master file descriptor
 *
 * @return 0 on success, -1 on error
 *
 * Side effects: Sets PTY window size via ioctl
 */
int copy_window_size(int master_fd);

/* ========================================================================== */
/* FUNCTION DECLARATIONS - I/O LOOP                                           */
/* ========================================================================== */

/**
 * Main I/O loop that copies data between terminal and PTY
 *
 * This is the heart of the script command. It uses select() to multiplex
 * between stdin and the PTY master:
 *
 * 1. stdin -> PTY master: User input goes to child (shell)
 * 2. PTY master -> stdout: Shell output goes to user
 * 3. PTY master -> output_fd: Shell output also logged to file
 *
 * The loop continues until:
 * - Child process exits (detected by read returning 0 or SIGCHLD)
 * - Error occurs on stdin or master_fd
 *
 * @param state Script state (contains master_fd, output_fd, child_pid)
 * @param options Script options (determines flush behavior)
 *
 * @return 0 on success, -1 on error
 *
 * Precondition: PTY master is open, output file is open, child is running
 * Postcondition: All I/O is complete, child has exited
 *
 * Side effects:
 * - Reads from stdin and master_fd
 * - Writes to stdout, master_fd, and output_fd
 * - May call fflush() if flush_mode is enabled
 */
int io_loop(script_state *state, const script_options *options);

/* ========================================================================== */
/* FUNCTION DECLARATIONS - FILE OPERATIONS                                    */
/* ========================================================================== */

/**
 * Opens the typescript output file
 *
 * @param filename Output filename (or NULL for default)
 * @param append_mode 0 to truncate, 1 to append
 *
 * @return File descriptor on success, -1 on error
 *
 * Ownership: Caller must close returned file descriptor
 *
 * Side effects: Creates or truncates file, prints error on failure
 */
int open_output_file(const char *filename, int append_mode);

/**
 * Writes start message to output file and stdout
 *
 * Format: "Script started on <timestamp>\n"
 *
 * @param output_fd Output file descriptor
 * @param quiet 0 to print to stdout, 1 to suppress
 *
 * Side effects: Writes to output_fd and possibly stdout
 */
void write_start_message(int output_fd, int quiet);

/**
 * Writes done message to output file and stdout
 *
 * Format: "Script done on <timestamp>\n"
 *
 * @param output_fd Output file descriptor
 * @param quiet 0 to print to stdout, 1 to suppress
 *
 * Side effects: Writes to output_fd and possibly stdout
 */
void write_done_message(int output_fd, int quiet);

/* ========================================================================== */
/* FUNCTION DECLARATIONS - SIGNAL HANDLING                                    */
/* ========================================================================== */

/**
 * Sets up signal handlers for the session
 *
 * Handles:
 * - SIGCHLD: Child process state change (exit, stop, continue)
 * - SIGWINCH: Window size change (terminal resize)
 *
 * @param state Script state (needed by handlers)
 *
 * @return 0 on success, -1 on error
 *
 * Side effects: Installs signal handlers
 */
int setup_signal_handlers(script_state *state);

/* ========================================================================== */
/* FUNCTION DECLARATIONS - UTILITY FUNCTIONS                                  */
/* ========================================================================== */

/**
 * Waits for child process and gets exit status
 *
 * Uses waitpid() with WNOHANG to check for child termination without blocking.
 *
 * @param child_pid Process ID of child
 * @param status Output parameter for exit status
 *
 * @return Child PID if exited, 0 if still running, -1 on error
 */
pid_t check_child_status(pid_t child_pid, int *status);

/**
 * Converts child wait status to exit code
 *
 * Follows bash convention:
 * - Normal exit: return child's exit code
 * - Signal termination: return 128 + signal_number
 *
 * @param status Raw status from wait()
 *
 * @return Exit code to return from main()
 */
int get_exit_code(int status);

/**
 * Gets the user's shell from SHELL environment variable
 *
 * @return Shell path, or DEFAULT_SHELL if SHELL is not set
 *
 * Ownership: Returns pointer to environment string, do NOT free
 */
const char *get_user_shell(void);

/**
 * Write all data to file descriptor with EINTR retry
 *
 * Helper function to ensure all data is written, handling partial writes
 * and EINTR interruptions. Used throughout the codebase for reliable writes.
 *
 * @param fd File descriptor to write to
 * @param data Data buffer to write
 * @param len Length of data to write
 * @param error_msg Error message prefix for perror on failure
 * @return 0 on success, -1 on error
 */
int write_all(int fd, const char *data, ssize_t len, const char *error_msg);

#endif /* FT_SCRIPT_H */
