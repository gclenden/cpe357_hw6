#ifndef PARSELINE_H
#define PARSELINE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#define LINE_LEN_LIMIT 512
#define PIPE_CMD_LIMIT 15
#define CMD_ARG_LIMIT 15

typedef struct stageHeader stage;
typedef struct lineData line;

int fillLine(line *line);
int fillStage(char *token, line *myLine);
void printStages(line *line);
int replace_white_space(char *token);
int handleRedirects(char **args, stage *myStage, int stageNum);
int handleArgs(char **args, stage *myStage, int stageNum);
int handleOutputRedir(int *i, int stageNum, int *outRedir, char **args, stage *myStage);
int handleInputRedir(int *i, int stageNum, int *inRedir, char **args, stage *myStage);
void freeStages(line *myLine);

struct stageHeader
{
	char line[LINE_LEN_LIMIT+1];
	/*'s'=stdin/stdout -- 'p'=pipe -- 'f'=file redirect*/
	uint8_t inputFlag;
	uint8_t outputFlag;
	char input[257];
	char output[257];
	int argc;
	char *argv[CMD_ARG_LIMIT+1];
};

struct lineData
{
        char line[LINE_LEN_LIMIT+1];
        int stageCount;
        stage stages[PIPE_CMD_LIMIT];
};


#endif
