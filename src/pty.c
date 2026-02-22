#include "ft_script.h"

int create_pty_master(void)
{
    int master_fd;
    int unlock;

    master_fd = open("/dev/ptmx", O_RDWR | O_NOCTTY); // man 7 pty
    if (master_fd == -1) {
        perror("open /dev/ptmx");
        return -1;
    }
    unlock = 0;
    if (ioctl(master_fd, TIOCSPTLCK, &unlock) != 0) { // man 2 ioctl_tty
        perror("ioctl TIOCSPTLCK");
        close(master_fd);
        return -1;
    }
    return master_fd;
}

/**
 * The slave name is a device path like "/dev/pts/0" or "/dev/pts/1".
 * The child process will open this device file to get access to the
 * slave end of the PTY.
 *
 * On modern Linux systems, PTY slaves are in /dev/pts/ (pts = pseudo-
 * terminal slave). Older systems used /dev/ptyXX and /dev/ttyXX.
 */
const char *get_pty_slave_name(int master_fd)
{
    static char slave_name[128];
    int         pty_num;

    /*
     * On Linux, TIOCGPTN gets the PTY index number (man 2 ioctl_tty).
     * We then construct the full path as "/dev/pts/N".
     */
    if (ioctl(master_fd, TIOCGPTN, &pty_num) != 0) {
        perror("ioctl TIOCGPTN");
        return NULL;
    }
    ft_strcpy(slave_name, "/dev/pts/");
    ft_uitoa(slave_name + 9, (unsigned int)pty_num);
    return slave_name;
}
