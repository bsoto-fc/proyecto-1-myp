#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "argparser.h"
#include "server.h"

#define PROJECT_NAME "Xerces Server"
#define IP ""

int main(int argc, char *argv[]) {
  
  uint16_t PORT = ParsePort(argc,argv);
  
  printf("Iniciando %s\n",PROJECT_NAME);
  
  StartServer(PORT,IP);
  
  return 0;
}

