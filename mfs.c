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
#include <stdint.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

struct __attribute__((__packed__)) DirectoryEntry {
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

// From fat32 pdf
struct FAT32 {
  char BS_OEMNAME[8];
  short BPB_BytsPerSec; // short
  unsigned BPB_SecPerClus;
  short BPB_RsvdSecCnt;
  unsigned BPB_NumFATS;
  short BPB_RootEntCnt;
  char BS_VolLab[11];
  int BPB_FATSz32;
  int BPB_RootClus;

  int RootDirSectors;
  int FirstDataSector;
  int FirstSectorofCluster;

  // storing our root offset
  int root_offset;
  int TotalFATSize;
  int bytesPerCluster;
};

// Helper functions
int LBAToOffset(int sector, struct FAT32 *fat);
unsigned NextLB(int sector, FILE *file, struct FAT32 *fat);
char* formatFileString(char* userInput);
int fileDoesExist(struct DirectoryEntry *dir, char* filename);

FILE* openFile(char *fileName, struct FAT32 *img, struct DirectoryEntry *dir);
void ls(FILE *file, struct FAT32 *fat, struct DirectoryEntry *dir);
void stat(struct DirectoryEntry *dir, FILE *file, char* userFileName);
void get(FILE *file, struct DirectoryEntry *dir, struct FAT32 *fat, char* userCleanName, char* userOriginalName);
void readFile(FILE *file, struct FAT32 *fat, struct DirectoryEntry dir, int offset, int numOfBytes);
void readDirectory(int cluster, FILE *file, struct DirectoryEntry *dir, struct FAT32 *fat);

int main()
{
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  char * currentFile = NULL;
  int hasFileClosed = 0;
  //Creating file pointer
  FILE *IMG = NULL;

  struct DirectoryEntry *dir = malloc(16 * sizeof(struct DirectoryEntry));

  // Create a single instance of image struct
  struct FAT32 *fat = malloc(sizeof(struct FAT32));

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
    // Open function : opens the file
    else if ( strcmp(token[0], "open") == 0){
      if(currentFile == NULL && IMG == NULL){
        IMG = openFile(token[1], fat, dir);
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
    // CLose Function : Close fat32.img
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
      if(IMG != NULL){
        printf("BPB_BytsPerSec:%6d %6x\n", fat->BPB_BytsPerSec, fat->BPB_BytsPerSec);
        printf("BPB_SecPerClus:%6d %6x\n", fat->BPB_SecPerClus, fat->BPB_SecPerClus);
        printf("BPB_RsvdSecCnt:%6d %6x\n", fat->BPB_RsvdSecCnt, fat->BPB_RsvdSecCnt);
        printf("BPB_NumFATS:%9d %6x\n", fat->BPB_NumFATS, fat->BPB_NumFATS);
        printf("BPB_FATSz32:%9d %6x\n", fat->BPB_FATSz32, fat->BPB_FATSz32);
        
      }else{
        printf("%s\n", "Error: File system not open.");
      }
	  }
    else if (strcmp(token[0], "stat") == 0){
      if(IMG != NULL){
        char *clean = NULL;
        clean = formatFileString(token[1]);
        stat(dir, IMG, clean);
      }else{
        printf("%s\n", "Error: File system not open.");
      }
    }
    else if (strcmp(token[0], "get") == 0){
     if(IMG != NULL){
        char *clean = NULL;
        clean = formatFileString(token[1]);
        get(IMG, dir, fat, clean, token[1]);
      }else{
        printf("%s\n", "Error: File system not open.");
      }
    }
    // cd Function : Change directory in Fat32.img
    else if (strcmp(token[0], "cd") == 0) {
      char * cleanFileName = NULL;
      int fileIndex;
      char * del = (char *)"/";
      char buffer[strlen(token[1])];
      char * fileToken;
      char * fileTokens[50];
      strcpy(buffer, token[1]);
      int i = 0;
      int maxTokenCount = 0;
      
      fileToken = strtok ( buffer, del);
      while (fileToken != NULL) {
        fileTokens[maxTokenCount] = (char *)malloc(sizeof(strlen(fileToken)));
        strcpy(fileTokens[maxTokenCount], fileToken);
        fileToken = strtok(NULL, del);
        maxTokenCount++;
      }


      if (IMG != NULL) {
        for ( i = 0; i < maxTokenCount; i++ ) {
          cleanFileName = formatFileString(fileTokens[i]);
          
          if ((fileIndex = fileDoesExist(dir, cleanFileName)) != -1) {
            if (dir[fileIndex].DIR_Attr == 0x10) {
              // printf("%s", dir[fileIndex].DIR_Name);
              readDirectory(dir[fileIndex].DIR_FirstClusterLow, IMG, dir, fat);
            } else if (dir[fileIndex].DIR_Name[0] == '.') {
              readDirectory(dir[fileIndex].DIR_FirstClusterLow, IMG, dir, fat);
            } else {
              printf("Error: Not a valid folder");
            }
          } 
        }
      } else {
        printf("%s\n", "Error: File system not open");
      }
      

    }
    else if (strcmp(token[0], "ls") == 0) {
     if(IMG != NULL){
       ls(IMG, fat, dir);
     }else{
        printf("%s\n", "Error: File system not open.");
      }
    }
    else if (strcmp(token[0], "read") == 0) {
      // TODO: Check if tokens are correct, more defensive programming
      // Might make another func to help with so
      int offset;
      int numOfBytes;
      char *cleanFileName = NULL;
      // read NUM.txt. 513 1
      cleanFileName = formatFileString(token[1]);
      offset = atoi(token[2]);
      numOfBytes = atoi(token[3]);
      int fileIndex;
      if(IMG != NULL){
        if((fileIndex = fileDoesExist(dir, cleanFileName)) != -1){
          // We now have the index of the file in the directory structure
          // printf("dir.DIR_FirstClusterLow: %d\n", dir[fileIndex].DIR_FirstClusterLow);
          readFile(IMG, fat, dir[fileIndex], offset, numOfBytes);
        }else{
          printf("%s\n", "File not found.");
        }
      }      
      // readOp(IMG, fat, position, filename, numOfBytes);
    }
    else if (strcmp(token[0], "volume") == 0) {
      if(IMG != NULL){
        printf("Volume is :%s\n", fat->BS_VolLab);
        
      }else {
        printf("%s\n", "Error: volume name not found.");
      }
    }
    
    // DEBUG - delete before turning in
    else if (strcmp(token[0], "exit") == 0) {
      return 0;
    }

    free( working_root );
  }
  return 0;
}


FILE* openFile(char *fileName, struct FAT32 *img, struct DirectoryEntry *dir){
  FILE *file;
  if(!(file=fopen(fileName, "r"))){
    printf("Error: File system image not found.\n");
    return 0;
  }
  // We have the file reading
  // Set all values of BPB Structure
  // ...this could be a lot cleaner.
  // 1. BPB_BytsPerSec, at offset 11
  fseek(file, 11, SEEK_SET);
  // read the value into the pointer to the struct
  fread(&img->BPB_BytsPerSec, 2, 1, file);
  // DEBUG - delete before turning in
  // printf("%d %x\n", img->BPB_BytsPerSec, img->BPB_BytsPerSec);
  // 2. BPB_SecPerClus, at offset 13
  fseek(file, 13, SEEK_SET);
  // ...repeat
  fread(&img->BPB_SecPerClus, 1, 1, file);
  // DEBUG - delete before turning in
  // printf("%d %x\n", img->BPB_SecPerClus, img->BPB_SecPerClus);
  // 3. BPB_RsvdSecCnt, at offset 14
  fseek(file, 14, SEEK_SET);
  // ...repeat
  fread(&img->BPB_RsvdSecCnt, 2, 1, file);
  // DEBUG - delete before turning in
  // printf("%d %x\n", img->BPB_RsvdSecCnt, img->BPB_RsvdSecCnt);
  // 4. BPB_NumFATs, at offset 16
  fseek(file, 16, SEEK_SET);
  // ...repeat
  fread(&img->BPB_NumFATS, 1, 1, file);
  // DEBUG - delete before turning in
  // printf("%d %x\n", img->BPB_NumFATS, img->BPB_NumFATS);
  // 5. BPB_FATSz32, at offset 36
  fseek(file, 36, SEEK_SET);
  // ...repeat
  fread(&img->BPB_FATSz32, 4, 1, file);
  // DEBUG - delete before turning in
  // printf("%d %x\n", img->BPB_FATSz32, img->BPB_FATSz32);

  // ...Others we didn't cover in class?
  // BS_OEMNAME
  fseek(file, 3, SEEK_SET);
  fread(&img->BS_OEMNAME, 8, 1, file);

  // BS_VolLab
  fseek(file, 71, SEEK_SET);
  fread(img->BS_VolLab, 11, 1, file);
  

  // BPB_RootEntCnt
  fseek(file, 17, SEEK_SET);
  fread(&img->BPB_RootEntCnt, 2, 1, file);

  // BPB_RootClus
  fseek(file, 44, SEEK_SET);
  fread(&img->BPB_RootEntCnt, 2, 1, file);

  // Defaults from pdf
  img->RootDirSectors = 0;
  img->FirstDataSector = 0;
  img->FirstSectorofCluster =  0;
  // Root Directory Address
  img->root_offset = (img->BPB_NumFATS * img->BPB_FATSz32 * img->BPB_BytsPerSec) + (img->BPB_RsvdSecCnt * img->BPB_BytsPerSec);
  img->bytesPerCluster = (img->BPB_SecPerClus * img->BPB_BytsPerSec);

  // Seek in the file where the root address is
  // loop and print the subdirectory files and
  fseek(file, img->root_offset, SEEK_SET);
  int i = 0;
  // fread 32 bytes into the directory entry array
  for(i=0; i<16; i++){
    fread(&dir[i], 32, 1, file);
  }

  // return a file pointer
  return file;
}

void readFile(FILE *file, struct FAT32 *fat, struct DirectoryEntry dir, int offset, int numOfBytes){
  int user_offset = offset;
  int block = dir.DIR_FirstClusterLow;
  printf("block: %d", block);
  int file_offset = LBAToOffset(block, fat);
  fseek(file, user_offset, SEEK_SET);
  
  int value;

  while( user_offset > fat->BPB_BytsPerSec){
    block = NextLB(block, file, fat);
    user_offset -= (fat->BPB_BytsPerSec);
    //printf("block:%2d, user_offset:%2d\n", block, user_offset);
  }

  file_offset = LBAToOffset(block, fat);
  printf("file_offset:%d\n", file_offset);
  fseek(file, file_offset + user_offset, SEEK_SET);
  fread(&value, numOfBytes, 1, file);
  printf("%d\n", value);
  int i = 0;
  // TODO
  for(i = 1; i < 10; i++){
    fread(&value, 1, 1, file);
    printf("%d\n", value);
  }
}

void ls(FILE *file, struct FAT32 *fat, struct DirectoryEntry *dir){
  // files with the archive flag
  int i = 0;
  signed char firstByteOfDIRName=  dir[2].DIR_Name[0];
  // Looks at all 16 directories
  for(i=0; i < 16; i++){
    
    // This is bad code vvv
    // But looks at first character is directory and compares it to 0xe5 in hex
    signed char firstByteOfDIRName=  dir[i].DIR_Name[0];
    if (  firstByteOfDIRName == (char)0xe5  ) {
      int j = 1; 
    } else if (dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 || dir[i].DIR_Attr == 0x01 ||  dir[i].DIR_Name[0] == '.')  {
      
      // temp char array for name
      char fileName[12];
      memset(fileName, 0, 12);
      strncpy(fileName, dir[i].DIR_Name, 11);
      printf("%2s\n", fileName);
    } 
  }
}

void stat(struct DirectoryEntry *dir, FILE *file, char* userFileName){
  int fileIndex;
  if((fileIndex = fileDoesExist(dir, userFileName)) != -1){
    // Print out more information
    // TODO
    printf("File Size: %d\n", dir[fileIndex].DIR_FileSize);
    printf("First Cluster Low: %d\n",  dir[fileIndex].DIR_FirstClusterLow);
    printf("DIR_ATTR: %d\n", dir[fileIndex].DIR_Attr);
    printf("First Cluster High: %d\n", dir[fileIndex].DIR_FirstClusterHigh);
  
  } else {
    printf("%s\n", "File not found.");
  }
}

void get(FILE *file, struct DirectoryEntry *dir, struct FAT32 *fat, char* userCleanName, char* userOriginalName){
  // fseek to that to either high or low cluster
  // fread by the size of the file
  // open a new file pointer to our system
  // write the file using fwrite?
  int fileIndex;
  // check if the file given exists
  if((fileIndex = fileDoesExist(dir, userCleanName)) != -1){
    // if it does
    FILE *localFile;
    int nextCluster;
    localFile = fopen(userOriginalName, "w");
    int size = dir[fileIndex].DIR_FileSize;
    int cluster = dir[fileIndex].DIR_FirstClusterLow;
    int offset = LBAToOffset(cluster, fat);

    fseek(file, offset, SEEK_SET);
    nextCluster = cluster;
    uint8_t value[512];
    while(size > 512){
      fread(&value, 512, 1, file);
      fwrite(&value, 512, 1, localFile);
      size -= 512;
      nextCluster = NextLB(nextCluster, file, fat);
      fseek(file, LBAToOffset(nextCluster, fat), SEEK_SET);
    }

    fread(&value, size, 1, file);
    fwrite(&value, size, 1, localFile);
    fclose(localFile);
    
  }else{
    printf("%s\n", "File not found.");
  }
}

/*
* Helper functions
*/

int LBAToOffset(int sector, struct FAT32 *fat){
  return ((sector - 2) * fat->BPB_BytsPerSec) + (fat->BPB_BytsPerSec * fat->BPB_RsvdSecCnt) + (fat->BPB_NumFATS * fat->BPB_FATSz32 * fat->BPB_BytsPerSec);
}

unsigned NextLB(int sector, FILE *file, struct FAT32 *fat){
  int FATAddress = (fat->BPB_BytsPerSec * fat->BPB_RsvdSecCnt) + (sector * 4);
  unsigned val;
  fseek(file, FATAddress, SEEK_SET);
  fread(&val, 2, 1, file);
  return val;
}

// sanitizer user input to match FAT file system spec
char* formatFileString(char* userInput) {
  char copyOfUser[strlen(userInput)];
  // Create a copy to tokenize so we
  // don't change the pointer
  strcpy(copyOfUser, userInput);
  char *filename;
  char *extension;
  char *token;
  char *toFATStr;
  int numOfSpaces;
  int numOfExtSpaces = 3;
  char * del = (char *) ".\n";
  // malloc the str to compare to the file system
  // 11 bytes total
  
  toFATStr = (char*)malloc(sizeof(char) * 11);
  
  if ( copyOfUser[0] == '.' && copyOfUser[1] == '.'){
    toFATStr = (char *) "..         ";
     
  } else if ( copyOfUser[0] == '.' ) {
    toFATStr = (char *) ".          ";
  }
  else {
    token = strtok(copyOfUser,del);
    filename = (char *)malloc(sizeof(token));
    strcpy(filename, token);
    int lenOfExtension;
    if((token = strtok(NULL, del)) != NULL){
      extension = (char *)malloc(sizeof(token));
      strcpy(extension, token);
      lenOfExtension = strlen(extension);
      numOfExtSpaces = 3 - lenOfExtension;
    } else {
      extension = (char *)malloc(sizeof(0));
      extension = (char *) "";
      numOfExtSpaces = 3;
    }
    
    // 8 bytes in a file name, take the range
    int lenOfFilename = strlen(filename);
    
    numOfSpaces = 8 - lenOfFilename;
    
    // printf("%s", toFATStr);
    // Concat the file name to our str
    strcat(toFATStr, filename);
    // if we have to append spaces
    if(numOfSpaces > 0){
      int i = 0;
      // concat spaces to match fat spec
      for(i = 0; i < numOfSpaces; i++){
        strcat(toFATStr, " ");
      }
    }

    // then concat file extension
    strcat(toFATStr, extension);
  
    if(numOfExtSpaces > 0){
      int i = 0;
      // concat spaces to match fat spec
      for(i = 0; i < numOfExtSpaces; i++){
        strcat(toFATStr, " ");
      }
    }
    // turn the toFATStr to all caps
    int i = 0;
    for(i = 0; i < strlen(toFATStr); i++){
      toFATStr[i] = toupper(toFATStr[i]);
    }

    return toFATStr;
  }
  return toFATStr;
}

int fileDoesExist(struct DirectoryEntry *dir, char* filename){
  int i = 0;
  for(i=0; i < 16; i++){
    if(dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 || dir[i].DIR_Name[0] == '.'){
      // temp char array for name
      char dirFileName[12];
      memset(dirFileName, 0, 12);
      strncpy(dirFileName, dir[i].DIR_Name, 11);
      if(strcasecmp(dirFileName, filename) == 0){
        return i; // return the index to provide a faster access
      }
    }
  }
  return -1; // it doesnt exist
}

void readDirectory(int cluster, FILE *file, struct DirectoryEntry *dir, struct FAT32 *fat) {
  int offset;
  if (cluster == 0) {
    offset = fat->root_offset;
  } else {
    offset = LBAToOffset(cluster, fat);
  }
  // int offset = LBAToOffset(cluster, fat);
  fseek(file, offset, SEEK_SET);
  int i;
  // fread 32 bytes into the directory entry array
  for(i=0; i<16; i++){
    fread(&dir[i], 32, 1, file);
   
  }
}
