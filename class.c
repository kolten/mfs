
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

short BPB_RsvdSecCnt;
short BPB_BytsPerSec;
int BPB_FATSz32;
unsigned BPB_SecPerClus;
unsigned BPB_NumFATS;

FILE *fp;

struct __attribute__((__packed__)) DirectoryEntry {
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstCluserHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstCluserLow;
  uint32_t DIR_FileSize;
};


int LBAToOffset(int sector);
unsigned NextLB(int sector);

int main(){
  struct DirectoryEntry dir[16];
  fp=fopen("fat32.img", "r");
  // Page 9 starts the reserve sections
  fseek(fp, 11, SEEK_SET);
  fread( &BPB_BytsPerSec, 2, 1, fp);

  fseek(fp, 13, SEEK_SET);
  fread(&BPB_SecPerClus, 1, 1, fp);

  fseek(fp, 14, SEEK_SET);
  fread(&BPB_RsvdSecCnt, 2, 1, fp);

  fseek(fp, 16, SEEK_SET);
  fread(&BPB_NumFATS, 1, 1, fp);

  fseek(fp, 36, SEEK_SET);
  fread(&BPB_FATSz32, 4, 1, fp);

  // printf("BPB_BytsPerSec:%6d %6x\n", BPB_BytsPerSec, BPB_BytsPerSec);
  // printf("BPB_SecPerClus:%6d %6x\n", BPB_SecPerClus, BPB_SecPerClus);
  // printf("BPB_RsvdSecCnt:%6d %6x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
  // printf("BPB_NumFATS:%9d %6x\n", BPB_NumFATS, BPB_NumFATS);
  // printf("BPB_FATSz32:%9d %6x\n", BPB_FATSz32, BPB_FATSz32);

  int root_offset = (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec);
  printf("%d\n", root_offset);
  fseek(fp, root_offset, SEEK_SET);

  int i = 0;
  for(i=0; i < 16; i++){
    fread(&dir[i], 32, 1, fp);
    
  }

  // INFO and LS
  // for(i = 0; i < 16; i++){
  //   if(dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 | dir[i].DIR_Attr == 0x01){
  //       char name[12];
  //       memset(name, 0 ,12);
  //       strncpy(name, dir[i].DIR_Name, 11);
  //       printf("%2s %6d %6d\n", name, dir[i].DIR_FileSize, dir[i].DIR_FirstCluserHigh);
  //   }
  // }

  int file_offset = LBAToOffset(17);
  fseek(fp, file_offset, SEEK_SET);

  uint8_t value;
  fread(&value, 1,1, fp);
  printf("%d", value);

  // read NUM.txt. 513 1
  int user_offset = 513;
  //DIR_FirstCluserLow 
  int block = 7216;
  while ( user_offset > BPB_BytsPerSec) 
  {
    block = NextLB(block);
    user_offset -= BPB_BytsPerSec;
  }

  //BLOCK HAS THE DATA WE NEED
  //now moved to beginning of block
  file_offset = LBAToOffset(block);
  fseek(fp, file_offset + user_offset, SEEK_SET);
  //READ OPERATION!!
  fread(&value, 1, 1, fp);
  for(i = 1; i < 10; i++){
    fread(&value, 1, 1, fp);
    // printf("%d", value);
  }

  // STAT
  // Loop over directory struct

  // CD
  // Change offset

  fclose(fp);
  return 0;
}

int LBAToOffset(int sector){
  return ((sector - 2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec);
}

unsigned NextLB(int sector){
  int FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
  unsigned val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}