# ft_script - Educational Implementation of Unix `script` Command

The `script` command makes a typescript of everything displayed on your terminal. This implementation demonstrates:

### Command-Line Options

```
Usage: ft_script [options] [file]

Options:
  -a              Append to output file instead of overwriting
  -c <command>    Execute command instead of interactive shell
  -q              Quiet mode (suppress start/done messages)
  -e              Return exit status of child process
  -f              Flush output after each write (BONUS)
  -h              Display help and exit
```

## Implementation Details

1. **Fork**: Creates child process
2. **Setsid**: Child creates new session (becomes session leader)
3. **Open slave**: PTY slave becomes controlling terminal
4. **Redirect I/O**: stdin/stdout/stderr → PTY slave
5. **Exec shell**: Replace process image with shell

### I/O Multiplexing

Uses `select()` to monitor multiple file descriptors:

- **stdin** → Read user input → Write to PTY master
- **PTY master** → Read child output → Write to stdout and file

<details>
<summary>WHAT IS A PTY?</summary>
A pseudo-terminal is a pair of character devices:
- Master: Controlled by the application (our ft_script)
- Slave: Used by the child process (the shell)
Notes: man 2 open, man 4 tty
cat /proc/sys/kernel/pty/nr (to see number of opened PTYs)
</details>

<details>
<summary>WHY RAW MODE??</summary>
For script to work correctly, we need to pass every character
immediately to the child without processing. If we stayed in cooked
mode, line editing would happen twice (once in our terminal, once
in the child's terminal), which would be confusing.
c_iflag (input modes):
- ICRNL: translate carriage return to newline on input
- IXON:  enable XON/XOFF flow control (Ctrl-S/Ctrl-Q)
c_oflag (output modes):
- OPOST: enable implementation-defined output processing
         (includes ONLCR: NL-to-CRNL translation)
c_lflag (local modes):
- ECHO:   echo input characters back to terminal
- ICANON: canonical (line-buffered) mode — read waits for newline
- IEXTEN: implementation-defined input processing (e.g. Ctrl-V)
- ISIG:   generate signals on INTR/QUIT/SUSP (Ctrl-C, Ctrl-Z...)
c_cc (special characters, noncanonical mode):
- VMIN = 1:  read() returns as soon as >= 1 byte is available
- VTIME = 0: no timeout — read() blocks until data arrives
  (the four MIN/TIME combinations are explained in man 3 termios
   under "Noncanonical mode")
(all flags documented in man 3 termios)
</details>

## References

    Data written to the slave is presented on the master file descriptor as input.  Data written to the master is presented to the slave as input. - from: man 7 pts

- `man 4 pts` - Pseudoterminal master and slave (ptmx, pts)
- `man 7 pty` - Pseudoterminal Interfaces
- `man 2 setsid` - Creates a session and sets the process group ID
- `man 1 script` - Script command manual
- `man 2 syscalls` - Linux system calls
- `man 3 posix_openpt` - PTY creation
- `man 2 select` - I/O multiplexing
- `man 3 termios` - Terminal attributes
- `man 7 credentials` - Process Identifiers
