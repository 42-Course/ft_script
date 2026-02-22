#include "ft_script.h"

static const char *get_timestamp(void)
{
    time_t now;
    char *timestamp;

    now = time(NULL);
    if (now == (time_t)-1) {
        return "unknown time\n";
    }

    timestamp = ctime(&now);
    if (timestamp == NULL) {
        return "unknown time\n";
    }

    return timestamp;
}

/**
 * "Script <prefix> on <timestamp>" 
 * <prefix> is either "started" or "done"
 */
static int build_message(char *buffer, const char *prefix, const char *timestamp)
{
    int pos = 0;
    const char *script_msg = "Script ";
    const char *on_msg = " on ";
    const char *src;
// replace later with strcpy/strcat
    src = script_msg;
    while (*src != '\0') {
        buffer[pos++] = *src++;
    }

    src = prefix;
    while (*src != '\0') {
        buffer[pos++] = *src++;
    }

    src = on_msg;
    while (*src != '\0') {
        buffer[pos++] = *src++;
    }

    src = timestamp;
    while (*src != '\0') {
        buffer[pos++] = *src++;
    }

    buffer[pos] = '\0';
    return pos;
}

static void write_script_message(int output_fd, int quiet, const char *prefix)
{
    char message[256];
    int message_len;

    message_len = build_message(message, prefix, get_timestamp());

    write_all(output_fd, message, message_len, "write to typescript");

    if (!quiet) {
        write_all(STDOUT_FILENO, message, message_len, "write to stdout");
    }
}

int open_output_file(const char *filename, int append_mode)
{
    int fd;
    int flags;
    const char *actual_filename;

    actual_filename = (filename == NULL || filename[0] == '\0')
                      ? DEFAULT_TYPESCRIPT : filename;

    flags = O_CREAT | O_WRONLY | (append_mode ? O_APPEND : O_TRUNC);

    fd = open(actual_filename, flags, 0644);
    if (fd == -1) {
        perror("open output file");
        return -1;
    }
    return fd;
}

void write_start_message(int output_fd, int quiet)
{
    write_script_message(output_fd, quiet, "started");
}

void write_done_message(int output_fd, int quiet)
{
    write_script_message(output_fd, quiet, "done");
}
