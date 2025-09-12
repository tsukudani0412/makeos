#include "defines.h"
#include "kozos.h"
#include "lib.h"


int test08_1_main(int argc, char *argv[]) {
  char buf[32];

  puts("test08_1 started.\n");

  while(1) {
    puts("> ");
    gets(buf);

    if(!strncmp(buf, "echo", 4)) {
      puts(buf + 4);
      puts("\n");
    } else if(!strcmp(buf, "exit")) {
      break;
    } else {
      puts("unknown\n");
    }
  }
}

int test08_1_sub(int argc, char **argv) {
  puts("2nd thread started\n");
  return 0;
}
