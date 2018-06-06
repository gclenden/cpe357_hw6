#ifndef MUSH_H
#define MUSH_H

#include "parseline.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define PIPE_WR 1
#define PIPE_RD 0
#define STDIN 0
#define STDOUT 1
#define WAITERR -2

int setupStages(line *myLine, pid_t *ids, struct sigaction *oldSig);
int safeWait(int *exitStatus);
/*int executeStages(line *myLine,);
*/
int dupStage(stage *myStage, int *prevPipe, int *currPipe);
int cleanPipe(int *pipe);
int myCD(stage *myStage);
void SigHandler();


#endif
