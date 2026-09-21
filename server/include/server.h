#ifndef SERVER_H_
#define SERVER_H_

#include <stdint.h>
#include "../util/socketutil.h"

void StartServer(uint16_t port,char* ip);

void* ReceiveKeyboardCommands(); 

void CreateStdinThreadForInput();

struct thread_info{
    int socketFD;
    AcceptedSocket* pSocket; // Referencia para poder liberar memoria. 
};

void* receiveAndPrintIncomingData(void* data); 

void* receiveAndSendResponse(void* data); 

void receiveAndSendResponseOnSeparateThread(AcceptedSocket* pSocket); 

void receiveAndPrintIncomingDataOnSeparateThread(AcceptedSocket* pSocket); 

void startAcceptingIncomingConnections(int serverSocketFD);
#endif // SERVER_H_
