#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "uproc.h"


#ifdef CS333_P3P4
//TODO add to a header?
//#define TICKS_TO_PROMOTE 500
//#define BUDGET 300

struct StateLists{//CHANGE3 The StateLists struct is the lists of all the states held in lists with head and tail pointers
  struct proc* ready[MAXPRIO+1];
  struct proc* readyTail[MAXPRIO+1];
  struct proc* free;
  struct proc* freeTail;
  struct proc* sleep;
  struct proc* sleepTail;
  struct proc* zombie;
  struct proc* zombieTail;
  struct proc* running;
  struct proc* runningTail;
  struct proc* embryo;
  struct proc* embryoTail;
};

void dumpProcLists();
#endif

struct{
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3P4
  struct StateLists pLists; //CHANGE3 This was added to access the state lists from the ptable
  uint PromoteAtTime;
#endif
} ptable;

#ifdef CS333_P3P4
void
procreadyelevate(void);
//void 
//procChangeState(p, STATE){//TODO IS THIS SOMETHING I SHOULD DO?? a helper for changing state lists?
//
//}

#endif
static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

#if defined(CS333_P3P4)
static void procdumpP3P4(struct proc * p, char *state);
#elif defined(CS333_P2)
static void procdumpP2(struct proc * p, char *state);
#elif defined(CS333_P1)
static void procdumpP1(struct proc *p, char *state);
#else
#endif

#ifdef CS333_P2
int getprocs(uint max, struct uproc* table);
#endif

#ifdef CS333_P3P4
static void initProcessLists(void);
static void initFreeList(void);
static int stateListAdd(struct proc** head, struct proc** tail, struct proc* p);
static int stateListRemove(struct proc** head, struct proc** tail, struct proc* p);

#endif


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;
#ifdef CS333_P3P4
  int rc = 0;
#endif

  acquire(&ptable.lock);
#ifdef CS333_P3P4
  p = ptable.pLists.free;
  if (p != 0)
    goto found;
  cprintf("There was not a free process (allocproc)");
#else
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
#endif
  release(&ptable.lock);
  return 0;

found:

#ifdef CS333_P3P4
  if (p->state !=UNUSED){
    panic("allocproc found item on free list that is not free!"); }
  rc = stateListRemove(&ptable.pLists.free, &ptable.pLists.freeTail, p);
  if (rc == -1){
    panic("The process was not removed from free list in allocproc()");
  }
  p->state = EMBRYO;
  if (p->state == EMBRYO){
    stateListAdd(&ptable.pLists.embryo, &ptable.pLists.embryoTail, p);
  }
  else{
    panic("Process not in EMBRYO state allocproc()");
  }
#else
  p->state = EMBRYO;
#endif
  p->pid = nextpid++;
  release(&ptable.lock);
  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    // FIXME: need to remove from the embryo list and add back to the free list.
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

#ifdef CS333_P1
  p->start_ticks = ticks; //Reminder: JENN WROTE THIS
#endif

#ifdef CS333_P2
  p->cpu_ticks_total = 0;          //total elapsed ticks in CPU
  p->cpu_ticks_in = 0;            //ticks when scheduled
#endif
#ifdef CS333_P3P4
  p->budget=BUDGET;
  p->prio = 0;
#endif
  return p;
}


#ifdef CS333_P3P4
void dumpProcLists() {
  cprintf("ready list: %p\n",ptable.pLists.ready);
  cprintf("free list: %p\n",ptable.pLists.free);
  cprintf("embryo list: %p\n",ptable.pLists.embryo);
  cprintf("zombie list: %p\n",ptable.pLists.zombie);
  cprintf("running list: %p\n",ptable.pLists.running);
  cprintf("sleep list: %p\n",ptable.pLists.sleep);
}
#endif

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

#ifdef CS333_P3P4
  acquire(&ptable.lock);//NOTE3 Locked before initializing process lists and free list

  initProcessLists();
  initFreeList(); 
  ptable.PromoteAtTime=TICKS_TO_PROMOTE+ticks;
  release(&ptable.lock);
#endif
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

#ifdef CS333_P2
  p->uid = UID;
  p->gid = GID;
  p->pid = 1;
  p->parent = 0;//TODO need to check this.
#endif
  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

#ifdef CS333_P3P4
  acquire(&ptable.lock);//NOTE3 Locked before initializing process lists and free list
  if (p->state !=EMBRYO){
    panic("userinit has non embryo on embryo list!"); }
  int rc =stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, p);
  if (rc == -1){
    panic("userinit did not remove process from embryo list");
  }
  p->state = RUNNABLE;
  stateListAdd(&ptable.pLists.ready[0], &ptable.pLists.readyTail[0], p);
  
  release(&ptable.lock);
#else
  p->state = RUNNABLE;
#endif
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
#ifdef CS333_P3P4
  int rc;
#endif
  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    // FIXME: np needs to be removed from embryo list and added back to free list.
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
#ifdef CS333_P2
  np->uid = proc->uid;
  np->gid = proc->gid;
#endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
#ifdef CS333_P3P4
  acquire(&ptable.lock);
  if (np->state != EMBRYO){
    panic("the fork function process should have been embryo alas!");
  }
  rc = stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, np);
  np->state = RUNNABLE;
  if (rc == -1){
    panic("not removed from embryo in fork function!");
  }
  stateListAdd(&ptable.pLists.ready[0], &ptable.pLists.readyTail[0], np);
  release(&ptable.lock);
#else
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
#endif
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4//TODO: This was moved to #else stub below after error.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

#else//TODO This was added after error in ready state
void
exit(void)
{
  struct proc *p;
  int fd;
  int rc;
  int i = 0;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);


  for(p=ptable.pLists.ready[i]; i<=MAXPRIO;++i){
  for(; p != 0; p = p->next){//while p!= 0, traverse
    if(p->parent == proc){
      p->parent = initproc;
    }
  }
  }
  for(p = ptable.pLists.sleep; p != 0; p = p->next){//while p!= 0, traverse
    if(p->parent == proc){
      p->parent = initproc;
    }
  }
  for(p = ptable.pLists.zombie; p != 0; p = p->next){//while p!= 0, traverse
    if(p->parent == proc){
      p->parent = initproc;
      wakeup1(initproc);
    }
  }
  for(p = ptable.pLists.embryo; p != 0; p = p->next){//while p!= 0, traverse
    if(p->parent == proc){
      p->parent = initproc;
    }
  }
  for(p = ptable.pLists.running; p != 0; p = p->next){//while p!= 0, traverse
    if(p->parent == proc){
      p->parent = initproc;
    }
  }

 // cprintf("state: %d", proc->state);
  // Jump into the scheduler, never to return.
  if(proc->state != RUNNING){
    panic("exit()NOT IN RUNNING STATE WHILE TRYING TO MAKE A ZOMBIE OMERG!");
  }
  rc = stateListRemove(&ptable.pLists.running, &ptable.pLists.runningTail, proc);
  if (rc == -1){
    panic("Didn't remove from running list in exit!");
  }
  proc->state = ZOMBIE;
  stateListAdd(&ptable.pLists.zombie, &ptable.pLists.zombieTail, proc);
  
  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  int rc;
  int i = 0;
  // set p to head, (check code for walking list);
  acquire(&ptable.lock);
  for(;;){
    havekids = 0;
  for(p=ptable.pLists.ready[i]; i<=MAXPRIO;++i){
    for(; p != 0; p = p->next){//while p!= 0, traverse
      if(p->parent != proc)
        continue;
      havekids = 1;
    }
      //if(p->parent != proc)
      //  continue;
  }
    for(p = ptable.pLists.sleep; p != 0; p = p->next){//while p!= 0, traverse
      if(p->parent != proc)
        continue;
      havekids = 1;
    }
    for(p = ptable.pLists.embryo; p != 0; p = p->next){//while p!= 0, traverse
      if(p->parent != proc)
        continue;
      havekids = 1;
    }
    for(p = ptable.pLists.running; p != 0; p = p->next){//while p!= 0, traverse
      if(p->parent != proc)
        continue;
      havekids = 1;
    }
    for(p = ptable.pLists.zombie; p != 0; p = p->next){//while p!= 0, traverse
      if(p->state != ZOMBIE){
        panic("Non-zombie found on list in wait function!");
      }
      if(p->parent != proc)
        continue;
      havekids = 1;
      // Found one.
      pid = p->pid;
      kfree(p->kstack);
      p->kstack = 0;
      freevm(p->pgdir);
      if (p->state != ZOMBIE){
        panic("The item on the zombie list is not a zombie in wait"); }
      rc = stateListRemove(&ptable.pLists.zombie, &ptable.pLists.zombieTail, p);
      p->state = UNUSED;
      if (rc == -1){
        panic("not removed from zombie in wait function!");
      }
      stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p);
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      release(&ptable.lock);
      return pid;
    }
    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }

}
#endif

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
#ifdef CS333_P2
      p->cpu_ticks_in= ticks;
#endif
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

#else
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle
  int i;
  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    if (ticks >= ptable.PromoteAtTime){
      procreadyelevate();
      ptable.PromoteAtTime = ticks+TICKS_TO_PROMOTE;
    }
    for (i = 0; i<=MAXPRIO; ++i){
    p =ptable.pLists.ready[i]; 
    if (p){
      // cprintf("scheduling %d %d %d %d\n",p->pid, i, p->budget, p->prio);
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      if (p->state!=RUNNABLE){
        panic("The process is not 'ready' and is in the ready list in scheduler!");
      }
      int rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p);
      if (rc == -1){//TODO make sure in correct state
        dumpProcLists();
        cprintf("pid %d\n",p->pid);
        panic("the process was not removed from ready in the scheduler");
      }
      if (p->prio != i){
        panic("NOT ON RIGHT PRIO IN SCHEDULER!");
      }
      p->state = RUNNING;
      stateListAdd(&ptable.pLists.running, &ptable.pLists.runningTail, p);
#ifdef CS333_P2
      p->cpu_ticks_in= ticks;
#endif
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
      break;
      }
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;
#ifdef CS333_P2
  //struct proc *p;//i know this is wrong mkay
#endif

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
#ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
#endif
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
#ifdef CS333_P3P4
  int rc = 0;
  acquire(&ptable.lock);  //DOC: yieldlock
  if (proc->state != RUNNING){
    panic("non running process on running list in yield");
  }
  rc = stateListRemove(&ptable.pLists.running, &ptable.pLists.runningTail, proc);
  if (rc == -1){
    panic("running process not removed from list!");
  }
  proc->budget -= ticks - proc->cpu_ticks_in;
  if(proc->budget <=0){
  proc->budget = BUDGET;
  if (proc->prio < MAXPRIO)  {
     proc->prio++;
     }
  }
  stateListAdd(&ptable.pLists.ready[proc->prio], &ptable.pLists.readyTail[proc->prio], proc);
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
#else
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
#endif
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
#ifdef CS333_P3P4
  int rc = 0;
#endif
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
#ifdef CS333_P3P4//TODO: This is new since the error :(
  if(proc->state != RUNNING){
    panic("The process should have been running and is not in sleep function");
  }
  rc = stateListRemove(&ptable.pLists.running, &ptable.pLists.runningTail, proc);//TODO is this right or proc needs to be defined?
  if (rc == -1){
    panic("The item was not removed from running list in sleep function");
  }
  proc->chan = chan;
  proc->budget -= ticks - proc->cpu_ticks_in;
  if(proc->budget <=0){
  proc->budget = BUDGET;
  if (proc->prio < MAXPRIO)  {
     proc->prio++;
     }
  }
  proc->state = SLEEPING;
  stateListAdd(&ptable.pLists.sleep, &ptable.pLists.sleepTail, proc);
#else
  proc->chan = chan;
  proc->state = SLEEPING;
#endif
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

//PAGEBREAK!
#ifndef CS333_P3P4
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#else
static void
wakeup1(void *chan)
{
  struct proc *p, *next;
  int rc = 0;
  //TODO Do I need to wake up all the processes or just the first one?
  p = ptable.pLists.sleep;
  while(p != 0){
    next = p->next;

    if(p->chan == chan){
      if (p->state != SLEEPING){
        panic("non sleepy process in sleep list!!");
      }
      rc = stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleepTail, p);
      if (rc == -1){
        panic("sleepy process not removed from list!");
      }
      rc = stateListAdd(&ptable.pLists.ready[p->prio], &ptable.pLists.readyTail[p->prio], p);
      p->state = RUNNABLE;
      if (rc == -1){
        rc = stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleepTail, p);
        p->state = SLEEPING;
        cprintf("so..the process you wanted to add to ready wouldn't add so going to put it on the sleep process again..");
        if (rc == -1){
          panic("sleepy process was removed but not added to ready process and not put back on sleep list. it's getting real!");
        }
      }
      //p->state = RUNNABLE;
    }
    p = next;
  }
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

#ifdef CS333_P3P4
int
setpriority(int pid, int prio){
  struct proc *p;
  int rc;          //return code
  int i = 0;      //array start
    
  acquire(&ptable.lock);
  if (prio > MAXPRIO){
    return -1;
  }
  if (pid <0 || pid >32767){
    return -1;
  }
  for(p=ptable.pLists.ready[i]; i<=MAXPRIO;++i){
  for(; p != 0; p = p->next){//while p!= 0, traverse
    if(p->pid == pid){
      p->budget = BUDGET;
      if(p->prio != prio){
        p->prio = prio;
        rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p);
        if (rc !=0){
          panic("didn't remove from ready list in set priority");
        }
        stateListAdd(&ptable.pLists.ready[p->prio], &ptable.pLists.readyTail[p->prio], p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  }
  for(p = ptable.pLists.running; p != 0; p = p->next){//while p!= 0, traverse
    if(p->pid == pid){
      p->prio = prio;
      p->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
  }

  for(p = ptable.pLists.sleep; p != 0; p = p->next){//while p!= 0, traverse
    if(p->pid == pid){
      p->prio = prio;
      p->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#endif
// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#else
int
kill(int pid)//TODO NEED TO GO THROUGH THIS AGAIN, only checked the sleep process and moved to running
{
  struct proc *p;
  int rc;          //return code
  int i = 0;      //array start
  acquire(&ptable.lock);
  for(p = ptable.pLists.embryo; p != 0; p = p->next){//while p!= 0, traverse
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }
  for(i=0;i<=MAXPRIO;i++){
    p=ptable.pLists.ready[i];
    while(p){
      if(p->pid == pid){
        p->killed = 1;
        release(&ptable.lock);
        return 0;
      }
      p = p->next;
    }
  }
  for(p = ptable.pLists.zombie; p != 0; p = p->next){//while p!= 0, traverse
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }
  for(p = ptable.pLists.running; p != 0; p = p->next){//while p!= 0, traverse
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }

  for(p = ptable.pLists.sleep; p != 0; p = p->next){//while p!= 0, traverse
    if (p->state != SLEEPING){
      panic("non sleepy process in kill function but on sleep list!!");
    }
    if(p->pid == pid){
      p->killed = 1;
      rc = stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleepTail, p);
      if (rc == -1){
        panic("sleepy process not removed from list!");
      }
      rc = stateListAdd(&ptable.pLists.ready[p->prio], &ptable.pLists.readyTail[p->prio], p);//set to zombie?
      p->state = RUNNABLE;
      if (rc == -1){
        rc = stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleepTail, p);
        cprintf("so..the process you wanted to add to ready wouldn't add so going to put it on the sleep process again..");
        p->state = SLEEPING;
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)    //This was copied from Mark Morrissey's email
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

#if defined(CS333_P3P4)
//#define HEADER "\nPID\tName\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\t PCs\n"
#define HEADER "\nPID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\t PCs\n"
#elif defined(CS333_P2)
#define HEADER "\nPID\tName         UID\tGID\tPPID\tElapsed\tCPU\tState\tSize\t PCs\n"
#elif defined(CS333_P1)
#define HEADER "\nPID\tName         Elapsed\tState\tSize\t PCs\n"
#else
#define HEADER ""
#endif

  cprintf(HEADER);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#if defined(CS333_P3P4)
    procdumpP3P4(p, state);
#elif defined(CS333_P2)
    procdumpP2(p, state);
#elif defined(CS333_P1)
    procdumpP1(p, state);
#else
    cprintf("%d %s %s", p->pid, state, p->name);
#endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

#if defined(CS333_P3P4)
static void
procdumpP3P4(struct proc * p, char *state){//from Mark Morrisey's code 
  int total;
  int ms; //milliseconds
  int s;  //seconds
  total = ticks - p->start_ticks;
  s = total / 1000;
  ms = total % 1000;
  cprintf("%d\t%s\t%d\t%d\t%d\t%d\t", 
      p->pid, 
      p->name, 
      p->uid, 
      p->gid, 
      p->parent ? p->parent->pid : p->pid,
      p->prio);
  if (ms < 10){
    cprintf("%d.00%d\t", s, ms);
  }
  else if (ms < 100){
    cprintf("%d.0%d\t", s, ms);
  }
  else{
    cprintf("%d.%d\t", s, ms);
  }
  //CPU ticks total
  total = p->cpu_ticks_total;
  s = total / 1000;
  ms = total % 1000;
  if (ms < 10){
    cprintf("%d.00%d\t", s, ms);
  }
  else if (ms < 100){
    cprintf("%d.0%d\t", s, ms);
  }
  else{
    cprintf("%d.%d\t", s, ms);
  }
  cprintf("%s\t", state);
  cprintf("%d\t", p->sz);
}


#elif defined(CS333_P2)
static void
procdumpP2(struct proc * p, char *state){ 
  int total;
  int ms; //milliseconds
  int s;  //seconds
  cprintf("%d\t%s\t\t%d\t%d\t", p->pid, p->name, p->uid, p->gid);
  cprintf("%d\t", p->parent->pid);
  total = ticks - p->start_ticks;
  s = total / 1000;
  ms = total % 1000;
  if (ms < 10){
    cprintf("%d.00%d\t", s, ms);
  }
  else if (ms < 100){
    cprintf("%d.0%d\t", s, ms);
  }
  else{
    cprintf("%d.%d\t", s, ms);
  }
  //CPU ticks total
  total = p-> cpu_ticks_total;
  s = total / 1000;
  ms = total % 1000;
  if (ms < 10){
    cprintf("%d.00%d\t", s, ms);
  }
  else if (ms < 100){
    cprintf("%d.0%d\t", s, ms);
  }
  else{
    cprintf("%d.%d\t", s, ms);
  }
  cprintf("%s\t", state);
  cprintf("%d\t", p->sz);
}

#elif defined(CS333_P1)
static void
procdumpP1(struct proc * p, char *state){
  //int i;
  //struct proc *p;
  //char *state;
  //uint pc[10];

  int total;
  int ms; //milliseconds
  int s;  //seconds
  cprintf("PID\tState\tName\tElapsed\tPCs\n");

  state = states[p->state];
  cprintf("%d\t%s\t\t%s", p->pid, state, p->name);
  total = ticks - p-> start_ticks;
  s = total / 1000;
  ms = total % 1000;
  cprintf("\t%d.%d", s, ms);
}
//    procdumpP1(p, state);

#else
//cprintf("didn't hit any procdump functions \n");
#endif

#ifdef CS333_P2
int 
getprocs(uint max, struct uproc* table){
  struct proc *p;
  int i = 0;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED || p->state == EMBRYO)
      continue;

    table[i].pid = p->pid;
    table[i].uid = p->uid;
    table[i].gid = p->gid;
    table[i].ppid = (p->parent) ? p->parent->pid : p->pid;  // p->parent->pid;
    table[i].elapsed_ticks = ticks - (p->start_ticks);
    table[i].CPU_total_ticks = p->cpu_ticks_total;
    //cprintf("total ticks %d\n", table[i].CPU_total_ticks); 
    safestrcpy(table[i].state, states[p->state], STRMAX);
    table[i].size = p->sz;
    safestrcpy(table[i].name, p->name, STRMAX);
    i++;
    if (i >= max){
      break;
    }
  }
  release(&ptable.lock);
  return i;
}
#endif


#ifdef CS333_P3P4
static int
stateListAdd(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0) {
    *head = p;
    *tail = p;
    p->next = 0;
  } else {
    (*tail)->next = p;
    *tail = (*tail)->next;
    (*tail)->next = 0;
  }

  return 0;
}

static int
stateListRemove(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0 || *tail == 0 || p == 0) {
    return -1;
  }

  struct proc* current = *head;
  struct proc* previous = 0;

  if (current == p) {
    *head = (*head)->next;
    return 0;
  }

  while(current) {
    if (current == p) {
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found, hit eject.
  if (current == 0) {
    return -1;
  }

  // Process found. Set the appropriate next pointer.
  if (current == *tail) {
    *tail = previous;
    (*tail)->next = 0;
  } else {
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = 0;

  return 0;
}

static void
initProcessLists(void) {
  int i;
  for(i = 0; i<=MAXPRIO;++i){
  ptable.pLists.ready[i] = 0;
  ptable.pLists.readyTail[i] = 0;
  }
  ptable.pLists.free = 0;
  ptable.pLists.freeTail = 0;
  ptable.pLists.sleep = 0;
  ptable.pLists.sleepTail = 0;
  ptable.pLists.zombie = 0;
  ptable.pLists.zombieTail = 0;
  ptable.pLists.running = 0;
  ptable.pLists.runningTail = 0;
  ptable.pLists.embryo = 0;
  ptable.pLists.embryoTail = 0;
}

static void
initFreeList(void) {
  if (!holding(&ptable.lock)) {
    panic("acquire the ptable lock before calling initFreeList\n");
  }

  struct proc* p;

  for (p = ptable.proc; p < ptable.proc + NPROC; ++p) {
    p->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p);
  }
}
#endif
#ifdef CS333_P3P4
void            
procreadyelevate(void){
  struct proc *p;
  struct proc *temp;
  int i;
  for (i = 0; i<=MAXPRIO; ++i){
  //cprintf("%d: ",i);
    p =ptable.pLists.ready[i]; 
    while(p!= 0){
      //cprintf(" -> %u ", p->pid);
     // cprintf(" %d, %d %s ", p->pid, p->budget, (p->next) ? "->" : "");
      temp = p->next;
      p->budget = BUDGET;
      if(i>0){
        p->prio--;
        stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p);//TODO write some check statememtn bla blahs
        stateListAdd(&ptable.pLists.ready[p->prio], &ptable.pLists.readyTail[p->prio], p);
      }
      p = temp;
    }
  }
  p=ptable.pLists.sleep;
  while(p!=0){
    if(p->prio > 0){
    p->prio--;
    }
    p->budget = BUDGET;
    p = p->next;
  }
  p=ptable.pLists.running;
  while(p!=0){
    if(p->prio > 0){
    p->prio--;
    }
    p->budget = BUDGET;
    p = p->next;
  }
}

void            
procreadydump(void){
  struct proc *p;
  int i;
  cprintf("Ready List Processes\n");
  //if (p!=0){
  //cprintf("%u", p->pid);
  //}
  for (i = 0; i<=MAXPRIO; ++i){
  cprintf("%d: ",i);
    p =ptable.pLists.ready[i]; 
  while(p!= 0){
    //cprintf(" -> %u ", p->pid);
    //cprintf(" %d, %d %s ", p->pid, p->budget, (p->next) ? "->" : "");
    cprintf(" (%d, %d) %s ", p->pid, p->budget, (p->next) ? "->" : "");
    p = p->next;
  }
  cprintf("\n");
  }
}

void            
procfreedump(void){
  struct proc *p = ptable.pLists.free;
  int count = 0;
  while (p != 0){
    ++count;
    p = p->next;
  }
  cprintf("Free List Size: %d processess\n", count);
}

void            
procsleepdump(void){
  struct proc *p = ptable.pLists.sleep;
  cprintf("Sleep List Processes\n");
  while(p!= 0){
    cprintf(" %d %s ", p->pid, (p->next) ? "->" : "");
    p = p->next;
  }
}

void            
proczombiedump(void){
  struct proc *p = ptable.pLists.zombie;
  cprintf("Zombie List Processes\n");
  while(p!= 0){
    cprintf(" (%d, %d) %s ", p->pid, (p->parent) ? p->parent->pid : 0, (p->next) ? "->" : "");
    p = p->next;
  }
}
#endif
