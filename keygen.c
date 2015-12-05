#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  int len, i, r;
  char c;

  //seeds random with time
  srand(time(NULL));

  //converts the length passed in from string to int
  len = atoi(argv[1]);

  //loops for the number of characters requested
  for(i=0; i<len; i++) {
    //generates a random number in the range of 0 to 26
    r = rand()%27;

    //prints a space if the number was 0
    if(r == 0) {
      printf(" ");
    }
    //prints the letter if it is not 0
    else {
      //converts the integer to char by assignment, adds 64 so that 1 becomes 65
      //(ascii num for A)
      c = r + 64;
      //prints char
      printf("%c", c);
    }
  }

  //prints a newline character at the end of the string
  printf("\n");

  return 0;
}
