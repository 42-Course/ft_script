#include "ft_script.h"

int setup_terminal_raw_mode(script_state *state)
{
    struct termios raw;

    if (!isatty(STDIN_FILENO)) {
        state->terminal_modified = 0;
        return 0;
    }

    if (tcgetattr(STDIN_FILENO, &state->original_termios) == -1) {
        perror("tcgetattr");
        return -1;
    }

    raw = state->original_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
        perror("tcsetattr");
        return -1;
    }
    state->terminal_modified = 1;
    return 0;
}

void restore_terminal_mode(const script_state *state)
{
    if (state->terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &state->original_termios);
    }
}

int copy_window_size(int master_fd)
{
    struct winsize ws;

    if (!isatty(STDIN_FILENO)) {
        return 0;
    }
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) {
        perror("ioctl TIOCGWINSZ");
        return -1;
    }
    if (ioctl(master_fd, TIOCSWINSZ, &ws) == -1) {
        perror("ioctl TIOCSWINSZ");
        return -1;
    }
    return 0;
}
