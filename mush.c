#include "mush.h"

static int cdReceived = 0;
static int numChild = 0;


int main(int argc, char **argv)
{
	pid_t ppid = getpid();
	line myLine;
	pid_t ids[PIPE_CMD_LIMIT];
	int i, exitStatus;
	FILE *input=NULL;

	struct sigaction oldSig;
	struct sigaction Sig;
	Sig.sa_handler = SigHandler;
	Sig.sa_flags = 0;
	sigfillset(&(Sig.sa_mask));
	sigdelset(&(Sig.sa_mask), SIGINT); 

	if (sigaction(SIGINT, &Sig, &oldSig) <0)
	{
		perror("sigaction error");
		return -1;
	}

	input = stdin;
	if(argc==2)
	{
		if((input=fopen(argv[1], "r"))==NULL)
		{
			perror(argv[1]);
			return 0;
		}
	}

	/*while I haven't read in an eof*/
	while((i=fillLine(&myLine, input))!=1)
	{
		/*check to see if there was an error reading in*/
		if(i<0)
			continue;

		if(setupStages(&myLine, ids, &oldSig)!=0)
			return 0;

		if(cdReceived)
		{
			//printf("\n\ncdReceived\n\n");
			if(safeWait(NULL)!=WAITERR)
				myCD(myLine.stages);
			cdReceived = 0;
		}

		else
			//printf("numChild: %i\n", numChild);
			while(numChild)
			{
			//		printf("waiting for a child\n");
				if(safeWait(&exitStatus)<0)
					break;

				fflush(NULL);
			//		printf("child exited: %i\n", exitStatus);
				numChild--;
			}

		fflush(NULL);
		fflush(stdin);
		/*
		if(getpid()==ppid)
			printf("parent is done for this line\n");
		else
			printf("\ti am a child; how the heck did i get here\n");
		
		fflush(NULL);
		*/
		/*
		   if(executeStages(&myLine, pipes)!=0)
		   return 0;
		   */
		freeStages(&myLine);
	}

	return 0;
}

int safeWait(int *exitStatus)
{
	if(wait(exitStatus)<0)//&errno!=ECHILD)
	{
		if(errno!=ECHILD && errno!=EINTR)
		{
			perror("safeWait");
			return WAITERR;
		}

		printf("ECHILD OR EINTR FOUND\n");

		return -1;
	}

	return 0;
}

int setupStages(line *myLine, pid_t *ids, struct sigaction *oldSig)
{
	stage *myStage=NULL;
	int i;
	int prevPipe[2], currPipe[2];

	memset(prevPipe, 0, sizeof(prevPipe));
	memset(currPipe, 0, sizeof(currPipe));

	/*check to see if cd is the first arguement before setting upstages*/
	if(strcmp("cd", myLine->stages[0].argv[0])==0)
        {
        	cdReceived=1;
		return 0;
	}

	for(i=0; i<myLine->stageCount; i++)
	{
		if(pipe(currPipe)<0)
		{
			perror("pipping currPipe");
			return -1;
		}

		/*child*/
		if((ids[i]=fork())==0)
		{
			if(sigaction(SIGINT, oldSig, NULL)==-1)
			{
				perror("child sigaction");
				return -1;
			}

			//numChild++;
			myStage=myLine->stages+i;

			if(strcmp("cd", myStage->argv[0])==0)
			{
				fprintf(stderr, "MUSH: I am not able to handle the provided cd expression\n");
				cdReceived=1;
			}

			if(!cdReceived && dupStage(myStage, prevPipe, currPipe)<0) 
				return -1;

			if(cleanPipe(prevPipe)<0)
				return -1;

			if(cleanPipe(currPipe)<0)
				return -1;

			if(cdReceived)
				raise(SIGKILL);

			/*exec*/
			if(!cdReceived && execvp(myStage->argv[0], myStage->argv)<0)
			{
				/*perror(myStage->argv[0]);
				*/
				perror("execvp");	
				return -1;
			}

			return 0;
		}

		/*parent*/
		else if(ids[i]>0)				
		{
			numChild++;
			if(cleanPipe(prevPipe)<0)
				return -1;

			prevPipe[0]=currPipe[0];
			prevPipe[1]=currPipe[1];
		}
		/*the fork failed*/
		else
		{
			perror("fork");
			return -1;
		}

		/*parent should wait for all of the kids and then move on to the next call*/			
	}
	return 0;
}

int dupStage(stage *myStage, int *prevPipe, int *currPipe)
{
	int file;

	if(myStage->inputFlag=='p')
	{
		if(dup2(prevPipe[PIPE_RD], STDIN)<0)
		{
			perror("Dup2 pipe in");
			return -1;
		}
	}

	else if(myStage->inputFlag=='f')
	{
		if((file=open(myStage->input, O_RDONLY))<0)	
		{
			perror(myStage->input);
			return -1;
		}

		if(dup2(file, STDIN)<0)
		{
			perror("dup2 file in");
			return -1;
		}	
	}

	if(myStage->outputFlag=='p')
	{
		if(dup2(currPipe[PIPE_WR], STDOUT)<0)
		{
			perror("Dup2 pipe out");
			return -1;
		}    
	}

	else if(myStage->outputFlag=='f')
	{
		if((file=open(myStage->output, O_WRONLY | O_CREAT | O_TRUNC, 0666))<0)
		{
			perror(myStage->output);         
			return -1;
		}

		if(dup2(file, STDOUT)<0)
		{
			perror("dup2 file out");
			return -1;
		}
	}

	return 0;
}

int cleanPipe(int *pipe)
{
	if(pipe[0] && close(pipe[0])<0)
	{
		perror("closing pipe[0]");
		return -1;
	}

	if(pipe[1] && close(pipe[1])<0)
	{
		perror("closing pipe[1]");
		return -1;
	}

	return 0;
}

int executeStages(line *myLine, int *prevPipe, int *currPipe){return 0;}



int myCD(stage *myStage)
{
	char *dir = myStage->argv[1];
	if (chdir(dir) < 0) 
	{
		perror(dir);
		return -1;
	}
	return 0;
}

void SigHandler(int signal)
{	
	if (signal == SIGINT) 
	{
		while (numChild > 0) 
		{
			//	printf("\n\nI am waiting for a child\n\n");
			//	fflush(stdout);
			if(safeWait(NULL)<0)
				break;
			numChild--;
		}
		printf("\n");
		fflush(NULL);
	}
}	
