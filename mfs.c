// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

FILE* openFile(char *fileName);

int main()
{
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  char * currentFile = NULL;
  int hasFileClosed = 0;
  //Creating file pointer
  FILE *IMG = NULL;

  while( 1 )
  {
    // Print out the msh prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Checking first token to carry out
    // which required functionality
    if ( token[0] == NULL){
      
    }
    else if ( strcmp(token[0], "open") == 0){
      if(currentFile == NULL && IMG == NULL){
        IMG = openFile(token[1]);
        // we have an open file, set it as our current file
        if(IMG != NULL){
          currentFile = (char *)malloc(sizeof(token[1]));
          strcpy(currentFile, token[1]);
        }
      }else{
        if(strcmp(currentFile, token[1]) == 0){
          printf("Error: File system image is already open.\n");
        }
      }
    }
    else if ( strcmp(token[0], "close") == 0){
      // 0 is success
      // -1 is failed
      // If the file pointer is not null
      if(IMG != NULL){
        int res = fclose(IMG);
        printf("%d\n", res);
        if(res == 0){
          // Reset the current filename && file pointer to null
          currentFile = NULL;
          IMG = NULL;
          // Set a flag for checking on other operations
        }
      }else{
        printf("%s\n", "Error: File system not open.");
      }
    }
    else if (strcmp(token[0], "info") == 0){
      
	  }
    else if (strcmp(token[0], "stat") == 0){
      
    }
    else if (strcmp(token[0], "get") == 0){
     
    }
    else if (strcmp(token[0], "cd") == 0) {
     
    }
    else if (strcmp(token[0], "ls") == 0) {
     
    }
    else if (strcmp(token[0], "read") == 0) {

    }
    else if (strcmp(token[0], "volume") == 0) {

    }

    // DEBUG
    else if (strcmp(token[0], "exit") == 0) {
      fclose(IMG);
      free(IMG);
      return 0;
    } 

    free( working_root );
  }
  return 0;
}


FILE* openFile(char *fileName){
  FILE *file;
  if(!(file=fopen(fileName, "r"))){
    printf("Error: File system image not found.\n");
    return 0;
  }
  // Move to the beginning of the file and return
  fseek(file, 0, SEEK_SET);
  return file;
}
