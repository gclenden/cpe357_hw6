#include "mush.h"
 
int main(int argc, char **argv)
{
	line myLine;
	pid_t ids[PIPE_CMD_LIMIT];
	int i, exitStatus;
	FILE *input=NULL;

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
			
		if(setupStages(&myLine, ids)!=0)
			return 0;

		for(i=0; i<myLine.stageCount; i++)
		{
			wait(&exitStatus);
			if(exitStatus==10)
				;/*myCD();*/
		}


		fflush(NULL);
		/*
		   if(executeStages(&myLine, pipes)!=0)
		   return 0;
		   */
		freeStages(&myLine);
	}

	return 0;
}

int setupStages(line *myLine, pid_t *ids)
{
	stage *myStage=NULL;
	int i;
	int prevPipe[2], currPipe[2];

	memset(prevPipe, 0, sizeof(prevPipe));
	memset(currPipe, 0, sizeof(currPipe));

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
			myStage=myLine->stages+i;
			
			if(strcmp("cd", myStage->argv[0])==0)
				exit(10);

			if(dupStage(myStage, prevPipe, currPipe)<0) 
				return -1;
	
			if(cleanPipe(prevPipe)<0)
				return -1;
		
			if(cleanPipe(currPipe)<0)
				return -1;
			
			/*exec*/
			if(execvp(myStage->argv[0], myStage->argv)<0)
			{
				/*perror(myStage->argv[0]);
				*/
				perror("execvp");	
				return -1;
			}
		}

		/*parent*/
			else if(ids[i]>0)				
			{
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
