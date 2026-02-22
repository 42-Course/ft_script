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

## References

- `man 1 script` - Script command manual
- `man 2 syscalls` - Linux system calls
- `man 3 posix_openpt` - PTY creation
- `man 2 select` - I/O multiplexing
- `man 3 termios` - Terminal attributes
