#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char* msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char** argv) {
  int portno, sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];

  if(argc < 4) {
    fprintf(stderr, "usage: %s plaintext key port", argv[0]);
    exit(0);
  }

  portno = atoi(argv[3]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd < 0) {
    error("ERROR opening socket");
  }
  server = gethostbyname("Localhost");
  if(server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
      (char *)&serv_addr.sin_addr.s_addr,
      server->h_length);
  serv_addr.sin_port = htons(portno);
  if(connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    exit(2);
  }
  bzero(buffer, 256);
  sprintf(buffer, "%s %s\n", argv[1], argv[2]);
  n = write(sockfd, buffer, strlen(buffer));
  if(n < 0) {
    error("ERROR writing to socket");
  }
  bzero(buffer, 256);
  n = read(sockfd, buffer, 255);
  if(n < 0) {
    error("ERROR reading from socket");
  }
  printf("%s\n", buffer);
  close(sockfd);

  return 0;
}
