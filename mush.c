#include "mush.h"
 
int main(int argc, char **argv)
{
	line myLine;
	pid_t ids[PIPE_CMD_LIMIT];
	int i;

	/*while I haven't read in an eof*/
	while((i=fillLine(&myLine))!=1)
	{
		/*check to see if there was an error reading in*/
		if(i<0)
			continue;
			
		if(setupStages(&myLine, ids)!=0)
			return 0;

		for(i=0; i<myLine.stageCount; i++)
		{
			wait(NULL);
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

	memset(prevPipe, 0, 2);
	memset(currPipe, 0, 2);

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

			if(dupStage(myStage, prevPipe, currPipe)<0) 
				return -1;

			/*

			   if(myStage->inputFlag=='p')
			   {
			   pipein=1;
			   if(dup2(pipes[i-1][0], stdin)<0)
			   {
			   perror("Dup2 pipe in");
			   return -1;
			   }
			   }

			   else if(myStage->inputFlag=='f')
			   {


			   if((pipeout=updateStageOutput(myStage, pipes+i-1, pipes+i))<0)
			   return -1;			

			   pipeout=0;
			   if(myStage->outputFlag=='p')
			   {
			   pipeout=1;
			   if(dup2(pipes[i][1], stdout)<0)
			   {
			   perror("Dup2 pipe out");
			   return -1;
			   }	
			   }
			   */

			if(cleanPipe(prevPipe)<0)
				return -1;

			if(cleanPipe(currPipe)<0)
				return -1;

			/*exec*/
			if(execvp(myStage->argv[0], myStage->argv)<0)
			{
				perror(myStage->argv[0]);
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
			if(dup2(prevPipe[PIPE_RD], STDIN_FILENO)<0)
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

			if(dup2(file, STDIN_FILENO)<0)
			{
				perror("dup2 file in");
				return -1;
			}	
		}

		if(myStage->outputFlag=='p')
		{
			if(dup2(currPipe[PIPE_WR], STDOUT_FILENO)<0)
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

			if(dup2(file, STDOUT_FILENO)<0)
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
