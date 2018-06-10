#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#define PORT 9000
#define HOST "127.0.0.1"

void putFuction(char * fileName, int cfd);
void getFuction(char * fileName, int cfd);

static void die(const char* msg)
{
    fputs(msg, stderr);
    putc('\n', stderr);
    exit(-1);
}

int main()
{
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(9000),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };
    char buf[256];
    int cfd;
    ssize_t bytes;
    
    if ((cfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("Couldn't open the socket");
    
    if (connect(cfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
        die("Couldn't connect to socket");
    
    char *line = NULL;  // forces getline to allocate with malloc
    size_t len = 0;     // ignored when line = NULL
    ssize_t readline;
    
    while ((readline = getline(&line, &len, stdin)) != -1) {

        line[strlen(line)-1] = 0;
        
        if (write(cfd, line, strlen(line)) < 0)
            die("Couldn't send message");
        
        if ((bytes = read(cfd, buf, sizeof(buf))) < 0)
            die("Couldn't receive message");
        
        if (strncmp("put ",line , 4 ) == 0) {
            char delimiter[] = " ";
            line = strtok(line, delimiter);
            line = strtok(NULL, delimiter);
            putFuction(line,cfd);
        }
            
        if (strncmp("get ",line , 4 )) {
            char delimiter[] = " ";
            line = strtok(line, delimiter);
            line = strtok(NULL, delimiter);
            getFuction(line,cfd);
        }
        
        int c = 0;
        char * substring = malloc(bytes + 1);
        while (c < bytes) {
            substring[c] = buf[c];
            c++;
        }
        substring[c] = '\0';
        
        printf("[recv] %s", substring);
        if(strncmp("connection closed",substring , strlen("connection closed")) == 0){
            break;
        }
    }
    
    close(cfd);
    return 0;
}
            
void putFuction(char * fileName, int cfd){
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
            
void getFuction(char * fileName, int cfd){
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
