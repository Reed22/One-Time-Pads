#include <stdio.h> 
#include <stdlib.h>
#include <time.h> 

#define LOWER_LIM 65    //65 is ascii for 'A'
#define UPPER_LIM 91    //90 is ascii for 'Z'; if 91, use space
#define SPACE_ASCII 32  //Ascii for space character

/**************************************************************
*                 int main(int argc, char *argv[])
*
* This program generates a random key consisting of capital
* letters and spaces.
*
* USAGE :  keygen key_length 
***************************************************************/
int main( int argc, char *argv[] )  {
    if(argc != 2){
        printf("Incorrect number of arguments\n");
        printf("USAGE: keygen key_length\n");
        exit(1);
    }
    
    int key_len = atoi(argv[1]); //Key length
    int rand_num; 
    char key[key_len + 2]; // key_len + newline + '\0'

    //Last character of key needs to be newline
    key[key_len] = '\n'; 

    srand(time(0));
    
    //Insert letters into key
    for(int i = 0; i < key_len; i++){
        //Generate randum number in range (LOWER_LIM, UPPER_LIM)
        rand_num = (rand() % (UPPER_LIM - LOWER_LIM + 1)) + LOWER_LIM;

        //If number is 91, make letter a space character
        if(rand_num == UPPER_LIM)
            key[i] = SPACE_ASCII;
        //rand_num will hold ascii represenation of capital letter A-Z
        else
            key[i] = rand_num;

     }
     printf("%s", key);

    return 0;
}
