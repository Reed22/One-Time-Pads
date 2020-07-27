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

bool debug = false;
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
*                 cchar* readFile(char* file_name)
***************************************************************/
char* readFile(int socket, char* file_name, int length){
  	//int file_descriptor;
	  char file_path[256];
    char c;
    char* readBuffer;
    //int length; 
    FILE* file;

    file = fopen(file_name, "r");

    // Determine how many characters are in the file
    fseek (file, 0, SEEK_END);
    length = ftell(file);

    //readBuffer = malloc(length * sizeof(char));
    char buf[length];
    //Rest file pointer to beginning
    fseek (file, 0, SEEK_SET);

    for(int i = 0; i < length; i++){
      c = getc(file);
      if(!isupper(c) && !isspace(c)){
        fprintf(stderr, "Error: Bad character detected\n");
        return NULL;
      }
      buf[i] = c;
    }
    //Strip the newline character
    buf[length - 1] = '\0';
    fclose(file);
    return readBuffer;
}

/**************************************************************
*                 char* sendFile(char* file_name)
***************************************************************/
char* sendFile(int socket, char* file_name, int length){
  	//int file_descriptor;
	  char file_path[256];
    char c;
    FILE* file;
    char buf[length];
    int chars_written;

    file = fopen(file_name, "r");

    fseek (file, 0, SEEK_SET);

    for(int i = 0; i < length; i++){
      c = getc(file);
      if(!isupper(c) && !isspace(c)){
        fprintf(stderr, "Error: Bad character detected\n");
        return NULL;
      }
      buf[i] = c;
    }
    //Strip the newline character
    buf[length] = '\0';
    
    if(debug){
      printf("buf: %s\n", buf);
    }

    fclose(file);

    chars_written = send(socket, buf, strlen(buf), 0); 

    if (chars_written < 0){
      error("CLIENT: ERROR writing to socket");
    }
    
    if (chars_written < strlen(buf)){
      printf("CLIENT: WARNING: Not all data written to socket!\n");
    }
    return NULL;
}


/**************************************************************
*                 int fileLength(char* file_name)
***************************************************************/
int fileLength(char* file_name){
  	//int file_descriptor;
	  char file_path[256];
    char c;
    char* readBuffer;
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
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char* host_name = "localhost";
  char buffer[256];
  
  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); 
    exit(0); 
  } 
  //Check to make sure key is at least as big as plaintext
  //argv[1] = plaintext    argv[2] = key
  int plaintext_len = fileLength(argv[1]);
  int key_len = fileLength(argv[2]);

  if(fileLength(argv[1]) > fileLength(argv[2])){
    fprintf(stderr,"Error: Key is shorter than plaintext\n");
    exit(1);
  }

  if(debug){
    printf("Plaintext LEN: %d\n", plaintext_len);
    printf("Key Len: %d\n", key_len);
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
  //Send plaintext
  sendFile(socketFD, argv[1], plaintext_len);
  sendFile(socketFD, argv[2], key_len);


  // Get input message from user
  //printf("CLIENT: Enter text to send to the server, and then hit enter: ");
  // Clear out the buffer array
  //memset(buffer, '\0', sizeof(buffer));
  // Get input from the user, trunc to buffer - 1 chars, leaving \0
  //fgets(buffer, sizeof(buffer) - 1, stdin);
  // Remove the trailing \n that fgets adds
  //buffer[strcspn(buffer, "\n")] = '\0'; 

  // Send message to server
  // Write to the server
  //charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  //if (charsWritten < 0){
  //  error("CLIENT: ERROR writing to socket");
  //}
  //if (charsWritten < strlen(buffer)){
  //  printf("CLIENT: WARNING: Not all data written to socket!\n");
  //}

  // Get return message from server
  // Clear out the buffer again for reuse
  //memset(buffer, '\0', sizeof(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  printf("CLIENT: I received this from the server: \"%s\"\n", buffer);

  // Close the socket
  close(socketFD); 
  return 0;
}