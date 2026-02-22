#include "ft_script.h"

/* Implementation of write_all — see declaration in ft_script.h */
int write_all(int fd, const char *data, ssize_t len, const char *error_msg)
{
    ssize_t total_written = 0;

    while (total_written < len) {
        ssize_t bytes_written = write(fd, data + total_written,
                                      len - total_written);
        if (bytes_written == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror(error_msg);
            return -1;
        }
        total_written += bytes_written;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    script_options options;
    script_state state;
    const char *slave_name;
    int exit_code;

    init_options(&options);
    if (parse_arguments(argc, argv, &options) != 0) {
        return EXIT_FAILURE;
    }
    state.output_fd = open_output_file(options.output_file,
                                       options.append_mode);
    if (state.output_fd == -1) {
        return EXIT_FAILURE;
    }
    state.master_fd = create_pty_master();
    if (state.master_fd == -1) {
        close(state.output_fd);
        return EXIT_FAILURE;
    }
    if (copy_window_size(state.master_fd) != 0) {
        /*
         * Not fatal - continue anyway
         * Child will just have default window size
         */
    }
    slave_name = get_pty_slave_name(state.master_fd);
    if (slave_name == NULL) {
        close(state.master_fd);
        close(state.output_fd);
        return EXIT_FAILURE;
    }
    state.child_pid = fork_child(state.master_fd, slave_name, &options);
    if (state.child_pid == -1) {
        close(state.master_fd);
        close(state.output_fd);
        return EXIT_FAILURE;
    }
    /* From here on, we're in the PARENT process only */
    state.terminal_modified = 0;

    /* Write start message BEFORE entering raw mode (proper newline handling) */
    write_start_message(state.output_fd, options.quiet_mode);

    if (setup_terminal_raw_mode(&state) != 0) {
        fprintf(stderr, "Warning: Failed to set terminal raw mode\n");
    }
    if (setup_signal_handlers(&state) != 0) {
        fprintf(stderr, "Warning: Failed to set up signal handlers\n");
    }

    if (io_loop(&state, &options) != 0) {
        fprintf(stderr, "Warning: I/O loop ended with error\n");
    }

    /* Restore terminal BEFORE writing done message (proper newline handling) */
    restore_terminal_mode(&state);

    while (waitpid(state.child_pid, &state.child_status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }

    write_done_message(state.output_fd, options.quiet_mode);
    close(state.master_fd);
    close(state.output_fd);
    if (options.return_exit_status) {
        exit_code = get_exit_code(state.child_status);
    } else {
        exit_code = EXIT_SUCCESS;
    }
    return exit_code;
}
