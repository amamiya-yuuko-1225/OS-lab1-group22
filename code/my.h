//pid_t
#include <fcntl.h>
//NULL
#include <stddef.h>
//wait
#include <sys/wait.h>

#include <errno.h>

#include <signal.h>

#include <unistd.h>

#define MAX_BG 32

#define TRUE 1
#define FALSE 0



void lsh_execute(Command *cmd_list);

void signal_set();