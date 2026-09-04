#include <stdio.h>
#include <stdint.h>
#include "argparser.h"

#define PROJECT_NAME "Xerces Server"

int main(int argc, char *argv[]) {
  
  uint16_t PORT = ParsePort(argc,argv);
  
  if(PORT == 0)
    return 1;
  
  printf("El puerto es: %d\n",PORT);
  
  return 0;
}

