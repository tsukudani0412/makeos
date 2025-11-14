#include "defines.h"
#include "sayo-os.h"
#include "lib.h"
#include "memory.h"

/* memory block header */
typedef struct _symem_block {
  struct _symem_block *next;
  int size;
} symem_block;

/* memory pool */
typedef struct _symem_pool {
  int size;
  int num;
  symem_block *free;
} symem_pool;

static symem_pool pool[] = {
  {16, 8, NULL}, {32, 8, NULL}, {64, 4, NULL}
};

#define MEMORY_AREA_NUM (sizeof(pool) / sizeof(*pool))

static int symem_init_pool(symem_pool *p) {
  int i;
  symem_block *mp;
  symem_block **mpp;
  extern char freearea;
  static char *area = &freearea;

  mp = (symem_block*)area;

  mpp = &p->free;
  for(i=0; i<p->num; i++) {
    *mpp = mp;
    memset(mp, 0, sizeof(*mp));
    mp->size = p->size;
    mpp = &(mp->next);
    mp = (symem_block *)((char *)mp + p->size);
    area += p->size;
  }

  return 0;
}

int symem_init(void) {
  int i;
  for(i=0; i<MEMORY_AREA_NUM; i++) {
    symem_init_pool(&pool[i]);
  }
  return 0;
}

void *symem_alloc(int size) {
  int i;
  symem_block *mp;
  symem_pool *p;

  for(i=0; i<MEMORY_AREA_NUM; i++) {
    p = &pool[i];

    if(size <= (p->size - sizeof(symem_block))) {
      if(p->free == NULL) {
        /* if memory block exhausted */
        sy_sysdown();
        return NULL;
      }
      mp = p->free;
      p->free = p->free->next;
      mp->next = NULL;

      /* add header padding */
      return mp+1;
    }
  }
  /* size exceeded all block size */
  sy_sysdown();
  return NULL;
}

void symem_free(void *mem) {
  int i;
  symem_block *mp;
  symem_pool *p;

  mp = ((symem_block *)mem - 1);

  for(i=0; i<MEMORY_AREA_NUM; i++) {
    p = &pool[i];
    if(mp->size == p->size) {
      mp->next = p->free;
      p->free = mp;
      return;
    }
  }
  sy_sysdown();
}



