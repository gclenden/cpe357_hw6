#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "parseline.h"
/*
int main(void)
{
	line myLine;

	if(fillLine(&myLine)<0)
		return -1;

	printStages(&myLine);
	
	freeStages(&myLine);

	return 0;
}
*/
int fillLine(line *myLine, FILE *file)
{
	char *token=NULL;
	char *toke_ptr=NULL;
	int cmdcount=0;
	int fd;

	memset(myLine, 0, sizeof(line));

	/*might want to just check if the file is a stdin to see if it should be print the promt*/
	if((fd=fileno(file))>0 && isatty(fd) && isatty(STDOUT))
		printf("8=p ");
	errno=0;
	/*read in the command line string*/
	if(fgets(myLine->line, sizeof(myLine->line)-1, file)==NULL)
	{
		if(errno!=0)
		{
			if(errno==EINTR)
				return -1;

			else
			{
				perror("read cmdline");
				return -1;
			}
		}	
		
		/*found eof -- time to stop the mush*/
		else if(feof(file))	
		{
			if(file==stdin)
				printf("\n");

			return 1;
		}

		fprintf(stderr, "I don't know what happened\n");
		return -1;
	}

		
	/*check to see if the line was too long*/
	if(myLine->line[LINE_LEN_LIMIT])
	{
		fprintf(stderr, "command too long\n");
		return -1;
	}
	/*
	   ptr=myLine->line;
	   */	myLine->stageCount=0;
	token=strtok_r(myLine->line, "|", &toke_ptr);
	while(token)
	{
		if(++cmdcount>PIPE_CMD_LIMIT)
		{
			fprintf(stderr, "Too many arguments\n");
			return -1;
		}

		if(fillStage(token, myLine)<0)
			return -1;

		myLine->stageCount++;
		token=strtok_r(NULL, "|", &toke_ptr);
	}

	return 0;
}


int fillStage(char *token, line *myLine)
{
	stage *myStage=NULL;
	stage *lastStage=NULL;
	char *toke_ptr=NULL;
	char *args[CMD_ARG_LIMIT];
	int i;

	myStage=myLine->stages+myLine->stageCount;
	lastStage=myLine->stages+myLine->stageCount-1;
	/*if(strcpy(myStage->line, token)==NULL)
		return -1;
	*/
	if(replace_white_space(token)<0)
		return -1;

	if(strcpy(myStage->line, token)==NULL)
                return -1;

	memset(args, 0, sizeof(args));
	i=0;
	args[0]=strtok_r(token, " ", &toke_ptr);
	while(args[i])
	{
		args[++i]=strtok_r(NULL, " ", &toke_ptr);	
		if(i>CMD_ARG_LIMIT && args[i])
		{
			fprintf(stderr, "%s: too many arguments\n", args[0]);
			return -1;
		}
	}

	if(args[0]==NULL)
	{
		fprintf(stderr, "invalid null command\n");
		return -1;
	}

	myStage->argc=0;
	myStage->outputFlag='s';
	if(myLine->stageCount)
	{
		myStage->inputFlag='p';
		if(lastStage->outputFlag=='f')
		{		
			fprintf(stderr, "%s: ambiguous output\n", lastStage->argv[0]);
			return -1;
		}

		lastStage->outputFlag='p';
	}

	else
		myStage->inputFlag='s';

	if(handleRedirects(args, myStage, myLine->stageCount)<0)
		return -1;

	if(handleArgs(args, myStage, myLine->stageCount)<0)
		return -1;

	return 0;
}

void printStages(line *myLine)
{
	int i, j;
	stage *myStage=NULL;

	if(myLine==NULL)
		return;
	for(i=0; i<myLine->stageCount; i++)
	{
		myStage=myLine->stages+i;
		printf("\n--------\n");
		printf("Stage %i: \"%s\"\n", i, myStage->line);
		printf("--------\n");

		printf("%12s", "input: ");
		switch (myStage->inputFlag)
		{
			case 's':
				printf("original stdin\n");
				break;
			case 'p':
				printf("pipe from stage %i\n", i-1);
				break;
			case 'f':
				printf("%s\n", myStage->input);
				break;
			default:
				printf("\ninput: what kind of flag was that\n");
				/*return;*/
				continue;
		}

		printf("%12s", "output: ");
		switch (myStage->outputFlag)
		{
			case 's':
				printf("original stdout\n");
				break;
			case 'p':
				printf("pipe to stage %i\n", i+1);
				break;
			case 'f':
				printf("%s\n", myStage->output);
				break;
			default:
				printf("\noutput: what kind of flag was that\n");
				return;
		}

		printf("%12s%i\n", "argc: ", myStage->argc);
		printf("%12s", "argv: ");
		for(j=0; j<myStage->argc; j++)
		{
			if(j)
				printf(",");

			printf("\"%s\"", myStage->argv[j]);
		}

		printf("\n");
	}	
}

int handleRedirects(char **args, stage *myStage, int stageNum)
{
	int inRedir = 0;
	int outRedir = 0;
	int i = 0;
	if(!args || !myStage)
		return -1;

	for(i=0; i<CMD_ARG_LIMIT; i++)
	{
		if(args[i]!=NULL)
		{	 	
		/*	if(args[i][0]=='>')
			{
				fprintf(stderr, "%s: bad output redirection\n", myStage->argv[0]);
				return -1;
			}
		*/
			if(handleInputRedir(&i, stageNum, &inRedir, args, myStage)<0)
				return -1;

			if(args[i]==NULL)
				continue;

			else if(handleOutputRedir(&i, stageNum, &outRedir, args, myStage)<0)
				printf("output dir failed\n");

			/*else if(addArg(&i, stageNum, &outRedir, args, myStage)<0)
                                printf("add arg failed\n");
			*/
		}
	}

	return 0;
}					

int handleInputRedir(int *i, int stageNum, int *inRedir, char **args, stage *myStage)
{
	if(args[*i][0]=='<')
	{
		if(stageNum>0)
		{
			fprintf(stderr, "%s: ambiguous input\n", args[0]);
			return -1;
		}

		/*check to see if there is a redirection already*/
		if(*inRedir)
		{
			fprintf(stderr, "%s: bad input redirection\n", args[0]);
			return -1;      
		}

		else if(args[*i+1]==NULL || args[*i+1][0] == '<' || args[*i+1][0] == '>')             
		{
			fprintf(stderr, "%s: bad input redirection\n", args[0]);
			return -1;
		}

		*inRedir=1;
		if(strcpy(myStage->input, args[*i+1])==NULL)
			return -1;

		myStage->inputFlag='f';
		args[*i]=NULL;
		args[++*i]=NULL;		
	}
	return 0;
}

int handleOutputRedir(int *i, int stageNum, int *outRedir, char **args, stage *myStage)
{
	if(args[*i][0]=='>')
	{
		if(*outRedir)
		{
			fprintf(stderr, "%s: bad output redirection\n", args[0]);
			return -1;
		}

		else if(args[*i+1]==NULL || args[*i+1][0] == '<' || args[*i+1][0] == '>')
                {
                        fprintf(stderr, "%s: bad output redirection\n", args[0]);
                        return -1;
                }

		*outRedir=1;
		
		if(strcpy(myStage->output, args[*i+1])==NULL)
			return -1;                        

		myStage->outputFlag='f';
		args[*i]=NULL;
		args[++*i]=NULL;
	}
		
	return 0;
}

/*
int addAgs 
{	else if(args[*i]!=NULL)
	{
		if((myStage->argv[myStage->argc]=malloc(strlen(args[*i])+1))==NULL)
			return -1;

		if(strcpy(myStage->argv[myStage->argc], args[*i])==NULL)
			return -1;
	}

	return 0;
}*/

int handleArgs(char **args, stage *myStage, int stageNum)
{
	int i;
	for(i=0; i<CMD_ARG_LIMIT; i++)
	{
		if(args[i]==NULL)
			continue;
	
		myStage->argc++;

		if((myStage->argv[myStage->argc-1]=malloc(strlen(args[i])+1))==NULL)
			return -1;

		if(strcpy(myStage->argv[myStage->argc-1], args[i])==NULL)
			return -1;
	}

	return 0;
}

void freeStages(line *myLine)
{
	int i, j;
	stage *myStage=NULL;
	
	for(i=0; i<PIPE_CMD_LIMIT; i++)
	{
		myStage=myLine->stages+i;
		for(j=0; j<CMD_ARG_LIMIT; j++)
			if(myStage->argv[j])
				free(myStage->argv[j]);
	}

	return;
}

int replace_white_space(char *token)
{
	int i=0;
	while(token[i]!=0)
	{
		if(isspace(token[i]))
			token[i]=' ';
		i++;
	}

	return 0;
}
