#include "ft_script.h"

static script_state *g_state = NULL;

static void handle_sigwinch(int signum);

int setup_signal_handlers(script_state *state)
{
    g_state = state;

    if (signal(SIGWINCH, handle_sigwinch) == SIG_ERR) {
        perror("signal SIGWINCH");
        return -1;
    }
    return 0;
}

static void handle_sigwinch(int signum)
{
    (void)signum;

    if (g_state != NULL) {
        copy_window_size(g_state->master_fd);
    }
}
