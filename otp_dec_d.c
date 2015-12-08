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

//prints out error and exit with code 1
void error(const char* msg) {
  perror(msg);
  exit(1);
}

//splits the message on newline
char** splitmsg(char msg[150001]) {
  char** args;
  int i;
  char* token;

  //allocate array of two strings and the strings
  args = malloc(sizeof(char*)*2);
  args[0] = malloc(sizeof(char)*75000);
  args[1] = malloc(sizeof(char)*75000);

  //tokenize the string on '\n'
  token = strtok(msg, "\n");
  //print the first part of the message to arg[0]
  sprintf(args[0], "%s\n", token);
  token = strtok(NULL, "\n");
  //print the second part of the message to arg[1]
  sprintf(args[1], "%s\n", token);

  //return the heap address of the array of args
  return args;
}

//decrypts the message
char* decryptstr(char* message, char* key) {
  char* decrypted;
  int i, n;

  //allocates the decrypted message
  decrypted = malloc(sizeof(char)*150001);

  //iterates through each char in the message
  for(i=0; i<150002; i++) {
    //if the end of the message has been reached, append the newline to the decrypted message and break
    if(message[i] == '\n') {
      decrypted[i] = '\n';
      break;
    }
    else {
      //replace the space character in the message with #26
      if(message[i] == 32) {
        message[i] = 26;
      }
      //replaces other characters with the corresponding letter number between 0 and 25
      else {
        message[i] = message[i] - 65;
      }

      //does the same character replacements as above for the key
      if(key[i] == 32) {
        key[i] = 26;
      }
      else {
        key[i] = key[i] - 65;
      }

      //subtracts the key char from the message char
      n = message[i] - key[i];

      //wraps around if the number is less than 0
      if(n < 0) {
        n = 27 + n;
      }

      //replaces #26 with a space
      if(n == 26) {
        decrypted[i] = 32;
      }
      //shifts all other characters back to their real ascii values
      else {
        decrypted[i] = n + 65;
      }
    }
  }

  //free malloc-ed stuff that we are done with
  free(key);
  free(message);

  //return the heap address of the decrypted text
  return decrypted;
}

//splits and decrypts the message
char* dec(char message[15001]) {
  char** args;

  //splits the message on newlines
  args = splitmsg(message);
  //decrypts the message
  args[0] = decryptstr(args[0], args[1]);

  //returns the heap address of the decrypted text
  return args[0];
}

//forks the process
int launch(socklen_t clilen, int newsockfd, struct sockaddr_in cli_addr) {
  int status, n;
  pid_t pid, wpid;
  char* decr;
  char message[150001];

  //fork the process
  pid = fork();
  //child process
  if(pid == 0) {
    //init message to empty
    bzero(message, 150001);
    //read message from socket
    n = read(newsockfd, message, 150001);
    //error if read fails
    if(n < 0) {
      error("ERROR reading from socket");
    }

    //decrypt the message
    decr = dec(message);
    //error if decrypting fails
    if(decr == NULL) {
      n = write(newsockfd, "Error\n", 6);
      close(newsockfd);
      return 1;
    }

    //loops through decrypted message 
    for(n=0; n<150002; n++) {
      //if the end of the message has been reached, append newline char and break
      if(decr[n] == '\n') {
        message[n] = '\n';
        break;
      }

      //move decrypted char to message array
      message[n] = decr[n];
    }

    //free malloc-ed stuff we are done with
    free(decr);

    //write the decrypted message to the socket
    n = write(newsockfd, message, n);
    //error if write fails
    if (n < 0) {
      error("ERROR writing to socket");
    }

    //close socket
    close(newsockfd);

    //return -1 to say that the process ended
    return -1;
  }
  //error if fork returns an error
  else if(pid < 0) {
    perror("otp_enc_d");
    return 1;
  }
  //if everything is right, let the parent continue
  else {
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);
    return 0;
  }
}

int connectsockets(int sockfd, char buffer[256], int status) {
  int newsockfd, n;
  socklen_t clilen;
  struct sockaddr_in cli_addr;

  //listen on socket for a connection
  listen(sockfd, 5);
  //get the length of the client's address
  clilen = sizeof(cli_addr);
  //accept the connection
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  //error if connection fails
  if(newsockfd < 0) {
    error("ERROR on accept");
  }

  //init the buffer to empty
  bzero(buffer, 256);
  //read from the client for handshake
  n = read(newsockfd, buffer, 256);
  //error if read fails
  if(n < 0) {
    error("ERROR reading handshake");
  }

  //reinit buffer to empty
  bzero(buffer, 256);
  //put 'dec' in buffer for handshake
  sprintf(buffer, "dec");
  //write handshake to client
  n = write(newsockfd, buffer, strlen(buffer));
  //error if write failed
  if(n < 0) {
    error("ERROR writing handshake");
  }
  //launch the new process
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

  //checks that a port was provided
  if (argc < 2) {
    //errors if no port was provided
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  //init socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  //error if socket does not exist
  if(sockfd < 0) {
    error("ERROR opening socket");
  }

  //init server address to empty
  bzero((char *) &serv_addr, sizeof(serv_addr));
  //convert port number to integer
  portno = atoi(argv[1]);
  //connect to client
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  //error if binding failed
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }

  //loop until the process completes
  do {
    //wait for any completed processes
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);

    //connect to client
    status = connectsockets(sockfd, buffer, status);

    //exit with success if decryption completed successfully
    if(status == -1) {
      exit(EXIT_SUCCESS);
      return 0;
    }

    //exit with failure if it failed anywhere within connect
    if(status == 1) {
      exit(EXIT_FAILURE);
      return 1;
    }

    //wait for completed processes again
    wpid = -1;
    wpid = waitpid(-1, &status, WNOHANG);
  }while (status != -1);

  //close socket
  close(sockfd);

  return 0;
}
