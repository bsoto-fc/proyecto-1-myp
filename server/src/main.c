#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "argparser.h"

#define PROJECT_NAME "Xerces Server"

int main(int argc, char *argv[]) {
  
  uint16_t PORT = ParsePort(argc,argv);
  
  printf("Iniciando %s\n",PROJECT_NAME);
  
  return 0;
}

