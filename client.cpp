#include <bits/stdc++.h>
#include<dirent.h>
#include <arpa/inet.h> 
#include <pthread.h>  
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

char peer_list[2000];
vector<string>userlist; vector<string>passwordlist;
bool auth=0;
string path;

struct peerInfo {
  string ip;
  string port;
  string group;
  vector<string> files;
  
};
deque<peerInfo> total_peers;
string file;

/**********************Functions used******************************/
void *leecher_handler(void *);
void *seeder_handler(void *);
void get_list_from_tracker();
void parse_peer_list();
void display_peer_info();
void get_peers_which_have_the_file(string file, vector<string> &ip_list,
                                   vector<string> &port_list,vector<string> &group_list);
int call_leecher();
int call_seeder(string ip, u_int16_t port);
/**********************Functions used******************************/

void get_list_from_tracker() {
  /* gets tracker file from tracker */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in tracker_addr;
  tracker_addr.sin_family = AF_INET;
  tracker_addr.sin_port = htons(8888);
  tracker_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  connect(sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr));

  recv(sockfd, peer_list, 2000, 0);
  peer_list[strlen(peer_list)] = '\0';
  // cout << peer_list << endl;
  close(sockfd);
  return;
}

void parse_peer_list() {
  //Parse tracker list 
  vector<string> tokens;
  stringstream check1(peer_list);
  string intermediate;

  // Tokenizing w.r.t. newline
  while (getline(check1, intermediate, '\n')) {
    tokens.push_back(intermediate);
  }

  vector<string> tmp;
  for (auto peers : tokens) {
    vector<string> tokens_tmp;
    stringstream check(peers);
    //parse ip, port,group separted by ','' 
    while (getline(check, intermediate, ',')) {
      tokens_tmp.push_back(intermediate);
    }
    peerInfo p;
    //insert into structure
    p.ip = tokens_tmp[0];
    p.port = tokens_tmp[1];
    p.group=tokens_tmp[2];
    for (int i = 3; i < tokens_tmp.size(); i++) {
      p.files.push_back(tokens_tmp[i]);
    }
    //tracker file read into total_peers
    total_peers.push_front(p);
  }
}

void display_peer_info() {
  //File info for each ip:port
  for (auto peer : total_peers) {
    cout << peer.ip << ", ";
    cout << peer.port << endl;
   // cout << peer.group << endl;
    cout << "This peer has following files : " << "\n";
    for (auto files : peer.files) {
      cout << files << endl;
    }
  }
}

void get_peers_which_have_the_file(string file, vector<string> &ip_list,
                                   vector<string> &port_list,vector<string> &group_list) {
  //Search for file in totalpeers->files that has to be downloaded
  for (auto peer : total_peers) {
    for (auto file_name : peer.files) {
      if (file_name == file) {
        ip_list.push_back(peer.ip);
        port_list.push_back(peer.port);
        group_list.push_back(peer.group);
        break;
      }
    }
  }
}

int call_leecher() {
  //Forms connection and downloads file
  get_list_from_tracker();
  parse_peer_list();
  //display_peer_info();
  cout << "format:download_file​ <file_name> <destination_path> \nNote:destination_path terminated by '/' \n ";
  cin >> file; cin>>file; cin>>path;
  vector<string> ip_list;
  vector<string> port_list;
   vector<string> group_list;

  get_peers_which_have_the_file(file, ip_list, port_list,group_list);
  //peers which have file stored in ip_list and port_list
  pthread_t thread_id;

  for (int index = 0; index < ip_list.size(); index++) {

    //Form connection using ip, port
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tracker_addr;
    tracker_addr.sin_family = AF_INET;
    u_int16_t port_id = atoi(port_list[index].c_str());
    tracker_addr.sin_port = htons(port_id);
    tracker_addr.sin_addr.s_addr = inet_addr(ip_list[index].c_str());
    connect(sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr));

    if (pthread_create(&thread_id, NULL, leecher_handler, (void *)&sockfd) < 0) {
      perror("could not create thread");
      return 1;
    }

    // Now join the thread , so that we dont terminate before the thread
    pthread_join(thread_id, NULL);
    close(sockfd);
    //puts("Handler assigned");
  }

  return 0;
}

int call_seeder(string ip, u_int16_t port) {
  //Uploads file asked by user and waits for connection
  int socket_desc, client_sock, c;
  struct sockaddr_in seeder, client;

  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
  }
  puts("Socket created");

  // Prepare the sockaddr_in structure
  seeder.sin_family = AF_INET;
  seeder.sin_addr.s_addr = inet_addr(ip.c_str());
  seeder.sin_port = htons(port);

  // Bind
  if (bind(socket_desc, (struct sockaddr *)&seeder, sizeof(seeder)) < 0) {
    // print the error message
    perror("bind failed. Error");
    return 1;
  }
  puts("bind done");

  listen(socket_desc, 5); 
  
  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  pthread_t thread_id;

  while ((client_sock = accept(socket_desc, (struct sockaddr *)&client,
                               (socklen_t *)&c))) {
    puts("Connection accepted");

    if (pthread_create(&thread_id, NULL, seeder_handler,
                       (void *)&client_sock) < 0) {
      perror("could not create thread");
      return 1;
    }

    // Now join the thread , so that we dont terminate before the thread
    pthread_join(thread_id, NULL);
   // puts("Handler assigned");
  }

  if (client_sock < 0) {
    perror("accept failed");
    return 1;
  }
  close(socket_desc);

  return 0;
}




//Thread function for leecher 
void *leecher_handler(void *socket_desc) {

  int sock = *(int *)socket_desc;
  int read_size;

  // cout << " reached leecher fn \n";
  // send the name of file which is to be downloaded
  send(sock, file.c_str(), strlen(file.c_str()),0);

  //cout << "Enter folder path(terminated with '/') where to save this file : ";
  //string path;
  //cin >> path;
  char full_path[128];
  memset(full_path,0,128);
  //Appending file after directory path
  strcat(full_path, path.c_str());   
  strcat(full_path, file.c_str());
  //cout<<"path:"<<full_path<<endl;
  int file_size;
  
  recv(sock, &file_size, sizeof(file_size), 0);
  //cout<<"rcvd size:"<<file_size<<endl;
    
  char buffer[2000];
  FILE *fp;
  fp = fopen(full_path, "wb");
  if (fp == NULL) {
    printf("Error while opening the file for writing");
    exit(EXIT_FAILURE);
  }

  int n;
  //while ( ( n = recv( serverfd , Buffer ,   BUFF_SIZE, 0) ) > 0  &&  file_size > 0){
  //cout<<"\nServer: "<<Buffer<<endl;
  while (file_size>0 && (n = recv(sock,buffer,2000, 0) ) > 0  ){
  // cout<<"rcvd"<<buffer<<endl;
  int a=fwrite (buffer , sizeof(char), n, fp);
  //     fprintf(fp, "%s", buffer);
  memset ( buffer , '\0', 2000);
  file_size = file_size - n;
  //cout<<"file size:"<<file_size<<" n:"<<n<<endl;

} 

 //  recv(sock, buffer, 2000, 0);
 // buffer[strlen(buffer)] = '\0';
  //printf("Saving the file to %s \n", full_path);
  fclose(fp);
  // printf(" %s\n",buffer);
  return 0;
}

void *seeder_handler(void *socket_desc) {
  // Get the socket descriptor
  int sock = *(int *)socket_desc;
  int read_size;

  char file_name[127];
  recv(sock, file_name, 126, 0);
  file_name[strlen(file_name)] = '\0';
  // Send some messages to the client
  // char message[] = "Hi Peer !!! Here is the current list of Peer and their
  // files : \n"; write(sock , message , strlen(message));
  printf("File to open : %s \n", file_name);
  FILE *fp = fopen(file_name, "rb");
  char buffer[2000];
  int n;
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  rewind(fp);
  //cout<<"sent size:"<<size<<endl;
send ( sock , &size, sizeof(size), 0);

  while (size > 0 && (n = fread(buffer, sizeof(char), 2000, fp)) > 0  ) {
    send(sock, buffer, n, 0);
   // cout<<"sent"<<buffer<<endl;
    memset(buffer, '\0', 2000);
    size = size - n;
    // cout<<"file size:"<<size<<" n:"<<n<<endl;
  }
  printf("File send complete by seeder_2 \n");
  printf(" %s ", buffer);
  if (read_size == 0) {
    puts("Client disconnected");
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }

  return 0;
}

int main(int argc, char *argv[]) {
  char ch;   
  string ip;
  u_int16_t port;
  //Format : ./client ip:port 
  string str=argv[1];
  string word;
  stringstream stream(str);
  getline(stream, word, ':') ;  //getting ip from ip:port
   // cout << word << "\n";
  ip=word;
  getline(stream, word, ':') ;  //getting port from ip:port
  //cout << word << "\n";
  port=stoi(word);

  cout << "Press key according to operation: Upload:u  Download:d  create_user:c   login:l  list_files:f  show_downloads:s " << endl;
  cin >> ch;
  
  if(ch=='d') {
    call_leecher(); //Downloads file
  }

   if(ch=='u') {
    cout<<"Enter in this format : upload_file​ <file_path> : ";
    cin>>file; cin>>file;

    std::ofstream outfile;
    outfile.open("tracker_info.txt", std::ios_base::app); //open tracker file
    outfile<<ip<<","<<port<<","<<"0"<<","<<file;       //append ip, port, group, files in tracker file
    outfile << "\n";
    outfile.close(); 

    call_seeder(ip, port);   //uploads file

  } 

  if(ch=='c')  {
    //create_user
    string user,pwd;
    cout<<"Enter in this format : create_user​ <user_id> <passwd>\n "; 
    cin>>user; cin>>user; cin>>pwd;
     // userlist.push_back(user); passwordlist.push_back(pwd);
    ofstream outfile;
    outfile.open("users.txt", std::ios_base::app);
    outfile<<user<<"\n"<<pwd;  //append user pwd in users file
    outfile << "\n";
    cout<<"user created successfully\n";
    outfile.close(); 

  } 

   if(ch=='l') {
    //login and authenticate user
    string user,pwd;
    cout<<"Enter in this format : login <user_id> <passwd> \n"; 
    cin>>user; cin>>user; cin>>pwd;
    ifstream infile;
    infile.open("users.txt");
   
    string line;
    while(getline(infile, line)) {
      if(line==user) {            //Finds username
       getline(infile, line);  
    
        if(line==pwd){            //matches password in next line
          cout<<"login successful\n"; 
          auth=1; break;
        }
        else{
          cout<<"wrong password\n";
        }
      }
    }
    cout<<"account not found\n";
    infile.close();
  } 

   if(ch=='f')  {
    //Display files in tracker list
    cout<<"**********Files with different peers are*********\n";
    get_list_from_tracker();
    parse_peer_list();
   display_peer_info();

  }  

   if(ch=='s')  {
    //Displays files in download folder
     struct dirent *ptr;  
    cout<<"**********Files in Download folder are*********\n";
    DIR *file = opendir("/home/manik/Documents/p2p_2/tmp"); 
  
    while ((ptr = readdir(file)) != NULL) 
            printf("%s\n", ptr->d_name); 
  
    closedir(file);     

  }  



  return 0;
}

