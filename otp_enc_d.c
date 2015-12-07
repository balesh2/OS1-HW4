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

char** splitmsg(char msg[150001]) {
  char** args;
  int i;
  char* token;

  args = malloc(sizeof(char*)*2);
  args[0] = malloc(sizeof(char)*75000);
  args[1] = malloc(sizeof(char)*75000);

  token = strtok(msg, "\n");
  sprintf(args[0], "%s\n", token);
  token = strtok(NULL, "\n");
  sprintf(args[1], "%s\n", token);

  return args;
}

char* encryptstr(char* message, char* key) {
   char* encrypted;
   int i, n;

   encrypted = malloc(sizeof(char)*75000);

   for(i=0; i<75000; i++) {
      if(message[i] == '\n') {
	 encrypted[i] = '\n';
	 break;
      }
      else {
	 if(message[i] == 32) {
	    message[i] = 26;
	 }
	 else {
	    message[i] = message[i] - 65;
	 }
	 if(key[i] == 32) {
	    key[i] = 26;
	 }
	 else {
	    key[i] = key[i] - 65;
	 }

	 n = message[i] + key[i];

	 if(n > 26) {
	    n = n - 27;
	 }

	 if(n == 26) {
	    encrypted[i] = 32;
	 }
	 else {
	    encrypted[i] = n + 65;
	 }
      }
   }

   free(key);
   free(message);

   return encrypted;
}

char* enc(char message[15001]) {
  char** args;

  args = splitmsg(message);
  args[0] = encryptstr(args[0], args[1]);

  return args[0];
}

int launch(socklen_t clilen, int newsockfd, struct sockaddr_in cli_addr) {
  int status, n;
  pid_t pid, wpid;
  char* encr;
  char message[150001];

  //fork the process
  pid = fork();
  if(pid == 0) {
     //child process
     bzero(message, 150001);
     n = read(newsockfd, message, 150001);
     if(n < 0) {
       error("ERROR reading from socket");
     }

     encr = enc(message);
     if(encr == NULL) {
	n = write(newsockfd, "Error\n", 6);
	close(newsockfd);
	return 1;
     }
     
     bzero(message, 150001);
     for(n=0; n<75000; n++) {
	if(encr[n] == '\n') {
	   message[n] = '\n';
	   break;
	}
	message[n] = encr[n];
     }

     n = write(newsockfd, message, 75000);
     if (n < 0) {
       error("ERROR writing to socket");
     }
     close(newsockfd);
     free(encr);

     return -1;
  }
  //error if fork returns an error
  else if(pid < 0) {
    //error forking
    perror("otp_enc_d");
    return 1;
  }
  //if everything is right, background the parent
  else {
     wpid = -1;
     wpid = waitpid(-1, &status, WNOHANG);
     return 0;
  }

  //return the status as 0 or error (1)
  if(status == 0) {
    return 0;
  }
  else {
    return 1;
  }
}

int connectsockets(int sockfd, char buffer[256], int status) {
   int newsockfd, n;
   socklen_t clilen;
   struct sockaddr_in cli_addr;

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if(newsockfd < 0) {
    error("ERROR on accept");
  }
  bzero(buffer, 256);
  n = read(newsockfd, buffer, 256);
  if(n < 0) {
     error("ERROR reading handshake");
  }
  bzero(buffer, 256);
  sprintf(buffer, "enc");
  n = write(newsockfd, buffer, strlen(buffer));
  if(n < 0) {
     error("ERROR writing handshake");
  }
  status = launch(clilen, newsockfd, cli_addr);

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

  do {
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);

    status = connectsockets(sockfd, buffer, status);
    if(status == -1) {
       exit(EXIT_SUCCESS);
       return 0;
    }

    if(status == 1) {
       exit(EXIT_FAILURE);
       return 1;
    }

    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);
  }while (status != -1);

  close(sockfd);

  return 0;
}
