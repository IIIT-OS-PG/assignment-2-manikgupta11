/*
    C socket server example, handles multiple clients using threads
    Compile
    gcc server.c -lpthread -o server
*/
 
#include <iostream>

#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
 
using namespace std;
//the thread function

void *connection_handler(void *);
 
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in seeder , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    seeder.sin_family = AF_INET;
    seeder.sin_addr.s_addr = inet_addr("127.0.0.5");
    seeder.sin_port = htons( 9898 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&seeder , sizeof(seeder)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;

    char file_name[127];
    recv(sock , file_name , 126 , 0);
    file_name[strlen(file_name)] = '\0';

    //Send some messages to the client
    //char message[] = "Hi Peer !!! Here is the current list of Peer and their files : \n";
    //write(sock , message , strlen(message));
    
    FILE *fp = fopen(file_name,"rb");
    char buffer[2048];
    int n;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);

    while((n = fread(buffer, sizeof(char), 2048, fp)) > 0 && size > 0) {
         send(sock, buffer, n, 0);
         memset(buffer, '\0', 2048);
         size = size - n;
    }

    printf("File send complete by seeder_1 \n");
    if(n == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(n == -1)
    {
        perror("recv failed");
    }
         
    return 0;
} 
