#include "socketutil.h"

void error(char* msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

int CreateTCPIPv4Socket(){
  return socket(AF_INET, SOCK_STREAM, 0); // Regresa negativo si algo sale mal.
}

struct sockaddr_in* CreateIPv4Address(char* ip, uint16_t port){
  struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in)); // IPv4 struct
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port); // htons garantiza que se utilice el Endian correcto para la conexión.
  if(strlen(ip) == 0)
    addr->sin_addr.s_addr = INADDR_ANY; 
  else
    inet_pton(AF_INET,ip,&addr->sin_addr.s_addr); // Convertir ip a unsigned integer y colocarlo en &addr.sin_addr.s_addr.
  return addr;
}
