#include "defines.h"
#include "sayo-os.h"
#include "interrupt.h"
#include "lib.h"

static int start_threads(int argc, char *argv[]) {
  sy_run(consdrv_main, "consdrv", 1, 0x200, 0, NULL);
  sy_run(command_main, "command", 8, 0x200, 0, NULL);

  sy_chpri(15);
  INTR_ENABLE;
  while(1) {
    asm volatile ("sleep");
  }

  return 0;
}

int main(void) {
  INTR_DISABLE;

  puts("\nkozos boot succeed!\n");

  sy_start(start_threads, "idle", 0, 0x100, 0, NULL);

  return 0;
}
