#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

int main() {
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (("%d\n", c) != 10){
            printf("Number: %d / Character: (%c)\n", c,c);  
        }
    }
  return 0;
}