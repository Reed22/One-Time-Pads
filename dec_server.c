#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h> 
#include <sys/wait.h>
#include <signal.h>

#define NUM_MESSAGES 2
#define CIPHER_MSG 0
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
*    void decrypt(int conn_socket, char cipher[],char key[])
*
* Decrypts cipher message and sends back to client
***************************************************************/
void decrypt(int conn_socket, char cipher[], char key[]){
    int cipher_len = strlen(cipher); //Holds length of ciphertext
    int cipher_num, key_num;  //Used to hold integer code for each char in cipher and key
    int chars_read = 0; //Used to determine if recv worked
    char plaintext[cipher_len]; //Used to hold final plaintext after decryption
    int diff; //Holds differnce of cipher_num and key_num
    int plain_index = 0; //Holds next index of plaintext[]

    //Go through each character of cipher and key to get plaintext
    for(int i = 1; i < cipher_len; i++){
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
        //If the difference is negative, add 27
        if(diff < 0){
          diff += 27;
        }

        //Insert character into plaintext[]
        plaintext[plain_index] = (diff % 27) + 65;

        //Accounts for space characrers
        if(plaintext[plain_index] == 65 + 26)
          plaintext[plain_index] = SPACE_ASCII;
        
        plain_index++;
    }

    //Send plaintext to client socket
    int expected_chars_sent = strlen(plaintext);
    while(chars_read < expected_chars_sent)
      chars_read += send(conn_socket, plaintext, strlen(plaintext), 0); 


    if (chars_read < 0)
      error("ERROR writing to socket");
}
/**************************************************************
*              int main(int argc, char *argv[])
***************************************************************/
int main(int argc, char *argv[]){
  int connectionSocket, chars_read, chars_written, expected_chars_sent, status; //Variables for socket
  char cipher_buf[100000]; //This needs to be larger
  char key_buf[100000];   //This needs to be larger
  pid_t process_array[256]; //Holds child processes
  int num_processes = 0;  //holds number of child processes; also used as index for process_array
  //Variables for socket
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

    //Fork a process and let child process do decryption
    pid_t child_pid = fork();
    if(child_pid == 0){
      //Clear out all buffers
      memset(cipher_buf, '\0', 100000);
      memset(key_buf, '\0', 100000);
      
      //Read the client's message from the socket
      for(int i = 0; i < NUM_MESSAGES; i++){
          //First message is ciphertext;
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
          }
      }
      //If first character is not lower_case 'd', then client is not dec_client
      //Send rejection message if so
      if(cipher_buf[0] != 'd'){
        char reject[] = "denied";
        chars_read += send(connectionSocket, reject, strlen(reject), 0); 
      }
      else{
          //Encrypt ciphertext and send to client socket
          decrypt(connectionSocket, cipher_buf, key_buf);
      }
      close(connectionSocket);
      exit(EXIT_SUCCESS);  
    }
    //If parent, add child process to array and kill any
    //processes that are done.
    else{
      process_array[num_processes] = child_pid;
      num_processes++;

      //Check for completed child processes.
      for(int i = 0; i < num_processes; i++){
          pid_t this_pid = waitpid(process_array[i], &status, WNOHANG);
          //If waitpid returns pid, kill that processes
          if(this_pid > 0){
              kill(this_pid, SIGTERM);                

              //Need to "delete" killed pid and decrement number of process
              for(int j = i; i < num_processes - 1; i++){
                  process_array[j] = process_array[j + 1];
              }
              num_processes--;
          }
      }
    }
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}