#include "ft_script.h"

static int handle_stdin_input(int master_fd);
static int handle_master_output(int master_fd, int output_fd,
                                  const script_options *options);

int io_loop(script_state *state, const script_options *options)
{
    fd_set read_fds;
    int max_fd;
    int select_result;

    max_fd = (STDIN_FILENO > state->master_fd) ? STDIN_FILENO : state->master_fd;
    while (42) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(state->master_fd, &read_fds);
        select_result = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (select_result == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            return -1;
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (handle_stdin_input(state->master_fd) == -1) {
                /*
                 * Error or EOF on stdin
                 * Could indicate user closed input (Ctrl-D)
                 * Continue to drain any remaining PTY output
                 */
            }
        }

        if (FD_ISSET(state->master_fd, &read_fds)) {
            if (handle_master_output(state->master_fd, state->output_fd,
                                      options) == -1) {
                break;
            }
        }

        if (check_child_status(state->child_pid, &state->child_status) > 0) {
            break;
        }
    }

    return 0;
}

/**
 * Reads from stdin and forwards to PTY master
 */
static int handle_stdin_input(int master_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
        if (errno == EINTR) {
            return 0;
        }
        perror("read stdin");
        return -1;
    }

    if (bytes_read == 0) {
        /* EOF on stdin (Ctrl-D) */
        return -1;
    }

    /* Forward to PTY master */
    return write_all(master_fd, buffer, bytes_read, "write master");
}

/**
 * Handles output from PTY master (child's output)
 * Reads from PTY master and writes to both stdout and the typescript file.
 */
static int handle_master_output(int master_fd, int output_fd,
                                 const script_options *options)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    bytes_read = read(master_fd, buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
        if (errno == EINTR) {
            return 0;
        }
        if (errno == EIO) {
            return -1;
        }
        perror("read master");
        return -1;
    }
    if (bytes_read == 0) {
        return -1;
    }
    if (write_all(STDOUT_FILENO, buffer, bytes_read, "write stdout") == -1) {
        return -1;
    }
    if (write_all(output_fd, buffer, bytes_read, "write output file") == -1) {
        return -1;
    }
    if (options->flush_mode) {
        if (fsync(output_fd) == -1) {
            if (errno != EINTR) {
                perror("fsync");
            }
        }
    }

    return 0;
}
