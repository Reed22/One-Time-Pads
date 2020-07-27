#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NUM_MESSAGES 2
#define PLAIN_MSG 0
#define KEY_MSG 1
#define SPACE_ASCII 32

/**************************************************************
*                 void error(const char *msg)
***************************************************************/
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

/**************************************************************
* void setupAddressStruct(struct sockaddr_in* address, 
*                         int portNumber)
***************************************************************/
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}
/**************************************************************
*                 void encrypt(plain, key)
***************************************************************/
void encrypt(int conn_socket, char plain[], char key[]){
    int plain_len = strlen(plain);
    int plain_num, key_num, charsRead;
    char ciphertext[plain_len];

    //plain_ascii = 'A' - 65;
    //printf("PLAIN ASCII: %d\n", plain_ascii);

    for(int i = 0; i < plain_len; i++){
        //Subtract 65 from ASCII to get order (A = 0, Z = 25)
        plain_num = plain[i] - 65;
        key_num = key[i] - 65;

        //If character is space, make the number 26 (one after Z)
        //Doing this after assigning nums to change num if the actual
        //character is a space
        if(plain[i] == SPACE_ASCII)
          plain_num = 26;

        if(key[i] == SPACE_ASCII)
          key_num = 26;
        
        //Mod 27 to account for space characters....need confirmation that this is okay. 
        //Add plain_num and key_num
        //Take Modulus of 27
        //Add 65 to get ASCII back
        ciphertext[i] = ((plain_num + key_num) % 27) + 65;

        if(ciphertext[i] == 26 + 65)
          ciphertext[i] = SPACE_ASCII;
    }

    charsRead = send(conn_socket, ciphertext, strlen(ciphertext), 0); 
    if (charsRead < 0)
      error("ERROR writing to socket");
}
/**************************************************************
*              int main(int argc, char *argv[])
***************************************************************/
int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  char plain_buf[256];
  char key_buf[256];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);


  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }

    printf("SERVER: Connected to client running at host %d port %d\n", 
                          ntohs(clientAddress.sin_addr.s_addr),
                          ntohs(clientAddress.sin_port));

    // Get the message from the client and display it
    memset(plain_buf, '\0', 256);
    memset(key_buf, '\0', 256);
    for(int i = 0; i < NUM_MESSAGES; i++){
        // Read the client's message from the socket
        //First message is plaintext;
        if(i == PLAIN_MSG){
          charsRead = recv(connectionSocket, plain_buf, 255, 0); 
          if (charsRead < 0){
            error("ERROR reading from socket");
          }
          printf("SERVER: I received this from the client: \"%s\"\n", plain_buf);
        }

        //Second message is key
        else if(i == KEY_MSG){
          charsRead = recv(connectionSocket, key_buf, 255, 0); 
          if (charsRead < 0){
            error("ERROR reading from socket");
          }
          printf("SERVER: I received this from the client: \"%s\"\n", key_buf);
        }
    }

    //Encrypt plaintext and send
    encrypt(connectionSocket, plain_buf, key_buf);
/*
    // Send a Success message back to the client
    charsRead = send(connectionSocket, 
                    "I am the server, and I got your message", 39, 0); 
    if (charsRead < 0){
      error("ERROR writing to socket");
    }
    // Close the connection socket for this client
*/
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}