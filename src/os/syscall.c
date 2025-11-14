#include "defines.h"
#include "interrupt.h"
#include "sayo-os.h"
#include "interrupt.h"
#include "syscall.h"

sy_thread_id_t sy_run(sy_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
  sy_syscall_param_t param;

  param.un.run.func = func;
  param.un.run.name = name;
  param.un.run.priority = priority;
  param.un.run.stacksize = stacksize;
  param.un.run.argc = argc;
  param.un.run.argv = argv;
 
  sy_syscall(SY_SYSCALL_TYPE_RUN, &param);
  
  return param.un.run.ret;
}

void sy_exit(void) {
  sy_syscall(SY_SYSCALL_TYPE_EXIT, NULL);
}

int sy_wait(void) {
  sy_syscall_param_t param;
  sy_syscall(SY_SYSCALL_TYPE_WAIT, &param);
  return param.un.wait.ret;
}

int sy_sleep(void) {
  sy_syscall_param_t param;
  sy_syscall(SY_SYSCALL_TYPE_SLEEP, &param);
  return param.un.sleep.ret;
}

int sy_wakeup(sy_thread_id_t id) {
  sy_syscall_param_t param;
  param.un.wakeup.id = id;
  sy_syscall(SY_SYSCALL_TYPE_WAKEUP, &param);
  return param.un.wakeup.ret;
}

sy_thread_id_t sy_getid(void) {
  sy_syscall_param_t param;
  sy_syscall(SY_SYSCALL_TYPE_GETID, &param);
  return param.un.getid.ret;
}

int sy_chpri(int priority) {
  sy_syscall_param_t param;
  param.un.chpri.priority = priority;
  sy_syscall(SY_SYSCALL_TYPE_CHPRI, &param);
  return param.un.chpri.ret;
}

void *sy_kmalloc(int size) {
  sy_syscall_param_t param;
  param.un.kmalloc.size = size;
  sy_syscall(SY_SYSCALL_TYPE_KMALLOC, &param);
  return param.un.kmalloc.ret;
}

int sy_kmfree(void *p) {
  sy_syscall_param_t param;
  param.un.kmfree.p = p;
  sy_syscall(SY_SYSCALL_TYPE_KMFREE, &param);
  return param.un.kmfree.ret;
}

int sy_send(sy_msgbox_id_t id, int size, char *p) {
  sy_syscall_param_t param;
  param.un.send.id = id;
  param.un.send.size = size;
  param.un.send.p = p;
  sy_syscall(SY_SYSCALL_TYPE_SEND, &param);
  return param.un.send.ret;
}

sy_thread_id_t sy_recv(sy_msgbox_id_t id, int *sizep, char **pp) {
  sy_syscall_param_t param;
  param.un.recv.id = id;
  param.un.recv.sizep = sizep;
  param.un.recv.pp = pp;
  sy_syscall(SY_SYSCALL_TYPE_RECV, &param);
  return param.un.recv.ret;
}

int sy_setintr(softvec_type_t type, sy_handler_t handler) {
  sy_syscall_param_t param;
  param.un.setintr.type = type;
  param.un.setintr.handler = handler;
  sy_syscall(SY_SYSCALL_TYPE_SETINTR, &param);
  return param.un.setintr.ret;
}

int kx_wakeup(sy_thread_id_t id) {
  sy_syscall_param_t param;
  param.un.wakeup.id = id;
  sy_srvcall(SY_SYSCALL_TYPE_WAKEUP, &param);
  return param.un.wakeup.ret;
}

void *kx_kmalloc(int size) {
  sy_syscall_param_t param;
  param.un.kmalloc.size = size;
  sy_srvcall(SY_SYSCALL_TYPE_KMALLOC, &param);
  return param.un.kmalloc.ret;
}

int kx_kmfree(void *p) {
  sy_syscall_param_t param;
  param.un.kmfree.p = p;
  sy_srvcall(SY_SYSCALL_TYPE_KMFREE, &param);
  return param.un.kmfree.ret;
}

int kx_send(sy_msgbox_id_t id, int size, char *p) {
  sy_syscall_param_t param;
  param.un.send.id = id;
  param.un.send.size = size;
  param.un.send.p = p;
  sy_srvcall(SY_SYSCALL_TYPE_SEND, &param);
  return param.un.send.ret;
}
