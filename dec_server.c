#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h> 

#define NUM_MESSAGES 2
#define CIPHER_MSG 0
#define KEY_MSG 1
#define SPACE_ASCII 32

bool debug = false;
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
*                 void decrypt(socket, cipher, key)
***************************************************************/
void decrypt(int conn_socket, char cipher[], char key[]){
    int cipher_len = strlen(cipher);
    int cipher_num, key_num;
    int chars_read = 0;
    char plaintext[cipher_len];
    int diff;

    //Go through each character of cipher and key to get plaintext
    for(int i = 0; i < cipher_len; i++){
        //Subtract 65 from ASCII to get order (A = 0, Z = 25)
        cipher_num = cipher[i] - 65;
        key_num = key[i] - 65;

        //If character is space, make the number 26 (one after Z)
        //Doing this after assigning nums to change num if the actual
        //character is a space
        if(cipher[i] == SPACE_ASCII)
          cipher_num = 26;

        if(key[i] == SPACE_ASCII)
          key_num = 26;
        
        //Mod 27 to account for space characters
        //Subtract cipher_num and key_num, take Modulus of 27, add 65 to get ASCII back
        diff = cipher_num - key_num;
        if(diff < 0){
          diff += 27;
        }

        plaintext[i] = (diff % 27) + 65;

        if(plaintext[i] == 65 + 26)
          plaintext[i] = SPACE_ASCII;

        //printf("( %c(%d) - %c(%d) ) MOD 27 = %d  +  65 = %c \n", cipher[i], cipher_num, key[i], key_num, ((diff) % 27), plaintext[i]);

    }

    int expected_chars_sent = strlen(plaintext);
    while(chars_read < expected_chars_sent)
      chars_read += send(conn_socket, plaintext, strlen(plaintext), 0); 
    
    if(debug){
      printf("Server: Expected(%d) ::: Actual(%d)\n", expected_chars_sent, chars_read);
    }

    if (chars_read < 0)
      error("ERROR writing to socket");
}
/**************************************************************
*              int main(int argc, char *argv[])
***************************************************************/
int main(int argc, char *argv[]){
  int connectionSocket, chars_read, chars_written, expected_chars_sent;
  char cipher_buf[100000]; //This needs to be larger
  char key_buf[100000];   //This needs to be larger
  char dec_client_req[256];
  char perm_resp[] = "granted";
  char denied[] = "denied";
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
    pid_t child_pid = fork();
    int child_status;
    if(child_pid == 0){
        memset(cipher_buf, '\0', 100000);
        memset(key_buf, '\0', 100000);
        memset(dec_client_req, '\0', 256);
        
        //Receive Authorization
        chars_read = recv(connectionSocket, dec_client_req, 255, 0); 
        if (chars_read < 0){
            error("ERROR reading from socket");
        }

        chars_written = 0;

        //Send permission granted message
        if(strcmp(dec_client_req, "dec_req") == 0){
            expected_chars_sent = strlen(perm_resp);
            while(chars_written < expected_chars_sent)
                chars_written += send(connectionSocket, perm_resp, strlen(perm_resp), 0); 
        }
        else {
            expected_chars_sent = strlen(denied);
            while(chars_written < expected_chars_sent)
                chars_written += send(connectionSocket, denied, strlen(denied), 0); 
        }

        //Read the client's message from the socket
        for(int i = 0; i < NUM_MESSAGES; i++){
            //First message is plaintext;
            if(i == CIPHER_MSG){
            chars_read = recv(connectionSocket, cipher_buf, 99999, 0); 
            if (chars_read < 0){
                error("ERROR reading from socket");
            }
            }

            //Second message is key
            else if(i == KEY_MSG){
            chars_read = recv(connectionSocket, key_buf, 99999, 0); 
            if (chars_read < 0){
                error("ERROR reading from socket");
            }
            //printf("SERVER: I received this from the client: \"%s\"\n", key_buf);
            }
        }

        //Decrypt ciphertext and send
        decrypt(connectionSocket, cipher_buf, key_buf);

        close(connectionSocket); 
    }
    else
      child_pid = waitpid(child_pid, &child_status, WNOHANG);

  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}