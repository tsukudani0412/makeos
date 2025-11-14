#ifndef _SAYO_OS_MEMORY_H_INCLUDED_
#define _SAYO_OS_MEMORY_H_INCLUDED_

int symem_init(void);
void *symem_alloc(int size);
void symem_free(void *mem);

#endif
