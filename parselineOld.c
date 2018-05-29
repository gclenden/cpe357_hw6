#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "parseline.h"

int main(void)
{
	line myLine;
	
	if(fillLine(&myLine)<0)
		return -1;

	printStages(&myLine);

	return 0;
}

int fillLine(line *myLine)
{
	char *token=NULL;
	char *toke_ptr=NULL;

	memset(myLine, 0, sizeof(line));
	printf("line: ");
	/*read in the command line string*/
	if(fgets(myLine->line, sizeof(myLine->line)+1, stdin)==NULL)
	{
		perror("read cmdline");
		return -1;
	}

/*	printf("got line: %s\n", myLine->line);
*/	
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
		if(fillStage(token, myLine)<0)
			return -1;

		token=strtok_r(NULL, "|", &toke_ptr);
	}
		
	return 0;
}


int fillStage(char *token, line *myLine)
{
	stage *myStage=NULL;
	char *toke_ptr=NULL;
	int index;
	char *arg=NULL;
	char buffer[257];

	/*printf("token: %s\n", token);
	*/

 	printf("filling Stage: %s\n", token);

	index=myLine->stageCount-1;

	if(++myLine->stageCount>PIPE_CMD_LIMIT)
	{
		fprintf(stderr, "pipeline too deep\n");
		return -1;
	}
	
	myStage=myLine->stages+index;

	myStage->outputFlag='s';
	if(strcpy(myStage->line, token)==NULL)
	{
		perror("stage line");
		return -1;
	}
	
	if(index==0)
		myStage->inputFlag='s';
	else
	{
		/*this isn't the first stage so there must be an earlier pipe*/
		myStage->inputFlag='p';
		/*check if the previous stage has an output file, if not, change its output to a pipe*/
		if(myLine->stages[index-1].outputFlag=='s')
			myLine->stages[index-1].outputFlag='p';
	}
	
	if(replace_white_space(token)<0)
		return -1;
	
	arg=strtok_r(token, " ", &toke_ptr);
	if(!arg)
	{
		fprintf(stderr, "invalid null command\n");
		return -1;
	}

	while(arg)
	{
		printf("arg: %s\n", arg);
/*		if(++myStage->argc>CMD_ARG_LIMIT)
		{
			fprintf(stderr, "%s: too many arguments\n", myStage->argv[0]);
			return -1;
		}
*/
		/*output redirect without a file: bad redirection*/
		if(*arg=='>')
		{
			fprintf(stderr, "%s: bad output redirection\n", myStage->argv[0]);
			return -1;
		}

		/*input redirect*/
		if(*arg=='<')
		{
			printf("input redirect\n");

			if(myStage->argc==0)
			{
				fprintf(stderr, "invalid null command\n");
				return -1;
			}

			switch(myStage->inputFlag)
			{
				printf("input flag switch\n");
				/*was expecting stdin, redirect works*/
				case 's':
					myStage->inputFlag='f';
					/*next arg should be the file name or it is a bad input*/
					if((arg=strtok_r(NULL, " ", &toke_ptr))==NULL || *arg=='>' || *arg=='<')
					{
						fprintf(stderr, "%s: bad input redirection\n", 
							myStage->argv[0]);
						return -1;
					}
				
					if(strcpy(myStage->input, arg)==NULL)
					{
						perror("redirecting input");
						return -1;
					}
					break;
				/*already redirected input, bad input redir*/	
				case 'f':
					fprintf(stderr, "%s: bad input redirection\n", myStage->argv[0]);
					return -1;
					break;
				/*expecting a pipe input, ambigous input*/
				case 'p':
					fprintf(stderr, "%s: ambiguous input\n", myStage->argv[0]);
					break;
				default:
					fprintf(stderr, "inputflag==%c: what kind of input flag is that?\n", 
						myStage->inputFlag);
					return -1;
			}
			
			arg=strtok_r(NULL, " ", &toke_ptr);
			continue;
		}
		/*check to see if it is an output redirect; proceed accordingly*/
		else	
		{
			memset(buffer, 0, sizeof(buffer));
			if(strcpy(buffer, arg)==NULL)
			{	
				perror("memset buffer");
				return -1;
			}
	
			/*this is an output redirect*/
			if((arg=strtok_r(NULL, " ", &toke_ptr))!=NULL && *arg=='>')
			{
				switch(myStage->outputFlag)
				{
					/*was expecting stdin, redirect works*/
                                	case 's':
                                        	myStage->outputFlag='f';
                                        	/*next arg should be the file name or it is a bad input*/
                                   		
	                                        if(strcpy(myStage->output, buffer)==NULL)
	                                        {
	                                                perror("redirecting input");
	                                                return -1;
	                                        }
	                                        break;
	                                /*already redirected input, bad input redir*/
	                                case 'f':
	                                        fprintf(stderr, "%s: bad output redirection\n", myStage->argv[0]);
	                                        return -1;
	                                        break;
	                                /*expecting a pipe input, ambigous input, should never happen because pipe won't be know until the next stage*/
	                                case 'p':
	                                        fprintf(stderr, "%s: ambiguous output\n", myStage->argv[0])
;
	                                        break;
	                                default:
	                                        fprintf(stderr, "inputflag==%c: what kind of input flag is that?\n",
	                                                myStage->inputFlag);
	                                        return -1;
	
				
				}
					if(strcpy(myStage->output, buffer)==NULL)
					{
						perror("strcpy output file");		
						return -1;
					}

				arg=strtok_r(NULL, " ", &toke_ptr);	
			}

			else if(arg!=NULL)
			{
				myStage->argc++;
				if(myStage->argc > CMD_ARG_LIMIT)
                                {
                                        fprintf(stderr, "%s: too many arguments\n", myStage->argv[0]);
                                        return -1;
                                }

				if((myStage->argv[myStage->argc-1]=(char *)calloc(strlen(arg)+1, 1))==NULL)
				{
					perror("malloc arg");
					return -1;			
				}
		
				if(strcpy(myStage->argv[myStage->argc], arg)==NULL)
				{
					perror("copy arg");
					return -1;
				}
	
				if(++myStage->argc > CMD_ARG_LIMIT)
		        	{
		        	        fprintf(stderr, "%s: too many arguments\n", myStage->argv[0]);
        				return -1;
       				}
			}   
			
		}
	}
	return 0;
}

void printStages(line *myLine)
{
	int i, j;
	stage *myStage=NULL;

	if(myLine==NULL)
		return;
	for(i=0; i<myLine->stageCount+10; i++)
	{
		myStage=myLine->stages+i;
		printf("\n--------\n");
		printf("Stage %i: \"%s\"\n", i, myStage->argv[0]);
		
		printf("input: ");
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

		printf("argc: %i\n", myStage->argc);
		printf("argv: ");
		for(j=0; j<myStage->argc; j++)
		{
			if(j)
				printf(",");
	
			printf("\"%s\"", myStage->argv[j]);
		}

		printf("\n");
	}	
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
