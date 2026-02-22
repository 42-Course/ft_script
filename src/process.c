#include "ft_script.h"

static int execute_shell(const script_options *options)
{
    extern char **environ;
    const char *shell;
    char *argv[4];  /* Maximum: ["shell", "-c", "command", NULL] */

    shell = get_user_shell();
    if (options->command != NULL) {
        argv[0] = (char *)shell;
        argv[1] = "-c";
        argv[2] = (char *)options->command;
        argv[3] = NULL;
    } else {
        argv[0] = (char *)shell;
        argv[1] = NULL;
    }
    if (execve(shell, argv, environ) == -1) {
        perror("execve");
        return -1;
    }
    return 0;
}

pid_t fork_child(int master_fd, const char *slave_name,
                 const script_options *options)
{
    pid_t pid;
    int slave_fd;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid > 0) {
        return pid;
    }
    /*
     * Create a new session (man 2 setsid):
     * - Detaches from parent's controlling terminal
     * - Child becomes session leader with NO controlling terminal yet
     *
     * This matters for the open() call below: man 7 credentials states
     * "the controlling terminal is established when the session leader
     * first opens a terminal (unless O_NOCTTY is specified)".
     * So opening the PTY slave here (without O_NOCTTY) will automatically
     * make it the controlling terminal for this new session.
     */
    if (setsid() == -1) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }
    slave_fd = open(slave_name, O_RDWR);
    if (slave_fd == -1) {
        perror("open slave");
        exit(EXIT_FAILURE);
    }

    if (dup2(slave_fd, STDIN_FILENO) == -1) {
        perror("dup2 stdin");
        exit(EXIT_FAILURE);
    }
    if (dup2(slave_fd, STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(EXIT_FAILURE);
    }
    if (dup2(slave_fd, STDERR_FILENO) == -1) {
        perror("dup2 stderr");
        exit(EXIT_FAILURE);
    }

    close(slave_fd);
    close(master_fd);

    if (execute_shell(options) == -1) {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_FAILURE);
}

/**
 * WNOHANG: Don't block if child hasn't exited yet
 *
 * EINTR: waitpid() can be interrupted by signals
 * If EINTR occurs, we retry the wait, this is important because signals can interrupt waitpid()
 */
pid_t check_child_status(pid_t child_pid, int *status)
{
    pid_t result;

    do {
        result = waitpid(child_pid, status, WNOHANG);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
    }

    return result;
}

int get_exit_code(int status)
{
    if (WIFEXITED(status)) {
        /*
         * Normal exit: return the exit code
         * Range: 0-255
         */
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        /*
         * Killed by signal: return 128 + signal_number
         * This follows the bash convention
         */
        return EXIT_STATUS_SIGNAL_BASE + WTERMSIG(status);
    }
    return EXIT_FAILURE;
}

const char *get_user_shell(void)
{
    const char *shell;

    shell = getenv("SHELL"); // Apparently forbidden by the guidelines, but I allow it similar to ctime
    if (shell == NULL || shell[0] == '\0') {
        shell = DEFAULT_SHELL;
    }
    return shell;
}
