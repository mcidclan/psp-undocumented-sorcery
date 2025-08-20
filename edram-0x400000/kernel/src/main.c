#include "wrap.h"
#include <pspsdk.h>

PSP_MODULE_INFO("wrap", 0x1000, 1, 1);
PSP_NO_CREATE_MAIN_THREAD();

extern int sceGeEdramSetSize(int size);

int setEdramSize(unsigned int size) {
  return sceGeEdramSetSize(size);
}

int module_start(SceSize args, void *argp){
  return 0;
}

int module_stop() {
  return 0;
}
