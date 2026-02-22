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
     * Create a new session
     * - Detaches from parent's controlling terminal
     * - Child becomes session leader (no controlling terminal yet)
     * This is required so that when we open the PTY slave, it becomes
     * the controlling terminal for this new session. (see manual for setsid())
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

    /*
     * Redirect stdin/stdout/stderr to the PTY slave
     * dup2(oldfd, newfd) makes newfd a copy of oldfd
     * - fd 0 (stdin) refers to the PTY slave
     * - fd 1 (stdout) refers to the PTY slave
     * - fd 2 (stderr) refers to the PTY slave
     */
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
 * Waits for child process and gets exit status
 *
 * WNOHANG: Don't block if child hasn't exited yet
 * - Returns 0 if child is still running
 * - Returns child PID if child has exited
 * - Returns -1 on error
 *
 * EINTR: waitpid() can be interrupted by signals
 * - If EINTR occurs, we retry the wait
 * - This is a common pattern for system calls that can be interrupted
 */
pid_t check_child_status(pid_t child_pid, int *status)
{
    pid_t result;

    /*
     * Loop to retry on EINTR (interrupted system call)
     * This is important because signals can interrupt waitpid()
     */
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

    shell = getenv("SHELL");
    if (shell == NULL || shell[0] == '\0') {
        shell = DEFAULT_SHELL;
    }
    return shell;
}
