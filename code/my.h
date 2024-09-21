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

#define MAX_BG 32

#define TRUE 1
#define FALSE 0

extern int bg_proc_pgid_shmid;

extern pid_t shell_pid;

void lsh_execute(Command *cmd_list);

void signal_set();