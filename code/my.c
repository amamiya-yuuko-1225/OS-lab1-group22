#include "parse.h"
#include "my.h"
#include <stdio.h>

//handle EOF (ctrl-d)
//exit the shell and kill all background processes
void EOF_handler(Bg_proc *bg_proc)
{
    //iterate all background processes
    while (bg_proc != NULL)
    {
        //exclude invalid pid, such as dummy head
        if (bg_proc->pid > 0)
        {
            if (kill(bg_proc->pid, SIGTERM) < 0) {
                perror("kill bg proc error before exiting shell");
            } 
        }
        bg_proc = bg_proc->next;
    }
    exit(0);
}

// SIGCHILD hanlder for the shell.
// when child process exited,
// shell captures the SIGCHILD signal and
// use 'waitpid' to remove child processes from 
// the process table to avoid zombies
void child_signal_handler()
{
    pid_t pid;
    //iterate all children
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        remove_bg_process(bg_proc_head, pid);
    }
}

// ctrl-c(SIGINT) handler for the shell.
// the shell just have to ignore it.
// Other foreground processes(cmds) just behave as default, i.e., 
// being killed by SIGINT.
// The background processes(cmds) should ignore it. This is set 
// when a background process is forked. See the code below.
void int_signal_handler()
{
    printf("\n>");
}

// set all signal handlers for shell
void signal_set()
{
    signal(SIGCHLD, child_signal_handler);
    signal(SIGINT, int_signal_handler);
    return;
}

// inbuilt command 'cd'
void cd(char *arg)
{
    if (arg == NULL)
    {
        perror("Insufficient arguments\n");
    }
    else
    {
        int cond;
        //just use the chdir system call
        cond = chdir(arg);
        if (cond == -1)
        {
            perror("Wrong path\n");
        }
    }
}

// check if a cmd is inbuilt and run it(except for 'exit');
// if it's inbuilt, execute it and return 1;
// if it's not inbuilt, return 0

int execute_inbuilt(char **pgmlist)
{
    //check if 'cd'
    if (strcmp(pgmlist[0], "cd") == 0) 
    {
        cd(pgmlist[1]);
        return 1;
    }
    return 0;
}

//To set i/o redirections
void io_redirection(Command *command_list)
{
    if (command_list->rstdout != NULL)
    {
        int out = open(command_list->rstdout, O_WRONLY | O_CREAT, 0777);
        dup2(out, STDOUT_FILENO);
        close(out);
    }
    if (command_list->rstdin != NULL)
    {
        int in = open(command_list->rstdin, O_RDONLY);
        dup2(in, STDIN_FILENO);
        close(in);
    }
}

// Add background processes to the list
// Bg_proc *bg_proc: dummy head of the bg_proc list 
// pid: pid to be added
void add_bg_process(Bg_proc *bg_proc, pid_t pid)
{
    while (bg_proc->next != NULL)
    {
        bg_proc = bg_proc->next;
    }
    Bg_proc *newone = (Bg_proc *)malloc(sizeof(Bg_proc));
    newone -> next = NULL;
    newone -> pid = pid;
    bg_proc->next = newone;
}

//Remove specified process from the list
void remove_bg_process(Bg_proc *bg_proc, pid_t pid)
{
    Bg_proc *p = bg_proc;
    while (p->next != NULL && p->next->pid != pid)
    {
        p = p->next;
    }
    if (p->next == NULL)
    {
        return;
    }
    Bg_proc *target = p->next;
    p->next = target->next;
    free(target);
}

//Execute programs in a recursive fasion.
//Pgm *current: the current program to be executed.
//int root: If root == TURE, the function is called by shell.
void pgm_execute(Command *command_list, Pgm *current, int root)
{
    //exit of recursion
    if (current == NULL)
    {
        return;
    }

    //creating pipes used to communicate with 
    //preceding programs (child processes) in a pipeline fasion.
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Pipe error:");
    }

    //fork a new process to execute the Pgm *current
    pid_t pid = fork();

    if (pid == 0)
    {
        // child process here
        // if running in background, ignore SIGINT
        if (command_list->background)
        {
            signal(SIGINT, SIG_IGN);
        }

        if (!root)
        {
            // If not the last program, redirect child's output to pipe's write end
            // The pipe is shared with the preceding program (parent process).
            dup2(pipe_fd[1], STDOUT_FILENO);
            // close the read end of the pipe
            close(pipe_fd[0]);
        }
        else
        {
            // Set i/o redirection for the command immediately after the fork of shell
            io_redirection(command_list);
        }

        // execute preceding programs recursively.
        pgm_execute(command_list, current->next, FALSE);

        // execute inbuilt or ordinary program
        if (!execute_inbuilt(current->pgmlist))
        {
            // if not inbuilt, use execvp to run it.
            if (execvp(current->pgmlist[0], current->pgmlist) == -1)
            {
                // execution error
                perror(current->pgmlist[0]);
                exit(0);
            }
        }
    }
    else if (pid > 0)
    {
        // parent process here.
        if (!root)
        {
            // if not the shell, redirect parent's input to pipe's read end.
            dup2(pipe_fd[0], STDIN_FILENO);
            // close write end.
            close(pipe_fd[1]);
        }
        if (command_list->background)
        {
            // if child is running in background, add it to the bg_proc list.
            add_bg_process(bg_proc_head, pid);
        }
        else
        {
            // if child is running foreground, the parent should wait for it's exit.
            int status;
            if (waitpid(pid, &status, 0) != pid) {
                perror("Wait child process failed:");
            }
        }
    }
    else
    {
        perror("Fork failed:");
    }
}

//The entrance when start to execute a command.
void cmd_execute(Command *cmd)
{
    // inbuilt cmd 'exit' is specially treated here
    // since we should call exit(0) at shell process.
    if (strcmp(cmd->pgm->pgmlist[0], "exit") == 0) // exit shell
    {
        puts("Exiting lsh···");
        exit(0);
    }
    //Execute programs recursivly.
    pgm_execute(cmd, cmd->pgm, TRUE);
}
