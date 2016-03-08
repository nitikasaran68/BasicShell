#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


static char typed[1024];
static char* args[512];
static char **all;
char hist[100][1024];
int count=0;
pid_t parent;

void signal_handler(int num){
    signal(SIGINT, signal_handler);
    if(getpid()!=parent){
    	printf("killed\n");
    	exit(0);
 	}
    printf("\n$enter command> ");
	fflush(NULL);
}

void help(){
	printf("This is my implementation of a basic shell\n");
	printf("It supports all basic commands of a unix shell\n");
	printf("It also supports help and history commands\n");
	printf("Piped commands are a little moody,\nfor example \"ls|grep .txt\" works,\nbut \"cat output.txt| grep nitika\" doesn't show the last line of expected output\n");
	printf("Redirection to/from files is also supported\n");
	printf("Enter 'exit' to exit\n");
}

char* skipwhite(char* s)
{
	while (isspace(*s)) ++s;
	return s;
}

void split(char* cmd)
{
	cmd = skipwhite(cmd);
	char* next = strchr(cmd, ' ');
	int i = 0;
 
	while(next != NULL) {
		next[0] = '\0';
		args[i] = cmd;
		++i;
		cmd = skipwhite(next + 1);
		next = strchr(cmd, ' ');
	}
 
	if (cmd[0] != '\0') {
		args[i] = cmd;
		next = strchr(cmd, '\n');
		if(next!=NULL) next[0] = '\0';
		++i; 
	}
 
	args[i] = NULL;
}

int command(int input){

	int pipes[2];
	pipe(pipes);

	pid_t pid = fork();

	if (pid == 0) {

		dup2(input, 0);
		dup2(pipes[1], 1);
		close(pipes[0]);
		if (execvp( args[0], args) == -1){
			printf("Could not process command\n");
			//perror("Error");
			_exit(EXIT_FAILURE); // If child fails
		}
		exit(0);
	}

	else{
		wait(NULL);
	}

	close(input);
	close(pipes[1]);
	return pipes[0];
}

void run(int n,int ifd,int ofd){	
	int current=0;
	split(all[current++]);
	if (args[0] != NULL && n>0) {

		if (strcmp(args[0], "exit") == 0) 
			exit(0);
		int pipes[2];
		pipe(pipes);
		int in;
		pid_t pid = fork();

		if (pid == 0) {
			if(ifd!=-1){
				dup2(ifd,0);
			}
			dup2(pipes[1],1);
			close(pipes[0]);
			if (execvp( args[0], args) == -1){
				printf("Could not process command\n");
				//perror("Error");
				_exit(EXIT_FAILURE); // If child fails
			}
			close(pipes[1]);
			if(ifd!=-1) close(ifd);
			exit(0);
		}

		else wait(NULL);

		in=pipes[0];
		for (int i = 1; i < n-1; ++i){
			split(all[current++]);
			in=command(in);
		}
		split(all[current]);
		pid_t pid2=fork();
		if(pid2==0){
			dup2(in,0);
			if(ofd!=-1){
				dup2(ofd,1);
			}
			if (execvp( args[0], args) == -1){
				printf("Could not process command\n");
			//	perror("Error");
				_exit(EXIT_FAILURE); // If child fails
			}
			if(ofd!=-1) close(ofd);
			close(in);
			exit(0);
		}
		wait(NULL);
	}
}

void run1(int ifd,int ofd){	
	split(all[0]);
//	printf("run1 %s\n",args[0]);
	if (args[0] != NULL) {

		if (strcmp(args[0], "exit") == 0) 
			exit(0);
		if (strcmp(args[0], "history") == 0) {
			for (int i = 0; i < count; ++i)
			{
				printf("%d) %s",i+1, hist[i]);
			}
			return;
		}
		if (strcmp(args[0], "help") == 0) {
			help();
			return;
		}
		pid_t pid = fork();

		if (pid == 0) {
			// child
			if(ifd!=-1){
				dup2(ifd,0);
			}
			if(ofd!=-1){
				dup2(ofd,1);
			}
			if (execvp( args[0], args) == -1){
				printf("Could not process command\n");
				//perror("Error");
				_exit(EXIT_FAILURE); // If child fails
			}
			if(ifd!=-1) close(ifd);
			if(ofd!=-1) close(ofd);
			exit(0);
		}

		else wait(NULL);

	}
}

char* func(char * s){
	char *tr,*ans;
	ans=(char*)malloc(sizeof(s));
	strcpy(ans,s);
	ans=skipwhite(ans);
	tr=ans;
	while(*tr!='\0'){
		if(*tr=='\n' || *tr==' '){
			*tr='\0';
			break;
		}
		tr++;
	}
	return ans;
}

int main(){

	signal(SIGINT, signal_handler);
	printf("Nitika's Basic Shell: Type 'exit' to exit.\n");
	char *p,*start;
	all=(char**)malloc(sizeof(char*)*10);
	// for (int i = 0; i < 10; ++i)
 // 			hist[i]=NULL;
	parent = getpid();
	int num;
	while (1) {
		/* Print the command prompt */
		printf("$enter command> ");
		fflush(NULL);
 		for (int i = 0; i < 10; ++i)
 		{
 			all[i]=NULL;
 		}
 		int ifd=-1,ofd=-1;
		/* Read a command line */
		if (!fgets(typed, 1024, stdin))  return 0;
		if(typed[0]=='\n')
			continue;
		strcpy(hist[count],typed);
		count++;
		p = typed;
		start=p;
		num=0;
		while(*p!='\0'){
			if(*p=='|'){
				*p='\0';
				all[num] = start;
				start=p+1;
				num++;
			}
			if(*p=='>'){
				char*tt=func(p+1);
				ofd=open(tt,O_WRONLY);
				if (ofd!=-1) printf("%s opened for output\n",tt );
				else printf("Error opening file\n");
				*p='\0';
				all[num] = start;
				break;
			}
			if(*p=='<'){
				*p='\0';
				start = func(start);
				ifd=open(start,O_RDONLY);
				if(ifd!=-1)printf("%s opened for input\n",start);
				else printf("Error opening file\n");
				start=p+1;
			}
			p++;
		}
		all[num]=start;
		num++;
	//	printf("%s\n",all[0] );
		if(num==1) run1(ifd,ofd);
		else run(num,ifd,ofd);
		
		// for (int i = 0; i < num; ++i)
 	// 	{
 	// 		free(all[i]);
 	// 	}
	}
	return 0;
}