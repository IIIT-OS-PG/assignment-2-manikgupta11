/*
    C socket server example, handles multiple clients using threads
    Compile
    gcc server.c -lpthread -o server
*/

#include <bits/stdc++.h>

#include <arpa/inet.h> //inet_addr
#include <pthread.h>   //for threading , link with lpthread
#include <stdio.h>
#include <stdlib.h> //strlen
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> //write

using namespace std;
// the thread function
char peer_list[2000];

struct peerInfo {
  string ip;
  string port;
  vector<string> files;
};
vector<peerInfo> total_peers;
void *peer_handler(void *);
void *connection_handler(void *);
string file;

void get_list_from_tracker() {
  /* first get list from tracker */
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
  vector<string> tokens;
  // stringstream class check1
  stringstream check1(peer_list);
  string intermediate;
  // Tokenizing w.r.t. space ' '
  while (getline(check1, intermediate, '\n')) {
    tokens.push_back(intermediate);
  }

  vector<string> tmp;
  for (auto peers : tokens) {
    vector<string> tokens_tmp;
    stringstream check(peers);
    while (getline(check, intermediate, ',')) {
      tokens_tmp.push_back(intermediate);
    }
    peerInfo p;
    p.ip = tokens_tmp[0];
    p.port = tokens_tmp[1];
    for (int i = 2; i < tokens_tmp.size(); i++) {
      p.files.push_back(tokens_tmp[i]);
    }
    total_peers.push_back(p);
  }
}

void display_peer_info() {
  for (auto peer : total_peers) {
    cout << peer.ip << ", ";
    cout << peer.port << endl;
    cout << "This peer has following files : "
         << "\n";
    for (auto files : peer.files) {
      cout << files << endl;
    }
  }
}

void get_peers_which_have_the_file(string file, vector<string> &ip_list,
                                   vector<string> &port_list) {

  for (auto peer : total_peers) {
    for (auto file_name : peer.files) {
      if (file_name == file) {
        ip_list.push_back(peer.ip);
        port_list.push_back(peer.port);
        break;
      }
    }
  }
}

int call_leecher() {
  get_list_from_tracker();
  parse_peer_list();
  display_peer_info();
  cout << "Input the file name which you want to download : ";
  cin >> file;
  vector<string> ip_list;
  vector<string> port_list;

  get_peers_which_have_the_file(file, ip_list, port_list);
  pthread_t thread_id;

  size_t si = 0, interval = 200;

  for (int index = 0; index < ip_list.size(); index++) {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tracker_addr;
    tracker_addr.sin_family = AF_INET;
    u_int16_t port_id = atoi(port_list[index].c_str());
    tracker_addr.sin_port = htons(port_id);
    tracker_addr.sin_addr.s_addr = inet_addr(ip_list[index].c_str());
    connect(sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr));

    if (pthread_create(&thread_id, NULL, peer_handler, (void *)&sockfd) < 0) {
      perror("could not create thread");
      return 1;
    }

    // Now join the thread , so that we dont terminate before the thread
    pthread_join(thread_id, NULL);
    puts("Handler assigned");
  }

  return 0;
}

int call_seeder(string ip, u_int16_t port) {
  int socket_desc, client_sock, c;
  struct sockaddr_in seeder, client;

  // Create socket
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

  // Listen
  listen(socket_desc, 3);

  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);

  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  pthread_t thread_id;

  while ((client_sock = accept(socket_desc, (struct sockaddr *)&client,
                               (socklen_t *)&c))) {
    puts("Connection accepted");

    if (pthread_create(&thread_id, NULL, connection_handler,
                       (void *)&client_sock) < 0) {
      perror("could not create thread");
      return 1;
    }

    // Now join the thread , so that we dont terminate before the thread
    pthread_join(thread_id, NULL);
    puts("Handler assigned");
  }

  if (client_sock < 0) {
    perror("accept failed");
    return 1;
  }

  return 0;
}



int main(int argc, char *argv[]) {
  char ch;
  cout << " If you want to upload press - U, else press - D to download :  "
       << endl;

  cin >> ch;
  switch (ch) {
  case 'D': {
    call_leecher();
  } break;
  case 'U': {
    cout << "Enter IP address : ";
    string ip;
    cin >> ip;

    cout << "Enter Port Number : ";
    u_int16_t port;
    cin >> port;

    call_seeder(ip, port);

  } break;
  default:
    break;
  }
  return 0;
}

/*
 * This will handle connection for each client
 * */
void *peer_handler(void *socket_desc) {
  // Get the socket descriptor
  int sock = *(int *)socket_desc;
  int read_size;

  // cout << " I am in the peer handler \n";
  // send the name of file which is to be download
  write(sock, file.c_str(), strlen(file.c_str()));

  cout << "Enter folder path where to save this file : ";
  string path;
  cin >> path;
  char full_path[128];
  memset(full_path,0,128);
  strcat(full_path, path.c_str());
  strcat(full_path, file.c_str());

  char buffer[2048];
  recv(sock, buffer, 2048, 0);
  buffer[strlen(buffer)] = '\0';
  printf("Saving the file to %s \n", full_path);

  FILE *fp;
  fp = fopen(full_path, "w+");
  if (fp == NULL) {
    printf("Error while opening the file for writting");
    exit(EXIT_FAILURE);
  }

  fprintf(fp, "%s", buffer);
  fclose(fp);

  // printf(" received from seeder : %s\n",buffer);

  if (read_size == 0) {
    puts("Client disconnected");
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }

  return 0;
}

void *connection_handler(void *socket_desc) {
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
  char buffer[2048];
  int n;
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  rewind(fp);

  while ((n = fread(buffer, sizeof(char), 2048, fp)) > 0 && size > 0) {
    send(sock, buffer, n, 0);
    memset(buffer, '\0', 2048);
    size = size - n;
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
