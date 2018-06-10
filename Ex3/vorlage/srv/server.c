#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 9000

void exit_function(int cfd);
void doprocessing (int cfd);
char * shell(char * line, int cfd);
void putFromClient(int cfd, char* fileName);
void getFromClient(int cfd, char* fileName);
char * ls(int cfd);
char * cdFunction(char *dir, int cfd);
void waitFunctionArray(int array[]);
void waitFunction(int pid);
void sig_handler(int signo);
int runProg(char *line, char* args[], int showPid);
int runPiped(char *prog1, char* args1[], int showPid1, char *prog2, char* args2[], int showPid2);

static void die(const char* msg)
{
    fputs(msg, stderr);
    putc('\n', stderr);
    exit(-1);
}

int main()
{
    
    struct sockaddr_in srv_addr, cli_addr;
    socklen_t sad_sz = sizeof(struct sockaddr_in);
    int sfd, cfd, pid;
    
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(9000);
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("Couldn't open the socket");
    
    if (bind(sfd, (struct sockaddr*) &srv_addr, sad_sz) < 0)
        die("Couldn't bind socket");
    
    if (listen(sfd, 10) < 0)
        die("Couldn't listen to the socket");
    
    while (1) {
        cfd = accept(sfd, (struct sockaddr*) &cli_addr, &sad_sz);
        if (cfd < 0)
            die("Couldn't accept incoming connection");
    
        pid = fork();
        
        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }
        
        if (pid == 0) {
            close(sfd);
            doprocessing(cfd);
            exit(0);
        }
        else {
            close(cfd);
        }
    }
    
    close(cfd);
    close(sfd);
    return 0;
}

void doprocessing (int cfd) {
    ssize_t bytes;
    char buf[256];
    
        while ((bytes = read(cfd, buf, sizeof(buf))) != 0)
        {
    
            if (bytes < 0){
                die("Couldn't receive message");
            }
    
            int c = 0;
            char * substring = malloc(bytes + 1);
            while (c < bytes) {
                substring[c] = buf[c];
                c++;
            }
            substring[c] = '\0';
    
            char * returnFromShell = shell(substring, cfd);
    
            if (write(cfd, returnFromShell, strlen(returnFromShell)) < 0){
                die("Couldn't send message");
            }
        }
}

void exit_function(int cfd){   // disconnect client
    if (write(cfd, "connection closed\n", strlen("connection closed\n")) < 0){
        die("Couldn't send message\n");
    }
    close(cfd);
}


//Shell functions

char * shell(char * line, int cfd)
{
    char *directory = "."; // home directory when starting
    
        if (strcmp("exit",line) == 0) { // exit shell when "exit" is called
            exit_function(cfd);
        }else if(strcmp("ls",line) == 0){ // list all items in current directory
            ls(cfd);
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
                                char *errorMessage = "\nerror: after pipe no correct program execution call\n";
                                line[strlen(errorMessage)-1] = 0;

                                if (write(cfd, errorMessage, strlen(errorMessage)) < 0)
                                    die("Couldn't send message");
                                
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
                char *dir = cdFunction(destination, cfd);
                if (dir != NULL) {
                    directory = malloc(strlen(dir));
                    strcpy(directory, dir);
                }
            }else{
                return("cd: argument missing");
            }
        }else if(strncmp("put ",line , 4) == 0){
            char delimiter[] = " ";
            line = strtok(line, delimiter);
            line = strtok(NULL, delimiter);
            putFromClient(cfd, line);
        }else if(strncmp("get ",line , 4) == 0){
            char delimiter[] = " ";
            line = strtok(line, delimiter);
            line = strtok(NULL, delimiter);
            getFromClient(cfd, line);
        }
        else {
            char *errorMessage = malloc(strlen(": command not found\n")+strlen(line)+1);
            strcpy(errorMessage, line);
            strcat(errorMessage, ": command not found\n");
            
            if (write(cfd, errorMessage, strlen(errorMessage)) < 0)
                die("Couldn't send message");
        }

    char *s2 = "/> ";
    char *result = malloc(strlen(directory)+strlen(s2)+1);
    strcpy(result, directory);
    strcat(result, s2);
    return result;
}

char * cdFunction(char *dir, int cfd){
    DIR *d;
    struct stat attribut;
    
    stat(dir, &attribut);
    if(attribut.st_mode & S_IFREG){     // check if requested path is not actually a file
        char *errorMessage = malloc(strlen("bash: cd: ")+strlen(dir)+strlen(": Not a directory\n")+1);
        strcpy(errorMessage, "bash: cd: ");
        strcat(errorMessage, dir);
        strcat(errorMessage, ": Not a directory\n");
        
        if (write(cfd, errorMessage, strlen(errorMessage)) < 0)
            die("Couldn't send message");

        return NULL;
    }else{
        d = opendir(dir);
        if (d == NULL) {    // requested directory does not exist
            
            char *errorMessage = malloc(strlen("bash: cd: ")+strlen(dir)+strlen(": No such file or directory\n")+1);
            strcpy(errorMessage, "bash: cd: ");
            strcat(errorMessage, dir);
            strcat(errorMessage, ": No such file or directory\n");
            
            if (write(cfd, errorMessage, strlen(errorMessage)) < 0)
                die("Couldn't send message");
            
            return NULL;
        }else{
            int value = chdir(dir); // change into requested directory
            if(value == -1){
                char *errorMessage = malloc(strlen("error while changing directory\n")+1);
                strcpy(errorMessage, "error while changing directory\n");

                if (write(cfd, errorMessage, strlen(errorMessage)) < 0)
                    die("Couldn't send message");
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

void putFromClient(int cfd, char* fileName){
    FILE *fp;
    fp = fopen(fileName, "ab");
    int bytesReceived = 0;
    char recvBuff[256];
    if(NULL == fp)
    {
        printf("Error opening file");
    }
    
    // Receive data in chunks of 256 bytes
    while((bytesReceived = read(cfd, recvBuff, 256)) > 0)
    {
        printf("Bytes received %d\n",bytesReceived);
        // recvBuff[n] = 0;
        fwrite(recvBuff, 1,bytesReceived,fp);
        // printf("%s \n", recvBuff);
    }
    
    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }
}

void getFromClient(int cfd, char* fileName){
    FILE *fp = fopen(fileName,"rb");
    if(fp==NULL)
    {
        printf("File opern error");
    }
    
    while(1)
    {
        //First read file in chunks of 256 bytes
        unsigned char buff[256]={0};
        int nread = fread(buff,1,256,fp);
        printf("Bytes read %d \n", nread);
        
        // If read was success, send data.
        if(nread > 0)
        {
            printf("Sending \n");
            if (write(cfd, buff, nread) < 0){
                die("Couldn't send message");
            }
        }
        
        if (nread < 256)
        {
            if (feof(fp))
                printf("End of file\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }
    }
}

void sig_handler(int signo) // signal handler if "ctrl+c" is pressed while waiting for children
{
    if (signo == SIGINT){
        printf("\nreceived Signal\n");
        printf("abort\n");
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
        return EXIT_SUCCESS;
    }else{
        printf("program not found\n");
        return -1;
    }
    
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

char * ls(int cfd){  // directory listing function (to check what folders ar awailable for cd)
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    char * dirItemName = "\n";
    if (d)
    {
        
        while ((dir = readdir(d)) != NULL)
        {
            dirItemName = malloc(strlen(dir->d_name) + strlen(dirItemName) + 2);
            strcpy(dirItemName, dir->d_name);
            strcat(dirItemName, "\n");
            if (write(cfd, dirItemName, strlen(dirItemName)) < 0){
                die("Couldn't send message");
            }
        }
        closedir(d);
    }
    return dirItemName;
}

