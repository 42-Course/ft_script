#include "ft_script.h"

static int *g_master_fd = NULL;

static void handle_sigwinch(int signum);

int setup_signal_handlers(script_state *state)
{
    g_master_fd = &(state->master_fd);

    if (signal(SIGWINCH, handle_sigwinch) == SIG_ERR) {
        perror("signal SIGWINCH");
        return -1;
    }
    return 0;
}

static void handle_sigwinch(int signum)
{
    (void)signum;

    if (g_master_fd != NULL) {
        copy_window_size(*g_master_fd);
    }
}
