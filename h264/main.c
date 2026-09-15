#include "main.h"

PSP_MODULE_INFO("h264", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-2048);

#define H264_FRAME_MAX_SIZE 0x4000
#define DMACPLUS_MAX_BYTE_COUNT (0x1000 - 64)

static struct RGB rgb __attribute__((aligned(64))) = {
  
  .stride = 512,
  .format = PSP_DISPLAY_PIXEL_FORMAT_8888,
  .height = 272,
  .bpp = 4,
};

static struct H264 h264 = {0};

static SceMpegRingbuffer ringBuffer = {0};
void h264Init(struct H264* const h264, struct RGB* const rgb) {
  
  sceMpegInit();
  memset(h264, 0, sizeof(struct H264));
  
  int size = (sceMpegQueryMemSize(0) + 63) & ~63;
  h264->data = memalign(64, size);
  memset(h264->data, 0, size);

  sceMpegCreate(&h264->mpeg, h264->data, size, &ringBuffer, rgb->stride, 0, 0);
  
  h264->frame = memalign(16, H264_FRAME_MAX_SIZE);
  h264->accessUnit.iEsBuffer = (int)sceMpegMallocAvcEsBuf(&h264->mpeg);
}

void allocLLI(struct MpegDMAC* const dmac) {
  
  const unsigned int lliCount = (dmac->count / DMACPLUS_MAX_BYTE_COUNT) + 1;
  const int size = (sizeof(struct DmacPlusLLI) * lliCount + 63) & ~63;
  dmac->lli = memalign(16, size);
  dmac->size = size;
}

static void mpegSc2MeLLI(struct MpegDMAC* const dmac) {
  
  unsigned int src = (unsigned int)dmac->src;
  unsigned int dst = (unsigned int)dmac->dst;
  
  int count = dmac->count;
  sceKernelDcacheWritebackInvalidateRange((void*)(dmac->src), count);

  unsigned int i = 0;
  while (count > DMACPLUS_MAX_BYTE_COUNT) {
    
    dmac->lli[i].src = (void*)src;
    dmac->lli[i].dst = (void*)dst;
    dmac->lli[i].count = DMACPLUS_MAX_BYTE_COUNT;
    dmac->lli[i].next = &(dmac->lli[i + 1]);

    src += DMACPLUS_MAX_BYTE_COUNT;
    dst += DMACPLUS_MAX_BYTE_COUNT;
    count -= DMACPLUS_MAX_BYTE_COUNT;
    i++;
  }
  
  dmac->lli[i].src = (void*)src;
  dmac->lli[i].dst = (void*)dst;
  dmac->lli[i].count = count;
  dmac->lli[i].next = 0;
  
  sceMpegbase_BEA18F91((void*)dmac->lli);
}

int h264StreamFrame(int fd, struct H264* const h264, int count) {
    
  const int read = sceIoRead(fd, h264->frame, count);
  const unsigned char* const frame = (unsigned char*)h264->frame;
  
  int size = 0;
  for (int i = 4; i < read - 4; i++) {
    
    const volatile unsigned char* const buf = &frame[i];
    
    int step = 0;
    unsigned char type = 0;

    if (buf[1]) {
      step = 1;
    }
    else if(buf[0]) {
    }
    else if (!buf[2] && buf[3] == 1) {
      type = buf[4] & 0x1f;
      step = 4;
    }
    else if (buf[2] == 1) {
      type = buf[3] & 0x1f;
      step = 3;
    }
    
    if (type == 1 || type == 7) {
      size = i;
      break;
    }
    
    i += step;
  }
  
  if (size) {
    const int step = size - read;
    sceIoLseek(fd, step, PSP_SEEK_CUR);
  }
  return size;
}

static unsigned int detail[16] = {0};
int sceMpegAvcDecodeDetail(void* mpeg, void* detail);

void* setFrameBuffer(int* const bufferIndex, struct RGB* const rgb) {
  
  const int rgbSize = rgb->stride * rgb->height * rgb->bpp;
  
  void* outFrame = (void*)(0x44000000 | ((*bufferIndex) * rgbSize));
  (*bufferIndex) = ((*bufferIndex) ^ 1) & 1;
  
  void* inFrame = (void*)(0x44000000 | ((*bufferIndex) * rgbSize));
  sceDisplaySetFrameBuf(outFrame, rgb->stride, rgb->format, PSP_DISPLAY_SETBUF_NEXTVSYNC);
  return inFrame;
}

void h264Read(char* const path, struct H264* const h264, struct RGB* const rgb) {
  
  const int fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
  
  h264->dmac.dst = (void*)h264->data[0x1AE];
  h264->dmac.count = H264_FRAME_MAX_SIZE;
  allocLLI(&h264->dmac);
  
  int bufferIndex = 0;
  rgb->frame = setFrameBuffer(&bufferIndex, rgb);

  SceCtrlData pad;
  do {
    
    //unsigned int vcount = sceDisplayGetVcount();
    unsigned int tick = sceKernelGetSystemTimeLow();
    sceCtrlPeekBufferPositive(&pad, 1);

    h264->dmac.count = h264StreamFrame(fd, h264, H264_FRAME_MAX_SIZE);
    
    if (!h264->dmac.count) { break; }
    h264->dmac.src = h264->frame;
    mpegSc2MeLLI(&h264->dmac);
    
    int init = 0;
    h264->accessUnit.iAuSize = h264->dmac.count;
    sceKernelDcacheWritebackInvalidateAll();
    sceMpegAvcDecode(&h264->mpeg, &h264->accessUnit, rgb->stride, &rgb->frame, (void*)&init);
    
    if (init) {
      
      do {
        detail[0] = -1;
        sceMpegAvcDecodeDetail(&h264->mpeg, (void*)detail);
        sceKernelDelayThread(1);
      } while (detail[0] != 0);
      
      rgb->frame = setFrameBuffer(&bufferIndex, rgb);
    }
    
    unsigned int elapsed = sceKernelGetSystemTimeLow() - tick;
    if (elapsed < 33333) {
      sceKernelDelayThread(33333 - elapsed);
    }
    
    /*
    while ((sceDisplayGetVcount() - vcount) < 2) {
      sceDisplayWaitVblankStart();
    }
    */
  } while(!(pad.Buttons & PSP_CTRL_SELECT));
  
  sceIoClose(fd);
}

void h264Clean(struct H264* const h264) {
  
  if (h264->mpeg) {

    sceMpegFlushAllStream(&h264->mpeg);
    sceMpegFreeAvcEsBuf(&h264->mpeg, (void*)h264->accessUnit.iEsBuffer);
    sceMpegDelete(&h264->mpeg);
    sceMpegFinish();
  }
  
  if (h264->data != 0) free(h264->data);
  if (h264->dmac.lli != 0) free(h264->dmac.lli);
  if (h264->frame != 0) free(h264->frame);
}

int main(int argc, char** argv) {
  
  sceDisplaySetMode(0, 480, 272);

  sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
  sceUtilityLoadAvModule(PSP_AV_MODULE_MPEGBASE);

  h264Init(&h264, &rgb);
  h264Read("frames.h264", &h264, &rgb);
  h264Clean(&h264);
  
  sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
  sceUtilityUnloadAvModule(PSP_AV_MODULE_MPEGBASE);

  sceKernelExitGame();
  return 0;
}
