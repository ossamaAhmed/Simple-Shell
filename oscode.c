#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define ARGS_MAX_NUM 20


/*
**This function takes a buffer (the arguments buffer) and deletes everything
** inside it and initializes it to null
** char *args[]: the arguments buffer
** returns the last index it freed
*/
int freecmd(char *args[])
{
    int i=0;

    for(i=0; i< ARGS_MAX_NUM ;i++){
        args[i]=NULL;
    }

    return i;
}
/*
**This function asks the user for the command and parses 
**the command into the different args. Then it returns the 
** arguments count.
** char *prompt : the prompt to be displayed to the user
** char *args[] : the arguments buffer to parse the command in it
** int *background : a flag that is set while parsing if an '&' is specified 
** at the end of the command 
** the function returns the number of the arguments entered in the command 
*/
int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line;

    //freeing the pointers for getline to use them as it wants 
    freecmd(args);
    token=NULL;
    line= NULL;
    loc= NULL;

    size_t linecap = 0;
    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);
    if (length == 1 && line[0]=='\n') {
        return(-1);
    }

    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    return i;
}
int runcmd(char *args[], int *background, char line[80]){

    freecmd(args);
    int i=0;
    char *token, *loc;
    token=NULL;
    loc= NULL;
    //printf("the length is %lu the return status was %c here it is\n",strlen(line),line[strlen(line)+1]);
    if(line[strlen(line)+1] =='2'){
        return -1;
    }
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    return i;

}
void cleanHistory(char history[10][80]) {
  int i;
  int j;
  for (i = 0; i < 10; i++) {
    for(j=0;j<80;j++){

        history[i][j] = '\n';
    }
  }
}

//checked 
int searchHistory(char history[10][80], int historyIndex, char* letter) {
  int i;
  int searchIndex=-1;
    for (i = ((historyIndex)%10); i < 10; i++) {
         if (history[i][0] == letter[0]) {
             return i;
        }
    }
    for (i = 0; i < ((historyIndex)%10); i++) {
        if (history[i][0] == letter[0]) {
             return i;
    }
  }
    return searchIndex;
}

//this is checked 
void saveHistory(char history[10][80], int historyIndex, char *args[], int count, int status) {
  int i;
  int j=0;
  int counter=0;
  int newIndex = historyIndex % 10;
  

  for(i=0; i<count;i++){
    for(; j<strlen(args[i]);j++){
        history[newIndex][j]= args[i][counter];
        counter++;
    }
    counter=0;
    j++;
  }
  history[newIndex][j-1]= '\0';
  history[newIndex][j]= status+ '0';
}

//this is checked
void printHistory(char history[10][80], int historyIndex) {
  /* bail if nothing was inserted in history */
  if (historyIndex < 0) {
    return;
  }

  int i;

  int newIndex = historyIndex % 10;

  for (i = newIndex; i >= 0; i--) {

    if (history[i][0] != '\0' && history[i][0] != '\n') {
        printf("%d %s\n",historyIndex, history[i]);
        historyIndex--;
    }
  }

  for (i = 9; i > newIndex; i--) {
    if (history[i][0] != '\0' && history[i][0] != '\n') {
        printf(" %d %s\n",historyIndex, history[i]);
        historyIndex--;
    }
  }
}


void printJobs(pid_t pids[]) {
  int i;
  for (i = 0; i < 10; i++) {
    if (pids[i] != 0) {
      if (kill(pids[i], 0) == 0) {
        printf("[%d]  %d\n", i, pids[i]);
      } else {
        pids[i] = 0;
      }
    }
  }
}

void sigchld_handler (int sig) {
    int status;
    pid_t child;
    while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("this is where we check for zombies processes\n");
    }
}

int main()
{
    char *args[20];
    int bg;
    char history[10][80];
    int historyIndex = -1;
    pid_t pids[10];
    int pidindex = 0;
    pid_t stoppid = -1;

    cleanHistory(history);
    setpgid(getpid(),getpid());
    while(1)
    {
        signal(SIGCHLD, sigchld_handler);
        bg=0;
        int recall=0;
        int cnt = getcmd("\n>>  ", args, &bg);

        //checking for built in commands
        if(cnt==-1){
            continue;
        }
        if(strcmp(args[0], "history") == 0){
            printf("This should print out the history built in command\n");
            historyIndex++;
            saveHistory(history, historyIndex, args, cnt, 1);
            printHistory(history, historyIndex);
            continue;
        }
        else if(strcmp(args[0], "cd") == 0 && cnt == 2){
            printf("this should change directory as a built in function not as execvp\n");
            chdir(args[1]);
            historyIndex++;
            saveHistory(history, historyIndex, args, cnt, 1);
            continue;
        }
        else if(strcmp(args[0], "exit") == 0){
            printf("this should exit a built in function not as execvp\n");
            int i;
            for (i = 0; i < 10; i++) {
                 if (pids[i] != 0) {
                 kill(pids[i], SIGTERM);
                }
            }
            exit(1);
            historyIndex++;
            saveHistory(history, historyIndex, args, cnt, 1);
            continue;
        }
        else if(strcmp(args[0], "pwd") == 0){
            printf("this should pwd the directory\n");
            char str[200];
            char* currentWorkingDirectory;
            currentWorkingDirectory= getcwd(str, 200);
            if (currentWorkingDirectory == NULL && errno == ERANGE){
                printf("buffer size is too small\n");
            }
            printf("str = %s\n", currentWorkingDirectory);
            historyIndex++;
            saveHistory(history, historyIndex, args, cnt, 1);
            continue;
        }
        else if(strcmp(args[0], "fg") == 0 && cnt == 2){

            int jobID = atoi(args[1]);
            pid_t fgpid = pids[jobID % 10];
            printf("bringing %d process to the foreground\n",getpgid(fgpid));
            int error;
            printf("the control of the stdout/ in is handled by %d \n",tcgetpgrp(STDIN_FILENO));
            //going around it
            error= tcsetpgrp(STDIN_FILENO, getpgid(fgpid));
            kill(fgpid, SIGCONT);
            printf("the control of the stdout/ in is handled by after executing the command %d \n",tcgetpgrp(STDIN_FILENO));
            int status;
            wait(&status);

            //returning control to the user
            printf("just returned now my process grp is %d \n", getpgrp());
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp (STDIN_FILENO, getpgrp());
            signal(SIGTTOU, SIG_DFL);


            printf("Parent\n");
            if(error==0){
                 printf("we are in this group process %d \n",getpid());
                printf("IT WORKED\n");
            }
            else{
                printf("It didn't work \n");
            }
            printf("this should do the stuff for the built in command fg\n");
            historyIndex++;
            saveHistory(history, historyIndex, args, cnt, 1);
            continue;
        }
        else if(strcmp(args[0], "jobs") == 0){
            printf("this should print out the jobs\n");
            printJobs(pids);
            historyIndex++;
            saveHistory(history, historyIndex, args, cnt, 1);
            continue;
        }

           /* check for recall */
        if (strcmp(args[0], "r") == 0) {
            int searchIndex= -1;
            printf("this should recall the last command\n");
            if(cnt==1){
                printf("execute the last command\n");
                recall = 1;
                searchIndex = historyIndex % 10;
            }
            else if(cnt==2){
                printf("execute the matched command in history\n");
                recall = 1;
                searchIndex = searchHistory(history, historyIndex, args[1]);
            }
            if(searchIndex!=-1){
                cnt=runcmd(args,&bg,history[searchIndex]);
                if(cnt==-1){
                    printf("this operation is not allowed, you tried it before\n");
                    continue;
                }
            }
            printf("%d\n",searchIndex );
        }

        //execute the command finally 
        printf("execute the command finally \n");
        pid_t childPid= fork(); // fork child
        if(childPid==0){ // child

            printf("the process id is %d\n",getpid() );
            printf("Child forking succeeded\n");
            if(bg!=0){
                printf("setting the process to a new group id number 1\n");
                setpgid(0,0);
            }
            int err= execvp(args[0],args);
            printf("the execution is %d \n",err );
            exit (err);
        }
        else if (childPid<0){ //forking failed

            printf("Child forking failed\n");

        }
        else{  // parent

            
            if(bg==0){
                printf("that's the parent process\n");
                printf("the parent process id is %d\n",getpid() );
                int returnStatus;    
                waitpid(childPid, &returnStatus, WNOHANG);  // Parent process waits here for child to terminate.
                printf("returnStatus is  %d\n",returnStatus);


                 if (returnStatus == 0) { // Verify child process terminated without error.  
                    printf("The child process terminated successfully\n"); 
                    historyIndex++;
                    saveHistory(history, historyIndex, args, cnt,returnStatus);
                 }

                 else  {
                    printf("the child process terminated with an error\n");  
                    historyIndex++;
                    saveHistory(history, historyIndex, args, cnt,2); 
                 }
            

            }
            else{
                //concurrent process 
                  printf("%d %d - background\n", pidindex, childPid);
                  pids[pidindex] = childPid;
                  pidindex = (pidindex + 1) % 10;
            }
           
        }

    }
}







