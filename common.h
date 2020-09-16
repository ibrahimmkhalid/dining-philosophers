#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXSTR 128
#define EXITSTR "CODE EXIT"

// get common name for a philosophers named pipe file
char* getFifoName(int id) {
  char* str = (char*)malloc(MAXSTR * sizeof(char));
  sprintf(str, "%s%d", "/tmp/phil_", id);
  return str;
}