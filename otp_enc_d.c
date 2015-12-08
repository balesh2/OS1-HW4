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

//prints error message and exits with status of 1
void error(const char* msg) {
  perror(msg);
  exit(1);
}

//splits the message in half on newline char
char** splitmsg(char msg[150001]) {
  char** args;
  int i;
  char* token;

  //allocates array of strings
  args = malloc(sizeof(char*)*2);
  args[0] = malloc(sizeof(char)*75000);
  args[1] = malloc(sizeof(char)*75000);

  //tokenizes message on newline char
  token = strtok(msg, "\n");
  //prints first half of message to the first string in array
  sprintf(args[0], "%s\n", token);
  token = strtok(NULL, "\n");
  //prints second half of message to the second string in array
  sprintf(args[1], "%s\n", token);

  //returns array of strings
  return args;
}

//encrypts the strings
char* encryptstr(char* message, char* key) {
  char* encrypted;
  int i, n;

  //allocates a string
  encrypted = malloc(sizeof(char)*75000);

  //loops through each character in the message
  for(i=0; i<75000; i++) {
    //breaks out of loop if end of message has been reached
    if(message[i] == '\n') {
      encrypted[i] = '\n';
      break;
    }
    //replace spaces with #26
    else {
      if(message[i] == 32) {
        message[i] = 26;
      }
      //replace letters with #0-26
      else {
        message[i] = message[i] - 65;
      }

      //do the same letter replacements as above on the key
      if(key[i] == 32) {
        key[i] = 26;
      }
      else {
        key[i] = key[i] - 65;
      }

      //add the message and key values
      n = message[i] + key[i];

      //if the number is greater than 26, wrap around past 0
      if(n > 26) {
        n = n - 27;
      }

      //replace the number 26 with spaces
      if(n == 26) {
        encrypted[i] = 32;
      }
      //replace all other chars with their actual ascii values
      else {
        encrypted[i] = n + 65;
      }
    }
  }

  //free malloc-ed stuff that we are done with
  free(key);
  free(message);

  //return the encrypted string
  return encrypted;
}

//splits message and encrypts it
char* enc(char message[15001]) {
  char** args;

  //splits message in half
  args = splitmsg(message);
  //encrypts message
  args[0] = encryptstr(args[0], args[1]);

  //returns encrypted message
  return args[0];
}

//forks process
int launch(socklen_t clilen, int newsockfd, struct sockaddr_in cli_addr) {
  int status, n, i;
  pid_t pid, wpid;
  char* encr;
  char message[150001], msglen[7], tmp[2];

  //fork the process
  pid = fork();
  //child process
  if(pid == 0) {
    //init the message to empty
    bzero(message, 150001);
    //read the message from the socket
    n = read(newsockfd, message, 150001);
    //error if read failed
    if(n < 0) {
      error("ERROR reading from socket");
    }

    //encrypt the message
    encr = enc(message);
    //error if encryption failed
    if(encr == NULL) {
      n = write(newsockfd, "Error\n", 6);
      close(newsockfd);
      return 1;
    }

    //reinit message to empty
    bzero(message, 150001);
    //loop through each char in encrypted text
    for(n=0; n<75000; n++) {
      //break if end of string has been reached
      if(encr[n] == '\n') {
        message[n] = '\n';
        break;
      }
      //copy the encrypted string to the message array
      message[n] = encr[n];
    }

    //write the encrypted message to the client
    n = write(newsockfd, message, 150001);
    //error if encryption failed
    if(n < 0) {
      error("ERROR writing");
    }
    //close socket
    close(newsockfd);
    //free malloc-ed stuff we are done with
    free(encr);

    //return -1 for successful completion
    return -1;
  }
  //error if fork returns an error
  else if(pid < 0) {
    perror("otp_enc_d");
    return 1;
  }
  //if everything is right, let the parent continue
  else {
    //check for any finished processes
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);
    return 0;
  }
}

int connectsockets(int sockfd, char buffer[256], int status) {
  int newsockfd, n;
  socklen_t clilen;
  struct sockaddr_in cli_addr;

  //listen on the socket
  listen(sockfd, 5);
  //get the length of the client's address
  clilen = sizeof(cli_addr);
  //accept connection
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  //error if accept failed
  if(newsockfd < 0) {
    error("ERROR on accept");
  }

  //init buffer to empty
  bzero(buffer, 256);
  //read from client
  n = read(newsockfd, buffer, 256);
  //error if handshake failed
  if(n < 0) {
    error("ERROR reading handshake");
  }

  //reinit buffer to empty
  bzero(buffer, 256);
  //print enc to buffer
  sprintf(buffer, "enc");
  //write to client for handshake
  n = write(newsockfd, buffer, strlen(buffer));
  //error if write failed
  if(n < 0) {
    error("ERROR writing handshake");
  }

  //launch the process
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

  //check that a port was provided
  if (argc < 2) {
    //error if there is no port
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  //init the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  //error if socket failed
  if(sockfd < 0) {
    error("ERROR opening socket");
  }

  //init server address to empty
  bzero((char *) &serv_addr, sizeof(serv_addr));
  //convert port number to an integer
  portno = atoi(argv[1]);

  //bind to the port
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  //error if bind fails
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }

  //loop until process completes
  do {
    //wait for any finished processes
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);

    //connect to the client
    status = connectsockets(sockfd, buffer, status);

    //exit with success if encryption completed successfully
    if(status == -1) {
      exit(EXIT_SUCCESS);
      return 0;
    }

    //exit with failure if encryption failed anywhere
    if(status == 1) {
      exit(EXIT_FAILURE);
      return 1;
    }

    //wait again for any finished processes
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);
  }while (status != -1);

  //close socket
  close(sockfd);

  return 0;
}
