//pid_t
#include <fcntl.h>
//NULL
#include <stddef.h>
//wait
#include <sys/wait.h>

#include <errno.h>

#include <signal.h>

#include <unistd.h>

//shared memory
#include <sys/shm.h>

#include <stdlib.h>

#define MAX_BG 32

#define TRUE 1
#define FALSE 0

typedef struct bg_proc {
    pid_t pid;
    struct bg_proc *next;
} Bg_proc;

extern Bg_proc* bg_proc_head;

extern pid_t shell_pid;

void lsh_execute(Command *cmd_list);

void signal_set();

void EOF_handler(Bg_proc *bg_proc);

