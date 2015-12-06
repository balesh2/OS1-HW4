#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char* msg) {
  perror(msg);
  exit(1);
}

char** spitmsg(char msg[256]) {
  char** args;
  int i;

  args = malloc(sizeof(char*)*2);
  args[0] = malloc(sizeof(char)*128);
  args[1] = malloc(sizeof(char)*128);

  args[0] = strtok(msg, " ");
  sprintf(args[0], "%s\n", args[0]);
  args[1] = msg;

  printf("%s\n", args[0]);
  printf("%s\n", args[1]);
  }

  return args;
}

int enc(char msg[256]) {
  char** args;

  args = splitmsg(msg);

  return 0;
}

int launch() {
  int status;

  //fork the process
  pid = fork();
  //error if it didn't fork right
  if(pid == 0) {
    exit(EXIT_FAILURE);
  }
  //error if fork returns an error
  else if(pid < 0) {
    //error forking
    perror("otp_enc_d");
    return 1;
  }
  //if everything is right, background it
  else {
    //init wpid so we can see if it changed
    wpid = -1;
    //wait for the background process if it is done, otherwise keep going
    wpid = waitpid(-1, &status, WNOHANG);
    //if the process has ended, print out the end message
  }

  //return the status as 0 or error (1)
  if(status == 0) {
    return 0;
  }
  else {
    return 1;
  }
}

int connect(int sockfd, char buffer[256], int status) {
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if(newsockfd < 0) {
    error("ERROR on accept");
  }
  //TODO get forking working
  //status = launch();
  bzero(buffer, 256);
  n = read(newsockfd, buffer, 255);
  if(n < 0) {
    error("ERROR reading from socket");
  }
  fprintf(message, "%s\n", buffer);
  printf("message: %s", message);
  n = write(newsockfd, "message received", 18);
  if (n < 0) {
    error("ERROR writing to socket");
  }
  close(newsockfd);

  status = enc(message);

  return status;
}

int main(int argc, char** argv) {
  int sockfd, portno;
  char buffer[256];
  struct sockaddr_in serv_addr;
  int status;
  char message[256];
  pid_t wpid;

  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    error("ERROR opening socket");
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }

  //do {
    status = connect(sockfd, buffer);

    //wpid = -1;
    //wpid = waitpid(-1, &status, WNOHANG);
  //}while (status != -1);

  close(sockfd);

  return 0;
}
