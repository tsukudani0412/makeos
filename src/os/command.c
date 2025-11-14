#include "defines.h"
#include "sayo-os.h"
#include "consdrv.h"
#include "lib.h"


static void send_use(int index) {
  char *p;
  p = sy_kmalloc(3);
  p[0] = '0';
  p[1] = CONSDRV_CMD_USE;
  p[2] = '0' + index;
  sy_send(MSGBOX_ID_CONSOUTPUT, 3, p);
}

static void send_write(char *str) {
  char *p;
  int len;
  len = strlen(str);
  p = sy_kmalloc(len + 2);
  p[0] = '0';
  p[1] = CONSDRV_CMD_WRITE;
  memcpy(&p[2], str, len);
  sy_send(MSGBOX_ID_CONSOUTPUT, len+2, p);
}

int command_main(int argc, char *argv[]) {
  char *p;
  int size;

  send_use(SERIAL_DEFAULT_DEVICE);

  while(1) {
    send_write("command> ");

    sy_recv(MSGBOX_ID_CONSINPUT, &size, &p);
    p[size] = '\0';
  
    if(!strncmp(p, "echo", 4)) {
      send_write(p + 4);
      send_write("\n");
    } else {
      send_write("unknown.\n");
    }
  
    sy_kmfree(p);
  }

  return 0;
}
