# OS LAB 1 GROUP 22

Our group have met all specifications outlined for the lab. The implementations of each required feature are presented as follows.

## 0. Code structure overview

Here is the overall structure of our code (following codes are pseudo-code).

```c
initialize_bg_proc_list(); //To store background processes, used for CTRL-D handler
set_sig_handler_for_shell(); //Set SIGCHILD, SIGINT handlers for shell
for(;;) {
	cmd = readline();
	cmd_execute(cmd); 
}
```

For the `cmd_execute()` function:

```c
handle_exit_cmd();
pgm_execute(cmd, current_pgm, root); // execute pgms in the cmd recursively
//root == TRUE means it is called by the shell
```

The structure of `pgm_execute()`:

```c
pid = fork();
pipe();// create pipe
if (pid == 0) {
	if (cmd -> background) {
		ignore_SIGINT(); // bg proc ingore SIGINT
	}
	if (root) {
		io_redirection(); // redirect io of the cmd in the first child of shell
	} else {
		redirect_output_to_pipe(); // pgms communicate using pipes
	}
	pgm_execute(cmd, next_pgm, FALSE); // execute pgms recusively
	execvp(current_pgm);
} else {
	if (!root) {
		redirect_input_to_pipe(); //pgms communicate using pipes
	}
	if (cmd -> background) {
		add_bg_proc(child);
	} else {
		wait(child);
	}
}
```

## 4 & 5. Piping & I/O Redirection

Here we discuss about the piping and I/O redirection together.

The following picture shows how data flows inside and out command `pgm1 < in | pgm2 > out` when using pipes and I/O redirection.

![Fig 4&5-1](D:\chalmers\operating_systems\labs\OS-lab1-group22\docs\figs\2.jpg)

<center>Fig 4&5-1</center>

As we can see, I/O redirection changes file descriptor `STDIN_FILENO` to file `in` and `STDOUT_FILENO` to file `out` for the whole command. Therefore, we do the I/O redirection in the first child process of shell.

```c
if (pid == 0) {
	if (root) {
		io_redirection();
	} 
}
```

Now let's look into the `io_redirection()` function.

```c
void io_redirection(Command *command_list)
{
    if (command_list->rstdout != NULL)
    {
        // if the last cmd, redirect to given parameter
        int out = open(command_list->rstdout, O_WRONLY | O_CREAT, 0777);
        dup2(out, STDOUT_FILENO);
        close(out);
    }
    if (command_list->rstdin != NULL)
    {
        // if the last cmd, redirect to given parameter
        int in = open(command_list->rstdin, O_RDONLY);
        dup2(in, STDIN_FILENO);
        close(in);
    }
}
```

To redirect output, firstly we call the `open()` system call to get the input file's file descriptor. We specify `O_WRONLY | O_CREAT` flags here, which means we open it in write-only mode, and if the file does not exsist, we generate a empty file. After we get the file descriptor of `rstdout`: `out`, we call `dup2()` to duplicate `out` to `STDOUT_FILENO`, after which the standard output will be redirected to `rstdout`. Finally, we close the `out` file descriptor. Redirecting input can also be done in a similar way.

After discussing about I/O redirection, we are going to explain the piping feature.

Since we execute the programs in a recursive way, the `pgm 2` process will be forked first, and then it will create a pipe, close it's write end, and redirect it's `STDIN_FILENO` to the read end (which overrides the overall I/O redirection). After that `pgm 2` executes `pgm_execute()`, and `fork()` his child `pgm 1`. `pgm 1` process can now redirect it's `STDOUT_FILENO` to the write end of the pipe sharing with `pgm 2`. After that, the `pgm 1` process will call `execvp()` to actually execute the program, and output to the pipe. After `execvp()` execution, the `pgm_execute()` called by `pgm 2` will return, and thus `pgm 2` process can execute it's program which fetches the output of `pgm 1`  from the pipe as its input.

To create a pipe, redirect input/output to pipe's read/write end, and close the unused end, we use the following code.

Creating pipes:

```c
    //creating pipes used to communicate with 
    //preceding programs (child processes) in a pipeline fasion.
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Pipe error:");
    }
```

`pipe()` function receive a `int[2]` to create a pipe. After creation, the file descriptors of read and write end will be respectively stored in the first and second element of the specified integer array, and return 0 if suceeded or -1 if failed.

Redirecting and closing file descriptors are explained previously in `io_redirection()` function.











