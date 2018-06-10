#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

void exit_function();
void ls();
void die(const char *msg);
char * cdFunction(char *dir);
void waitFunctionArray(int array[]);
void waitFunction(int pid);
void sig_handler(int signo);
int runProg(char *line, char* args[], int showPid);
int runPiped(char *prog1, char* args1[], int showPid1, char *prog2, char* args2[], int showPid2);

int main(void)
{
    char *line = NULL;  // forces getline to allocate with malloc
    size_t len = 0;     // ignored when line = NULL
    ssize_t read;
    char *directory = "."; // home directory when starting
    printf ("\n%s/> ", directory);
    
    while ((read = getline(&line, &len, stdin)) != -1) {
        
        line[strlen(line)-1] = 0; // remove "Enter" from String
        
        if (strcmp("exit",line) == 0) { // exit shell when "exit" is called
            exit_function();
        }else if(strcmp("ls",line) == 0){ // list all items in current directory
            ls();
        }else if(strncmp("wait",line , 4) == 0){ // call wait function
            char delimiter[] = " ";
            int pidArray[8] = {-1,-1,-1,-1,-1,-1,-1,-1}; // we can assum we have max 8 children
            int j = 0;
            line = strtok(line, delimiter);
            while(line != NULL){    // fill array with pids to wait for
                line = strtok(NULL, delimiter);
                if(line != NULL){
                    int pid = atol(line);       //cast string to int
                    pidArray[j] = pid;
                }
                j++;
            }
            
            waitFunctionArray(pidArray); // actual call of wait function
            
        }else if(strncmp("./",line , 2) == 0 || strncmp("../",line , 3) == 0 ){ // call for programm execution from current and parent directory
            char delimiter[] = " ";
            char* args[4];
            char* args2[4];
            char *ptr = strtok(line, delimiter);    // get first program
            char *ptr2 = "";                        // second program if piped
            int showPid = 0;                        // if "&" is give to show pid
            int showPid2 = 0;
            int piped = 0;                          // if piped
            int j = 0;
            for (int i = 0; line != NULL; i++) {
                if((line != NULL && strcmp("|",line) == 0) || piped == 1){  // check if current part of line is pipe or pipe was already reat
                    if (piped == 1) {
                        if(j == 0){
                            if (strncmp("./",line , 2) == 0 || strncmp("../",line , 3) == 0) {  // check if part of line after pipe is a program call
                                ptr2 = line;
                            }else{
                                printf ("error: after pipe no correct program execution call\n");
                                piped = 0;
                            }
                        }
                        if (strcmp("&",line) == 0) {    // check if "&" wants to see pid for program 2
                            showPid2 = 1;
                        }
                        args2[j] = line;    // arguments of program 2
                        j++;
                    }else{
                        piped = 1;
                    }
                }else if (line != NULL && strcmp("&",line) == 0 && i < 4) { // check if "&" wants to see pid for program 1
                    showPid = 1;
                }else {
                    if (i<4) {
                        args[i] = line;     // arguments of program 1
                    }
                }
                line = strtok(NULL, delimiter); // remove delimiter from line and get next part
            }
            args[3] = NULL;
            args2[3] = NULL;

            if (piped == 1) {
                runPiped(ptr, args, showPid, ptr2, args2, showPid2); // if piped run piped program call
            }else{
                runProg(ptr, args, showPid);                         // else run normal program call
            }
            
            
        }else if(strncmp("cd ",line , 3) == 0){ // call "cd" function
            char delimiter[] = " ";
            line = strtok(line, delimiter);
            line = strtok(NULL, delimiter);
            char destination[1000];
            strcpy(destination, line);
            if(line != NULL){
                char *dir = cdFunction(destination);
                if (dir != NULL) {
                    directory = dir;
                }
            }else{
                printf("cd: argument missing\n");
            }
        }else {
            printf("%s: command not found\n", line);
        }
        printf ("%s/> ", directory);
    }
    
    free (line);
    
    return 0;
}

void sig_handler(int signo) // signal handler if "ctrl+c" is pressed while waiting for children
{
    if (signo == SIGINT){
        printf("\nreceived Signal\n");
        printf("abort\n");
    }
    
}

void ls(){  // directory listing function (to check what folders ar awailable for cd)
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
}

void exit_function(){   // close this shell
    exit(0);
}

int runPiped(char *prog1, char* args1[], int showPid1, char *prog2, char* args2[], int showPid2){   // call when two programs are requested
    printf ("in runPiped\n");
    struct stat attribut1;
    struct stat attribut2;

    stat(prog1, &attribut1);
    stat(prog2, &attribut2);
    
    if((attribut1.st_mode & S_IFREG) && (attribut2.st_mode & S_IFREG)){ // check if programs are existing in directory
        int fd[2];
        if (pipe(fd) < 0)   // create pipe for communication
            die("pipe()");
        
        pid_t proc_id1;
        pid_t proc_id2;
        proc_id1 = fork();  // "clone" shell for child 1
        proc_id2 = fork();  // "clone" shell for child 2
        
        if (proc_id1 < 0)
        {
            fprintf(stderr, "fork error\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        
        if (proc_id2 < 0)
        {
            fprintf(stderr, "fork error\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        
        if (proc_id1 == 0)
        {    /* child process */
            
            close(fd[1]);   // close read for first child
            int ret = dup2(fd[0],0);
            if (ret < 0) perror("dup2");
            
            if (showPid1 == 1) {
                printf("[%d]\n", (int) getpid());
            }
            
            execvp(prog1, args1);   // execute program1
            exit(-1);
        }else{
            close(fd[0]);   // close write for parent
        }
        
        if (proc_id2 == 0)
        {    /* child process */
            
            close(fd[0]);   // close write for first child
            int ret = dup2(fd[1],1);
            if (ret < 0) perror("dup2");
            
            if (showPid2 == 1) {
                printf("[%d]\n", (int) getpid());
            }
            
            execvp(prog2, args2);   // execute program2
            exit(-1);
        }else{
            close(fd[0]);   // close write for parent
        }

        return EXIT_SUCCESS;
    }else{
        printf("one or both programs not found\n");
        return -1;
    }
}

int runProg(char *line, char* args[], int showPid){ // call when only one program is requested
    struct stat attribut;
    
    stat(line, &attribut);
    if(attribut.st_mode & S_IFREG){ // check if program is existing in directory
        pid_t proc_id;
        proc_id = fork();   // "clone" shell for child
        
        if (proc_id < 0)
        {
            fprintf(stderr, "fork error\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        
        if (proc_id == 0)
        {    /* child process */
            if (showPid == 1) {
                printf("[%d]\n", (int) getpid());
            }
            
            execvp(line, args); // execute program
            exit(-1);
        }
        //        printf("\n");
        return EXIT_SUCCESS;
    }else{
        printf("program not found\n");
        return -1;
    }
    
}

void waitFunctionArray(int array[]){    // wait function an array of pids
    for (int i = 0; i < 8; i++) {
        if (array[i] != -1) {
            waitFunction(array[i]);
        }
    }
}

void waitFunction(int pid){
    
    int status = 0;
    if (signal(SIGINT, sig_handler) == SIG_ERR) // signal handler to abort wait function
        printf("\ncan't catch SIGINT\n");
    
    pid_t child_id = waitpid(pid, &status, 0);  // actuall wait call
    
    printf("[%d] TERMINATED\n",child_id);
    printf("[%d] EXIT STATUS: %d\n",child_id, WEXITSTATUS(status));
    
}

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

char * cdFunction(char *dir){
    DIR *d;
    struct stat attribut;
    
    stat(dir, &attribut);
    if(attribut.st_mode & S_IFREG){     // check if requested path is not actually a file
        printf("bash: cd: %s: Not a directory\n", dir);
        return NULL;
    }else{
        d = opendir(dir);
        if (d == NULL) {    // requested directory does not exist
            printf("bash: cd: %s: No such file or directory\n", dir);
            return NULL;
        }else{
            int value = chdir(dir); // change into requested directory
            if(value == -1){
                printf("error while changing directory\n");
            }
            closedir(d);
            if(strncmp("..",dir , 2) == 0){
                char cwd[1024];
                char *path = getcwd(cwd, sizeof(cwd));  // get whole path
                char *folder = NULL;
                char delimiter[] = "/";
                path = strtok(path, delimiter);
                while (path != NULL) {  // remove whole path except for last folder
                    folder = path;
                    path = strtok(NULL, delimiter);
                }
                return folder;  // return current folder the shell is in
            }else{
                return dir;
            }
        }
    }
}


