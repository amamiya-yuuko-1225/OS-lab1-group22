#include "parse.h"
#include "my.h"
#include <stdio.h>

void EOF_handler(Bg_proc *bg_proc)
{
    while (bg_proc != NULL)
    {
        if (bg_proc->pid > 0)
        {
            kill(bg_proc->pid, SIGTERM);
        }
        bg_proc = bg_proc->next;
    }
    exit(0);
}

// parent calls 'waitpid' when child is killed to avoid zombies
void child_signal_handler()
{
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        remove_bg_process(bg_proc_head, pid);
    }
}

// ctrl-c handler for the shell
void int_signal_handler()
{
    printf("\n>");
}

// set all signal handler
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
        printf("insufficient arguments\n");
    }
    else
    {
        int cond;
        cond = chdir(arg);
        if (cond == -1)
        {
            printf("wrong path\n");
        }
    }
}

// check if a cmd is inbuilt (except for 'exit');
// if yes, execute it and return 1; else return 0
int execute_inbuilt(char **pgmlist)
{

    if (strcmp(pgmlist[0], "cd") == 0) //
    {
        cd(pgmlist[1]);
        return 1;
    }
    return 0;
}

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

void cmd_execute(Command *command_list, Pgm *current, int root)
{
    // if running other foreground processes, the shell should ignore SIGINT
    if (current == NULL)
    {
        return;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        // child
        // if background, ignore SIGINT
        if (command_list->background)
        {
            signal(SIGINT, SIG_IGN);
        }

        if (!root)
        {
            // output redirect
            // if not the last cmd, redirect to pipe sharing with the prior(parent) cmd
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[0]);
        }
        else
        {
            // redirection for the last cmd
            io_redirection(command_list);
        }

        // execute former cmds recursively.
        cmd_execute(command_list, current->next, FALSE);

        // execute inbuilt or ordinary program
        if (!execute_inbuilt(current->pgmlist))
        {
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
        // parent
        // input redirection
        if (!root)
        {
            // if not the shell, redirect to pipe
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[1]);
        }
        if (command_list->background)
        {
            add_bg_process(bg_proc_head, pid);
        }
        else
        {
            // if running foreground
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else
    {
        perror("fork failed");
    }
}

void lsh_execute(Command *cmd)
{
    // inbuilt cmd 'exit' is specially treatet
    // since exit(0) does not work properly in child process
    if (strcmp(cmd->pgm->pgmlist[0], "exit") == 0) // exit shell
    {
        puts("exiting lsh···");
        exit(0);
    }
    cmd_execute(cmd, cmd->pgm, TRUE);
    // after executing cmd
}
