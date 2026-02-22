/**
 * WHAT IS A PTY?
 * A pseudo-terminal is a pair of character devices:
 * - Master: Controlled by the application (our ft_script)
 * - Slave: Used by the child process (the shell)
 *
 * Notes: man 2 open, man 4 tty
 * cat /proc/sys/kernel/pty/nr (to see number of opened PTYs)
 */

#include "ft_script.h"

/**
 * 1. Call open("/dev/ptmx") to get a master file descriptor
 *    - O_RDWR: Open for reading and writing
 *    - O_NOCTTY: Don't make this our controlling terminal
 *      (We want the PTY slave to be the child's controlling terminal)
 * 2. Call ioctl(TIOCSPTLCK) to unlock the slave
 *    - The slave is initially locked to prevent access
 *    - Must unlock before the child can open it
 */
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

    /*
     * On Linux, TIOCGPTN gets the PTY number
     * We then construct the path as /dev/pts/N
     */
    int pty_num;
    int i, temp, len;

    if (ioctl(master_fd, TIOCGPTN, &pty_num) != 0) {
        perror("ioctl TIOCGPTN");
        return NULL;
    }

    /*
     * todo: use ft_memset or ft_strcpy
     */
    slave_name[0] = '/';
    slave_name[1] = 'd';
    slave_name[2] = 'e';
    slave_name[3] = 'v';
    slave_name[4] = '/';
    slave_name[5] = 'p';
    slave_name[6] = 't';
    slave_name[7] = 's';
    slave_name[8] = '/';

    /*
     * todo: use ft_itoa and strcat
     */
    len = 9;
    if (pty_num == 0) {
        slave_name[len++] = '0';
    } else {
        i = len; /* Remember start position */
        temp = pty_num;
        /* Build digits in reverse order */
        while (temp > 0) {
            slave_name[len++] = '0' + (temp % 10);
            temp /= 10;
        }
        /* Reverse the digits to correct order */
        {
            int start = i;
            int end = len - 1;
            while (start < end) {
                char c = slave_name[start];
                slave_name[start] = slave_name[end];
                slave_name[end] = c;
                start++;
                end--;
            }
        }
    }
    slave_name[len] = '\0';
    return slave_name;
}
