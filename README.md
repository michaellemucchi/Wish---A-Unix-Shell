# Wish - A Unix Shell
## Overview
Wish (short for Wisconsin Shell) is a simple Unix command-line interpreter (CLI), or shell, that mimics the basic functionality of common Unix shells such as bash. This project was designed to familiarize myself with Linux programming, process creation and management, and the essential functionality of shells.

## Features
- Interactive mode: The shell operates interactively, allowing users to type commands one at a time.
- Batch mode: The shell can read commands from a file and execute them sequentially.
- Built-in commands: Supports the following built-in commands:
  - exit: Terminates the shell.
  - cd <directory>: Changes the current working directory.
  - path <dir1> <dir2> ...: Modifies the search path for finding executables.
- Executable commands: The shell searches for executables in the specified directories and executes them.
- Redirection: Allows the redirection of output to a file using the > operator.
- Parallel execution: Supports parallel execution of multiple commands using the & operator.
- Error handling: Any error encountered is reported through a uniform error message.

## How It Works
The Wish shell repeatedly:

1. Prints a prompt (wish> ) for user input.
2. Reads a command line.
3. Parses the command and its arguments.
4. Executes the command by either invoking a built-in function or creating a child process using fork() and execv().
5. Waits for the command to finish unless it's a parallel command.

- Commands are searched for in the directories specified in the shell's path. The shell starts with /bin/ as the default path.


## Usage
### Interactive Mode
Run the shell by invoking it without arguments:

```./wish```

You will see the prompt wish> , after which you can type commands like:


```wish> ls -la```
```wish> cd /usr/local```
```wish> path /bin /usr/bin```
```wish> ls > output.txt```
```wish> cmd1 & cmd2 & cmd3```
```wish> exit```

### Batch Mode
To run the shell in batch mode, pass a filename containing a list of commands as an argument:


```./wish batch.txt```

The shell will execute each command in the file sequentially without displaying the prompt.

### Built-in Commands
- exit: Terminates the shell.
- cd <directory>: Changes the current working directory. Errors occur if the directory does not exist or if multiple arguments are provided.
- path <dir1> <dir2> ...: Modifies the search path for executables.

### Redirection
Use the > operator to redirect the output of a command to a file:

```wish> ls -la > output.txt```

### Parallel Execution
Use the & operator to execute multiple commands in parallel:

```wish> cmd1 & cmd2 args & cmd3 args```

The shell will run all commands in parallel and wait for them to complete before returning to the prompt.

## Error Handling
If an error occurs (e.g., invalid syntax or failure to execute a command), the shell prints the following error message to stderr:

```An error has occurred```

The shell continues executing unless the error is critical (e.g., too many arguments on startup or failure to open a batch file).

## Compilation
To compile the shell, use the following command:

```g++ -o wish wish.cpp -Wall -Werror```
note: -g to enable debugging with gdb
