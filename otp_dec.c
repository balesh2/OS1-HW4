#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char* msg) {
  perror(msg);
  exit(1);
}

int check(char* arg, int low, int upp, int exc) {
   int i;

   i = 0;
   while(arg[i] != 0) {
      if((low-1) < arg[i] && arg[i] < (upp+1) || arg[i] == exc) {
	 i++;
      }
      else {
	 return 1;
      }
   }
   
   return 0;
}

char* getcontents(char* filename) {
   char* cont;
   FILE* file;
   char buffer[150001];

   cont = malloc(sizeof(char)*150001);

   file = fopen(filename, "r");
   fgets(buffer, 150001, file);
   fclose(file);
   sprintf(cont, "%s", buffer);

   return cont;
}

int main(int argc, char** argv) {
  int portno, sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[150001];
  char* message;
  char* k;

  if(argc < 4) {
    fprintf(stderr, "usage: %s plaintext key port", argv[0]);
    exit(1);
  }

  if(check(argv[3], 48, 57, -1)) {
     fprintf(stderr, "please enter a valid port number\n");
     exit(1);
  }
  else {
     portno = atoi(argv[3]);
  }

  message = getcontents(argv[1]);
  if(!check(message, 65, 90, 32)) {
     fprintf(stderr, "the message contains invalid characters");
     exit(1);
  }
  k = getcontents(argv[2]);
  if(!check(k, 65, 90, 32)) {
     fprintf(stderr, "the key contains invalid characters");
     exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd < 0) {
    error("ERROR opening socket");
  }
  server = gethostbyname("Localhost");
  if(server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(1);
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
  bzero(buffer, 150001);
  sprintf(buffer, "%s%s", message, k);
  n = write(sockfd, buffer, strlen(buffer));
  if(n < 0) {
    error("ERROR writing to socket");
  }
  bzero(buffer, 150001);
  n = read(sockfd, buffer, 150000);
  if(n < 0) {
    error("ERROR reading from socket");
  }
  printf("%s\n", buffer);
  close(sockfd);

  return 0;
}
