#ifndef _SAYO_OS_SYSCALL_H_INCLUDED_
#define _SAYO_OS_SYSCALL_H_INCLUDED_

#include "defines.h"
#include "interrupt.h"

typedef enum {
  SY_SYSCALL_TYPE_RUN = 0,
  SY_SYSCALL_TYPE_EXIT,
  SY_SYSCALL_TYPE_WAIT,
  SY_SYSCALL_TYPE_SLEEP,
  SY_SYSCALL_TYPE_WAKEUP,
  SY_SYSCALL_TYPE_GETID,
  SY_SYSCALL_TYPE_CHPRI,
  SY_SYSCALL_TYPE_KMALLOC,
  SY_SYSCALL_TYPE_KMFREE,
  SY_SYSCALL_TYPE_SEND,
  SY_SYSCALL_TYPE_RECV,
  SY_SYSCALL_TYPE_SETINTR,
} sy_syscall_type_t;

typedef struct {
  union {
    struct {
      sy_func_t func;
      char *name;
      int priority;
      int stacksize;
      int argc;
      char **argv;
      sy_thread_id_t ret;
    } run;
    struct {
      int dummy;
    } exit;
    struct {
      int ret;
    } wait;
    struct {
      int ret;
    } sleep;
    struct {
      sy_thread_id_t id;
      int ret;
    } wakeup;
    struct {
      sy_thread_id_t ret;
    } getid;
    struct {
      int priority;
      int ret;
    } chpri;
    struct {
      int size;
      void *ret;
    } kmalloc;
    struct {
      char *p;
      int ret;
    } kmfree;
    struct {
      sy_msgbox_id_t id;
      int size;
      char *p;
      int ret;
    } send;
    struct {
      sy_msgbox_id_t id;
      int *sizep;
      char **pp;
      sy_thread_id_t ret;
    } recv;
    struct {
      softvec_type_t type;
      sy_handler_t handler;
      int ret;
    } setintr;
  } un;
} sy_syscall_param_t;

#endif
