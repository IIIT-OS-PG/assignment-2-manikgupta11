/*
    C socket server example, handles multiple clients using threads
    Compile
    gcc server.c -lpthread -o server
*/
 

#include <bits/stdc++.h> 

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
char peer_list[2000];

struct peerInfo{
    string ip;
    string port;
    vector<string> files;
};
vector<peerInfo> peers;

void get_list_from_tracker(){
/* first get list from tracker */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in tracker_addr;
  tracker_addr.sin_family = AF_INET;
  tracker_addr.sin_port = htons(8888);
  tracker_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  connect(sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr));
  
  recv(sockfd , peer_list , 2000 , 0);
  peer_list[strlen(peer_list)] = '\0';
  cout << peer_list << endl;
  close(sockfd);
  return;
}

void parse_peer_list(){
    vector <string> tokens; 
    // stringstream class check1 
    stringstream check1(peer_list);
    string intermediate; 
    // Tokenizing w.r.t. space ' ' 
    while(getline(check1, intermediate, '\n')) 
    { 
        tokens.push_back(intermediate); 
    }

    vector<string> tmp;
    for(auto peers : tokens){
        vector <string> tokens_tmp; 
        stringstream check(peers);
        while(getline(check,intermediate,',')){
            tokens_tmp.push_back(intermediate);
        }
        peerInfo p;
        p.ip = tokens_tmp[0];
        p.port = tokens_tmp[1];
        for(int i = 2 ; i < tokens_tmp.size() ; i++){
            p.files.push_back(tokens_tmp[i]);
        }
        peers.push_back(p);
    }
}

void *peer_handler(void *);

int main(int argc , char *argv[])
{
    get_list_from_tracker();
    parse_peer_list();



  return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *peer_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;

    //Send some messages to the client
    char message[] = "Hi Peer !!! Here is the current list of Peer and their files : \n";
    write(sock , message , strlen(message));
    FILE *fp = fopen("peer_list.txt","rb");
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
