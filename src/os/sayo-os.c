#include "defines.h"
#include "sayo-os.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "memory.h"
#include "lib.h"

#define THREAD_NUM 6
#define PRIORITY_NUM 16
#define THREAD_NAME_SIZE 15

typedef struct _sy_context {
  uint32 sp; /* Stack pointer */
} sy_context;

/* task control block */
typedef struct _sy_thread {
  struct _sy_thread *next;
  char name[THREAD_NAME_SIZE + 1];
  int priority;
  char *stack;
  uint32 flags;
#define SY_THREAD_FLAG_READY (1 << 0)

  struct {
    sy_func_t func;
    int argc;
    char **argv;
  } init;

  struct {
    sy_syscall_type_t type;
    sy_syscall_param_t *param;
  } syscall;

  sy_context context;
} sy_thread;

/* message buffer */
typedef struct _sy_msgbuf {
  struct _sy_msgbuf *next;
  sy_thread *sender;
  struct {
    int size;
    char *p;
  } param;
} sy_msgbuf;

/* message box */
typedef struct _sy_msgbox {
  sy_thread *receiver;
  sy_msgbuf *head;
  sy_msgbuf *tail;

  long dummy[1];
} sy_msgbox;

/* thread ready-queue */
static struct {
  sy_thread *head;
  sy_thread *tail;
} readyque[PRIORITY_NUM];

static sy_thread *current;
static sy_thread threads[THREAD_NUM];
static sy_handler_t handlers[SOFTVEC_TYPE_NUM];
static sy_msgbox msgboxes[MSGBOX_ID_NUM];

void dispatch(sy_context *context);

static int getcurrent(void) {
  if(current == NULL) {
    return -1;
  }
  if(!(current->flags & SY_THREAD_FLAG_READY)) {
    return 1;
  }

  readyque[current->priority].head = current->next;
  if(readyque[current->priority].head == NULL) {
    readyque[current->priority].tail = NULL;
  }
  current->flags &= ~SY_THREAD_FLAG_READY;
  current->next = NULL;

  return 0;
}

static int putcurrent(void) {
  if(current == NULL) {
    return -1;
  }
  if(current->flags & SY_THREAD_FLAG_READY) {
    return 1;
  }

  if(readyque[current->priority].tail) {
    readyque[current->priority].tail->next = current;
  } else {
    readyque[current->priority].head = current;
  }
  readyque[current->priority].tail = current;
  current->flags |= SY_THREAD_FLAG_READY;

  return 0;
}

static void thread_end(void) {
  sy_exit();
}

/* thread startup */
static void thread_init(sy_thread *thp) {
  /* call thread's main function */
  thp->init.func(thp->init.argc, thp->init.argv);
  thread_end();
}

/* call systemcall */
static sy_thread_id_t thread_run(sy_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
  int i;
  sy_thread *thp;
  uint32 *sp;
  extern char userstack;
  static char *thread_stack = &userstack;

  /* find vacant task-control-block */
  for(i=0; i<THREAD_NUM; i++) {
    thp = &threads[i];
    if(!thp->init.func)
      break;
  }
  if(i == THREAD_NUM)
    return -1;
  
  memset(thp, 0, sizeof(*thp));

  /* tcb setup */
  strcpy(thp->name, name);
  thp->next     = NULL;
  thp->priority = priority;
  thp->flags    = 0;

  thp->init.func = func;
  thp->init.argc = argc;
  thp->init.argv = argv;

  /* get stack area */
  memset(thread_stack, 0, stacksize);
  thread_stack += stacksize;

  thp->stack = thread_stack;

  /* initialize stack */
  sp = (uint32 *)thp->stack;
  *(--sp) = (uint32)thread_end;
  /* setup program-counter
   * If priority is 0, non-interruptible thread. */
  *(--sp) = (uint32)thread_init | ((uint32)(priority ? 0 : 0xc0) << 24);

  *(--sp) = 0; /* ER6 */
  *(--sp) = 0; /* ER5 */
  *(--sp) = 0; /* ER4 */
  *(--sp) = 0; /* ER3 */
  *(--sp) = 0; /* ER2 */
  *(--sp) = 0; /* ER1 */

  /* prepare startup arg */
  *(--sp) = (uint32)thp; /* ER0 */

  /* setup thread context */
  thp->context.sp = (uint32)sp;

  /* put back thread that called system-call to ready-queue */
  putcurrent();

  /* put created thread to ready-queue */
  current = thp;
  putcurrent();

  return (sy_thread_id_t)current;
}

static int thread_exit(void) {
  puts(current->name);
  puts(" EXIT.\n");
  memset(current, 0, sizeof(*current));
  return 0;
}

static int thread_wait(void) {
  putcurrent();
  return 0;
}

static int thread_sleep(void) {
  return 0;
}

static int thread_wakeup(sy_thread_id_t id) {
  putcurrent();

  current = (sy_thread*)id;
  putcurrent();

  return 0;
}

static sy_thread_id_t thread_getid(void) {
  putcurrent();
  return (sy_thread_id_t)current;
}

static int thread_chpri(int priority) {
  int old = current->priority;
  if(priority >= 0)
    current->priority = priority;
  putcurrent();
  return old;
}

static void *thread_kmalloc(int size) {
  putcurrent();
  return symem_alloc(size);
}

static int thread_kmfree(void *p) {
  symem_free(p);
  putcurrent();
  return 0;
}

static void sendmsg(sy_msgbox *mboxp, sy_thread *thp, int size, char *p) {
  sy_msgbuf *mp;

  mp = (sy_msgbuf *)symem_alloc(sizeof(*mp));
  if(mp == NULL)
    sy_sysdown();
  mp->next        = NULL;
  mp->sender      = thp;
  mp->param.size  = size;
  mp->param.p     = p;

  if(mboxp->tail) {
    mboxp->tail->next = mp;
  } else {
    mboxp->head = mp;
  }
  mboxp->tail = mp;
}

static void recvmsg(sy_msgbox *mboxp) {
  sy_msgbuf *mp;
  sy_syscall_param_t *p;

  mp = mboxp->head;
  mboxp->head = mp->next;
  if(mboxp->head == NULL)
    mboxp->tail = NULL;
  mp->next = NULL;

  p = mboxp->receiver->syscall.param;
  p->un.recv.ret = (sy_thread_id_t)mp->sender;
  if(p->un.recv.sizep)
    *(p->un.recv.sizep) = mp->param.size;
  if(p->un.recv.pp)
    *(p->un.recv.pp) = mp->param.p;

  mboxp->receiver = NULL;

  symem_free(mp);
}

static int thread_send(sy_msgbox_id_t id, int size, char *p) {
  sy_msgbox *mboxp = &msgboxes[id];

  putcurrent();
  sendmsg(mboxp, current, size, p);

  if(mboxp->receiver) {
    current = mboxp->receiver;
    recvmsg(mboxp);
    putcurrent();
  }

  return size;
}

static int thread_recv(sy_msgbox_id_t id, int *sizep, char **pp) {
  sy_msgbox *mboxp = &msgboxes[id];

  if(mboxp->receiver)
    sy_sysdown();

  mboxp->receiver = current;

  if(mboxp->head == NULL)
    return -1;

  recvmsg(mboxp);
  putcurrent();

  return current->syscall.param->un.recv.ret;

}

/* regist interrupt handler */
static int thread_setintr(softvec_type_t type, sy_handler_t handler) {
  static void thread_intr(softvec_type_t type, unsigned long sp);

  softvec_setintr(type, thread_intr);

  handlers[type] = handler;
  putcurrent();

  return 0;
}

static void call_functions(sy_syscall_type_t type, sy_syscall_param_t *p) {
  switch(type) {
    case SY_SYSCALL_TYPE_RUN: /* sy_run() */
      p->un.run.ret = thread_run(p->un.run.func,
                                 p->un.run.name,
                                 p->un.run.priority,
                                 p->un.run.stacksize,
                                 p->un.run.argc,
                                 p->un.run.argv);
      break;
    case SY_SYSCALL_TYPE_EXIT:
      /* tcb will be cleared, do not write return value */
      thread_exit();
      break;
    case SY_SYSCALL_TYPE_WAIT:
      p->un.wait.ret = thread_wait();
      break;
    case SY_SYSCALL_TYPE_SLEEP:
      p->un.sleep.ret = thread_sleep();
      break;
    case SY_SYSCALL_TYPE_WAKEUP:
      p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
      break;
    case SY_SYSCALL_TYPE_GETID:
      p->un.getid.ret = thread_getid();
      break;
    case SY_SYSCALL_TYPE_CHPRI:
      p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
      break;
    case SY_SYSCALL_TYPE_KMALLOC:
      p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
      break;
    case SY_SYSCALL_TYPE_KMFREE:
      p->un.kmfree.ret = thread_kmfree(p->un.kmfree.p);
      break;
    case SY_SYSCALL_TYPE_SEND:
      p->un.send.ret = thread_send(p->un.send.id, p->un.send.size, p->un.send.p);
      break;
    case SY_SYSCALL_TYPE_RECV:
      p->un.recv.ret = thread_recv(p->un.recv.id, p->un.recv.sizep, p->un.recv.pp);
      break;
    case SY_SYSCALL_TYPE_SETINTR:
      p->un.setintr.ret = thread_setintr(p->un.setintr.type, p->un.setintr.handler);
      break;
    default:
      break;
  }
}

/* process system-call */
static void syscall_proc(sy_syscall_type_t type, sy_syscall_param_t *p) {
  getcurrent();
  call_functions(type, p);
}

/* process service-call */
static void srvcall_proc(sy_syscall_type_t type, sy_syscall_param_t *p) {
  current = NULL;
  call_functions(type, p);
}

static void schedule(void) {
  int i;

  for(i=0; i<PRIORITY_NUM; i++) {
    if(readyque[i].head)
      break;
  }
  if(i == PRIORITY_NUM)
    sy_sysdown();

  current = readyque[i].head;
}

static void syscall_intr(void) {
  syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void) {
  puts(current->name);
  puts(" DOWN.\n");
  getcurrent();
  thread_exit();
}

static void thread_intr(softvec_type_t type, unsigned long sp) {
  /* preserve current thread context */
  current->context.sp = sp;
  
  if(handlers[type])
    handlers[type]();

  schedule();

  dispatch(&current->context);
}

void sy_start(sy_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
  symem_init();

  current = NULL;

  memset(readyque, 0, sizeof(readyque));
  memset(threads,  0, sizeof(threads));
  memset(handlers, 0, sizeof(handlers));
  memset(msgboxes, 0, sizeof(msgboxes));

  /* regist intrrupt handler */
  thread_setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
  thread_setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

  /* can't use system-call, so create thread directly */
  current = (sy_thread *)thread_run(func, name, priority, stacksize, argc, argv);

  dispatch(&current->context);
}

void sy_sysdown(void) {
  puts("system error!\n");
  while(1);
}

void sy_syscall(sy_syscall_type_t type, sy_syscall_param_t *param) {
  current->syscall.type  = type;
  current->syscall.param = param;
  asm volatile ("trapa #0");
}

void sy_srvcall(sy_syscall_type_t type, sy_syscall_param_t *param) {
  srvcall_proc(type, param);
}
















