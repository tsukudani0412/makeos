#ifndef _SAYO_OS_H_INCLUDED_
#define _SAYO_OS_H_INCLUDED_

#include "defines.h"
#include "interrupt.h"
#include "syscall.h"

/* System call */
sy_thread_id_t sy_run(sy_func_t func, char *name, int priority, int stacksize, int argc,char *argv[]);

void sy_exit(void);
int sy_wait(void);
int sy_sleep(void);
int sy_wakeup(sy_thread_id_t id);
sy_thread_id_t sy_getid(void);
int sy_chpri(int priority);
void *sy_kmalloc(int size);
int sy_kmfree(void *p);
int sy_send(sy_msgbox_id_t id, int size, char *p);
sy_thread_id_t sy_recv(sy_msgbox_id_t id, int *sizep, char **pp);
int sy_setintr(softvec_type_t type, sy_handler_t handler);

/* Service call */
int kx_wakeup(sy_thread_id_t id);
void *kx_kmalloc(int size);
int kx_kmfree(void *p);
int kx_send(sy_msgbox_id_t id, int size, char *p);

/* libary function */
void sy_start(sy_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]);
void sy_sysdown(void);
void sy_syscall(sy_syscall_type_t type, sy_syscall_param_t *param);
void sy_srvcall(sy_syscall_type_t type, sy_syscall_param_t *param);

/* system task */
int consdrv_main(int argc, char *argv[]);

/* User task */
int command_main(int argc, char *argv[]);

#endif
