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

//prints out error message and exits with status of 1
void error(const char* msg) {
  perror(msg);
  exit(1);
}

//checks that there are only valid characters and returns count
int check(char* arg, int low, int upp, int exc) {
  int i;

  i = 0;
  //loops until EOF
  while(arg[i] != 0 && arg[i] != '\n') {
    //checks that the character is within the given range, or equal to the exception
    //the exception for most of the runs of check with be the space character
    if((low-1) < arg[i] && arg[i] < (upp+1) || arg[i] == exc) {
      //adds one to the count if the character is ok
      i++;
    }
    else {
      //returns an error if there is a character that is not allowed
      return -1;
    }
  }

  //returns the character count if all chars are good
  return i;
}

//reads the contents of the file
char* getcontents(char* filename) {
  char* cont;
  FILE* file;
  char buffer[150001];

  //allocate contents of file var
  cont = malloc(sizeof(char)*150001);

  //open file
  file = fopen(filename, "r");
  //get the contents of the file
  fgets(buffer, 150001, file);
  //close the file
  fclose(file);
  //put the contents of the buffer into the var holding file contents
  sprintf(cont, "%s", buffer);

  //return the heap address of file contents var
  return cont;
}

//checks that the arguments given are ok
void checkargs(int argc, char** argv) {
  //checks that there are enough arguments
  if(argc < 4) {
    //errors if there are not enough
    fprintf(stderr, "usage: %s plaintext key port\n", argv[0]);
    exit(1);
  }

  //checks that the given port number is actually a number
  if(check(argv[3], 48, 57, -1) == -1) {
    //errors if it isn't
    fprintf(stderr, "please enter a valid port number\n");
    exit(1);
  }
}

int main(int argc, char** argv) {
  int portno, sockfd, n, l1, l2;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[150001];
  char* message;
  char* k;

  //checks that the arguments given are ok
  checkargs(argc, argv);

  //get portnumber as an integer
  portno = atoi(argv[3]);

  //get contents of message file
  message = getcontents(argv[1]);
  //check that there are no bad characters
  l1 = check(message, 65, 90, 32);
  if(l1 == -1) {
    //error if there are bad chars
    fprintf(stderr, "the message contains invalid characters\n");
    exit(1);
  }

  //get contents of key file
  k = getcontents(argv[2]);
  //check that there are no bad characters in key
  l2 = check(k, 65, 90, 32);
  if(l2 == -1) {
    //error if there are
    fprintf(stderr, "the key contains invalid characters\n");
    exit(1);
  }

  //check that the key is not shorter than the message
  if(l2 < l1) {
    //error if it is
    fprintf(stderr, "the key is shorter than the message\n");
    exit(1);
  }

  //init the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  //error if the socket didn't open
  if(sockfd < 0) {
    error("ERROR opening socket");
  }

  //connect to localhost
  server = gethostbyname("Localhost");
  //error if it can't be found
  if(server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(1);
  }

  //init the server address to zero
  bzero((char *) &serv_addr, sizeof(serv_addr));
  //set server address sinfamily to AF_INET
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
      (char *)&serv_addr.sin_addr.s_addr,
      server->h_length);
  //open the port
  serv_addr.sin_port = htons(portno);
  //attempt to connect
  if(connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    //error if unable to connect
    perror("ERROR connecting");
    exit(2);
  }

  //init the buffer to empty
  bzero(buffer, 150001);
  //put dec in the buffer
  sprintf(buffer, "dec");
  //write the buffer to otp_dec_d
  n = write(sockfd, buffer, strlen(buffer));
  //error if write failed
  if(n < 0) {
    error("ERROR writing handshake");
  }

  //reinit buffer to empty
  bzero(buffer, 150001);
  //read from the port
  n = read(sockfd, buffer, 150000);
  //error if read failed
  if(n < 0) {
    error("ERROR reading handshake");
  }
  //check that dec was returned in the handshake
  if(!(buffer[0] == 'd' && buffer[1] == 'e' && buffer[2] == 'c')) {
    //error if it wasn't
    error("ERROR cannot connect to encryption program");
  }

  //reinit buffer to empty
  bzero(buffer, 150001);
  //combine contents of message and key files into the buffer
  sprintf(buffer, "%s%s", message, k);

  //free malloc-ed variables
  free(message);
  free(k);

  //write key and message to otp_dec_d
  n = write(sockfd, buffer, strlen(buffer));
  //error if write failed
  if(n < 0) {
    error("ERROR writing to socket");
  }

  //reinit buffer to empty
  bzero(buffer, 150001);
  //read from the socket
  n = read(sockfd, buffer, 150000);
  //error if read failed
  if(n < 0) {
    error("ERROR reading from socket");
  }

  //print decrypted text to stdout
  printf("%s\n", buffer);

  //close socket
  close(sockfd);

  return 0;
}
