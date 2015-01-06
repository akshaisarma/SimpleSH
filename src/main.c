#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

/* Structs */
struct lnode {
	char *cntnt;
	struct lnode *next;
};

/* Globals */
#define ARGLIMIT 5
struct lnode *shpath = NULL;
struct lnode *dirstck = NULL;
char *iline = NULL;
char *cmd = NULL;

/* Functions */
void change_directory();
void path_command();
void pushd_command();
void popd_command();
void dirs_command();
void search_path();
void path_add();
void path_remove();
void malloc_clean_up();
void clean_up();

int main (int argc, const char **argv) {
	int buffind, buffsize;
	buffind = buffsize = 0;
	char c;
	while (1) {
		printf("w4118sh $ ");
		while ((c = getchar()) != '\n') {
			if (buffind == buffsize) {
				buffsize += 128;
				iline = realloc(iline, (size_t)buffsize);
				if (iline == NULL) {
					fprintf(stderr, "Error: Realloc "
						"failed.\n");
					return 1;
				}
			}
			iline[buffind++] = c;
		}
		iline[buffind] = '\0';
		
		cmd = strtok(iline, " ");
		if (!strcmp(cmd, "exit"))
			break;
		else if (!strcmp(cmd, "cd"))
			change_directory();
		else if (!strcmp(cmd, "path"))
			path_command();
		else if (!strcmp(cmd, "pushd"))
			pushd_command();
		else if (!strcmp(cmd, "popd"))
			popd_command();
		else if (!strcmp(cmd, "dirs"))
			dirs_command();
		else 
			search_path();
		
		iline[0] = '\0';
		buffind = 0;
	}
	clean_up();
	return 0;
}

void change_directory()
{
	cmd = strtok(NULL, " ");
	if (chdir(cmd) == -1)
		perror("Error: Change Directory.");
} 

void path_command()
{
	cmd = strtok(NULL, " ");
	if (cmd == NULL) {
		struct lnode* tmp = shpath;
		if (tmp == NULL) {
			printf("Path is empty.\n");	
			return;
		}
		while (tmp->next) {
			printf("%s:", tmp->cntnt);
			tmp = tmp->next;
		}
		printf("%s\n", tmp->cntnt);
	}
	else {
		if (!strcmp(cmd, "+"))
			path_add();		 
		else if (!strcmp(cmd, "-")) 
			path_remove();
		else 
			fprintf(stderr, "Error: Unsupported argument after "
				"path command. Should be + or -.\n");
	}
}

void path_add() 
{
	struct lnode* tmp = shpath;
	cmd = strtok(NULL, " ");
	if (cmd == NULL) {
		fprintf(stderr, "Error: Expecting a path after +.\n");
		return;
	}
	/* Special Case */
	if (shpath == NULL) {
		shpath = malloc(sizeof(struct lnode));
		if (shpath == NULL) 
			malloc_clean_up();
		shpath->next = NULL;
		shpath->cntnt = malloc(strlen(cmd)+1);
		if (shpath->cntnt == NULL) 
			malloc_clean_up();
		strcpy(shpath->cntnt, cmd);
		return;
	}
	while (tmp->next) {
		if (!strcmp(cmd, tmp->cntnt)) {
			fprintf(stderr, "PATH already contains your path."
				" Not adding...\n");
			return;
		}
		tmp = tmp->next;
	}
	struct lnode *new = malloc(sizeof(struct lnode));
	if (new == NULL) 
		malloc_clean_up();
	new->next = NULL;
	new->cntnt = malloc(strlen(cmd)+1);
	if (new->cntnt == NULL) 
		malloc_clean_up();
	strcpy(new->cntnt, cmd);
	tmp->next = new;		
}

void path_remove()
{
	cmd = strtok(NULL, " ");
	if (cmd == NULL) {
		fprintf(stderr, "Error: Expecting a path after -.\n");
		return;
	}
	/* Special Case */
	if (!strcmp(cmd, shpath->cntnt)) {
		struct lnode *tmp = shpath;
		shpath = shpath->next;
		free(tmp);
		return;
	}
	struct lnode* flwr = shpath;
	struct lnode* ldr = shpath->next;
	
	while (ldr) {
		if (!strcmp(cmd, ldr->cntnt)) {
			struct lnode *tmp = ldr;
			flwr->next = ldr->next;
			free (tmp);
			return;
		}
		flwr = ldr;
		ldr = ldr->next;
	}
	fprintf(stderr, "PATH does not contain your path. Cannot remove...\n");
}

void pushd_command()
{
	long pathlen = pathconf(".", _PC_PATH_MAX);
	cmd = strtok(NULL, " ");
	if (cmd == NULL) {
		fprintf(stderr, "Error: Expecting a path.\n");
		return;
	}
	struct lnode *tmp = malloc(sizeof(struct lnode));
	if (tmp == NULL) 
		malloc_clean_up();
	tmp->next = dirstck;
	tmp->cntnt = malloc((size_t)pathlen);
	if (tmp->cntnt == NULL) 
		malloc_clean_up();
	getcwd(tmp->cntnt, pathlen);
	
	if (chdir(cmd) == -1) {
		perror("Error: Pushd Change directory.");
		free (tmp->cntnt);
		free (tmp);
		return;
	}
	tmp->next = dirstck;
	dirstck = tmp;
	dirs_command();
}

void popd_command()
{
	if (dirstck == NULL) {
		printf("Directory stack is empty.\n");
		return;
	}
	struct lnode* tmp = dirstck; 
	dirstck = dirstck->next;
	if (chdir(tmp->cntnt) == -1) {
		perror("Error: Popd Change directory.");
		return;
	}
	free(tmp);
	
	dirs_command();
}

void dirs_command()
{
	long pathlen = pathconf(".", _PC_PATH_MAX);
	char *curpath = malloc((size_t)pathlen);
	if (curpath == NULL) 
		malloc_clean_up();
	getcwd(curpath, pathlen);
	printf("%s ", curpath);
	free (curpath);
	
	struct lnode *tmp = dirstck;
	while (tmp) {
		printf("%s ", tmp->cntnt);
		tmp = tmp->next;
	}
	printf("\n");
}

void search_path()
{
	struct lnode *tmp = shpath;
	struct stat buf;
	char *chkstr = NULL;
	while (tmp) {
		/* Length of path is the len of path string 
		 + 1 for '/' + len of program */
		int pathlen = strlen(tmp->cntnt);
		int pgmlen = strlen (cmd);
		chkstr = realloc(chkstr,1 + pathlen + pgmlen);
		if (chkstr == NULL) {
			fprintf(stderr, "Error: x failed.\n");
			clean_up();
			exit(1);
		}
		strcpy(chkstr, tmp->cntnt);
		chkstr[pathlen] = '/';
		chkstr[pathlen+1] = '\0';
		strcat(chkstr, cmd);
		if (stat(chkstr,&buf) == -1) {
			if (errno != ENOENT){
				perror("Error: Checking file in path "
				       "directories");
				break;
			}
			tmp = tmp->next;
			continue;
		}
		switch (fork()) {
			case 0: {
				int i;
				/* Since ARGLIMIT number of args are accepted,
				 need ARGLIMIT + 2 strings for name of program
				 + 5 args + null terminator. */
				char *prmlst[ARGLIMIT+2];
				prmlst[0] = cmd;
				for (i = 1; i <= ARGLIMIT+1; i++) 
					prmlst[i] = strtok(NULL, " ");
				prmlst[ARGLIMIT+1] = '\0';
				execv(chkstr, prmlst);
				perror("Error: Exec error");
				break;
			}
			case -1:
				perror("Error: Fork ");
				break;
			default:
				if (wait(0) == -1)
					perror("Error: Wait call");
				break;
		}
		break;	
	}
	if (tmp == NULL)
		fprintf(stderr, "The program %s was not found."
			" Check if dir is in path.\n", cmd);
	free (chkstr);
}
void malloc_clean_up()
{
	fprintf(stderr, "Error: Malloc failed.\n");
	clean_up();
	exit(1);	
}

void clean_up() 
{
	free (iline);
	struct lnode *tmp = shpath;
	while (tmp) {
		tmp = tmp->next;
		free(shpath);
		shpath = tmp;
	}
	tmp = dirstck;
	while (tmp) {
		tmp = tmp->next;
		free(dirstck);
		dirstck = tmp;
	}
	printf("Goodbye.\n");
}