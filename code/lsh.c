/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file(s)
 * you will need to modify the CMakeLists.txt to compile
 * your additional file(s).
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Using assert statements in your code is a great way to catch errors early and make debugging easier.
 * Think of them as mini self-checks that ensure your program behaves as expected.
 * By setting up these guardrails, you're creating a more robust and maintainable solution.
 * So go ahead, sprinkle some asserts in your code; they're your friends in disguise!
 *
 * All the best!
 */
#include <assert.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The <unistd.h> header is your gateway to the OS's process management facilities.
#include <unistd.h>

#include "parse.h"
#include "my.h"

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *string);

// All background processes are placed in a group different from shell
// we denote their group id as 'bg_proc_pgid'
//there are 3 states for this 'bg_proc_pgid':
// 1: val == -1, no background processes occurred
// 2: val != -1, and there exists running background processes
// 3: val != -1, and all bg processes dead.
// we use shared memory here, since after fork, the child process will
// have it's own vitrual memory space, we can't simply use ordinary variables
// to communicate between each other.
int bg_proc_pgid_shmid;

pid_t *init_bg_proc_pgid()
{
  bg_proc_pgid_shmid = shmget(IPC_PRIVATE, sizeof(pid_t), 0777);
  if (bg_proc_pgid_shmid == -1)
  {
    perror("shared memory failed");
  }

  pid_t *bg_proc_pgid = (pid_t *)shmat(bg_proc_pgid_shmid, (void *)0, 0);
  *bg_proc_pgid = -1;
  return bg_proc_pgid;
}

void EOF_handler(pid_t *bg_proc_pgid)
{
  //kill the background proc group
  //if the group was created, then try to kill it.
  //even if the group is empty now(all proc dead)
  //try to kill a non-exsisting group does not generate any error. 
  if (*bg_proc_pgid != -1)
  {
    kill(-*bg_proc_pgid, SIGINT);
  }
  shmdt(bg_proc_pgid);
  shmctl(bg_proc_pgid_shmid, IPC_RMID, NULL);
  exit(0);
}

pid_t shell_pid = -1;

int main(void)
{

  shell_pid = getpid();
  pid_t *bg_proc_pgid = init_bg_proc_pgid();
  signal_set();
  for (;;)
  {
    char *line;
    line = readline("> ");

    // Ctrl-D EOF
    if (line == NULL)
    {
      EOF_handler(bg_proc_pgid);
    }

    // Remove leading and trailing whitespace from the line
    stripwhite(line);

    // If stripped line not blank
    if (*line)
    {
      add_history(line);

      Command cmd;
      if (parse(line, &cmd) == 1)
      {
        // Just prints cmd
        print_cmd(&cmd);
        lsh_execute(&cmd);
      }
      else
      {
        printf("Parse ERROR\n");
      }
    }

    // Clear memory
    free(line);
  }

  return 0;
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inspiration.
 */
static void print_cmd(Command *cmd_list)
{
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd_list->rstdin ? cmd_list->rstdin : "<none>");
  printf("stdout:     %s\n", cmd_list->rstdout ? cmd_list->rstdout : "<none>");
  printf("background: %s\n", cmd_list->background ? "true" : "false");
  printf("Pgms:\n");
  print_pgm(cmd_list->pgm);
  printf("------------------------------\n");
}

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
static void print_pgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    print_pgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/* Strip whitespace from the start and end of a string.
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  size_t i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}
