#include "common.h"

int main(int argc, char* argv[]) {
  char* pipeFileName = getFifoName(atoi(argv[1]));
  int pfd = open(pipeFileName, O_RDONLY);
  char* buf = (char*)malloc(MAXSTR * sizeof(char));
  while (1) {
    if (read(pfd, buf, MAXSTR) > 0) {
      if (strstr(buf, EXITSTR))
        break;
      else
        printf("%s", buf);
    } else {
      break;
    }
  }
  free(pipeFileName);
  free(buf);
  close(pfd);
  return 0;
}