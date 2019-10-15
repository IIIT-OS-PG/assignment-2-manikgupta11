#include <iostream>
#include<stdio.h>
#include<string.h>   
#include<stdlib.h>    
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include<pthread.h> 

using namespace std;

void *connection_handler(void *);
string tracker_file;
 
int main(int argc , char *argv[])
{
    tracker_file=argv[1];
    int tracker_no =atoi(argv[2]);
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 5);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    //open new thread for every request
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
 // This will handle connection for each client

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;

    //Send some messages to the client
    //char message[] = "Hi Peer !!! Here is the current list of Peer and their files : \n";
    //write(sock , message , strlen(message));
    FILE *fp = fopen(tracker_file.c_str(),"rb");
    char buffer[2048];
    int n;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    
    //read peer_list intp buffer and send to client
    while((n = fread(buffer, sizeof(char), 2048, fp)) > 0 && size > 0) {
         send(sock, buffer, n, 0);
         memset(buffer, '\0', 2048);
         size = size - n;
    }
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    return 0;
} 
