#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h> 

#define A_ASCII 65    //65 is ascii for 'A'
#define Z_ASCII 90    //90 is ascii for 'Z'
#define SPACE_ASCII 32  //Ascii for space character

/**************************************************************
*                 void error(const char *msg)
***************************************************************/
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

/**************************************************************
* void setupAddressStruct(struct sockaddr_in* address, 
*                         int portNumber, 
*                         char* hostname)
*
* Set up the address struct
***************************************************************/
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

/**************************************************************
*                 char* sendFile(char* file_name)
***************************************************************/
int sendFile(int socket, char* file_name, int length){
	  //char file_path[256];
    char c;             //holds char of file before inserting into buf
    FILE* file;         //file pointer used to open file_name
    char buf[length];   //holds full text from file
    int chars_written = 0;  //used to tell if all text was written

    file = fopen(file_name, "r");

    fseek (file, 0, SEEK_SET);

    for(int i = 0; i < length; i++){
      c = getc(file);
      if(!isupper(c) && !isspace(c)){
        fprintf(stderr, "Error: Bad character detected\n");
        return -1;
      }
      buf[i] = c;
    }
    //Strip the newline character
    buf[length] = '\0';

    fclose(file);
    int expected_chars_written = strlen(buf);
    while(chars_written < expected_chars_written)
      chars_written += send(socket, buf, strlen(buf), 0); 

    if (chars_written < 0){
      error("CLIENT: ERROR writing to socket");
    }
    
    if (chars_written < strlen(buf)){
      printf("CLIENT: WARNING: Not all data written to socket!\n");
    }
    return 1;
}


/**************************************************************
*                 int fileLength(char* file_name)
***************************************************************/
int fileLength(char* file_name){
    int length; 
    FILE* file;

    file = fopen(file_name, "r");

    // Determine how many characters are in the file
    fseek (file, 0, SEEK_END);
    length = ftell(file);

    //return length - 1 to exclude newline character from count
    return length - 1;
}

/**************************************************************
*                 int main(int argc, char *argv[])
***************************************************************/
int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead, success;
  struct sockaddr_in serverAddress;
  char* host_name = "localhost";
  
  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); 
    exit(0); 
  } 
  //Check to make sure key is at least as big as plaintext
  //argv[1] = plaintext    argv[2] = key
  int ciphertext_len = fileLength(argv[1]);
  int key_len = fileLength(argv[2]);

  if(fileLength(argv[1]) > fileLength(argv[2])){
    fprintf(stderr,"Error: Key is shorter than plaintext\n");
    exit(1);
  }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
   //argv[3] = port
  setupAddressStruct(&serverAddress, atoi(argv[3]), host_name);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  //Send Authorization
  char dec_req[] = "dec_req";
  char dec_res[256];
  int expected_chars_written = strlen(dec_req);
  int chars_written = 0;

  //Send Authorization
  while(chars_written < expected_chars_written)
      chars_written += send(socketFD, dec_req, strlen(dec_req), 0);

  //Receive Authorization response
  charsRead = recv(socketFD, dec_res, sizeof(dec_res) - 1, 0);
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  if(strcmp(dec_res, "granted") != 0){
    fprintf(stderr,"Error: Permission Denied\n");
    exit(2);
  }

  //Send plaintext and key
  //Returns -1 if bad character detected
  success = sendFile(socketFD, argv[1], ciphertext_len);
  if(success == -1){
    exit(1);
  }

  success = sendFile(socketFD, argv[2], key_len);
  if(success == -1){
    exit(1);
  }

  //Declare buff with length + 1 (+1 to account for newline)
  char buffer[ciphertext_len + 1]; 
  memset(buffer, '\0', sizeof(buffer));

  //Read ciphertext
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }

  //Insert newline character and null characterto end of buffer
  //buffer[plaintext_len] = '\n';
  //buffer[plaintext_len+1] = '\0';

  printf("%s\n", buffer);


  // Close the socket
  close(socketFD); 
  return 0;
}