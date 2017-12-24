#ifndef PROCESS_H
#define PROCESS_H

#include <pthread.h>
#include "util/list.h"
#include "util/timer.h"
#include "emu/cpu.h"
#include "kernel/fs.h"
#include "kernel/signal.h"

struct process {
    struct cpu_state cpu; // do not access this field except on the current process
    pthread_t thread;

    dword_t pid, ppid;
    dword_t uid, gid;
    dword_t euid, egid;

    addr_t vdso;
    addr_t start_brk;
    addr_t brk;

    struct fd *pwd;
    struct fd *root;
    struct fd *files[MAX_FD];
    mode_t_ umask;

    struct sigaction_ sigactions[NUM_SIGS];
    sigset_t_ blocked;
    sigset_t_ queued; // where blocked signals go when they're sent
    sigset_t_ pending;
    lock_t signal_lock;

    struct process *parent;
    struct list children;
    struct list siblings;

    dword_t sid, pgid;
    struct list session;
    struct list group;
    struct tty *tty;

    bool has_timer;
    struct timer *timer;

    // the next two fields are protected by the lock on the parent process, not
    // the lock on the process. this is because waitpid locks the parent
    // process to wait for any of its children to exit.
    dword_t exit_code;
    bool zombie;
    pthread_cond_t child_exit;
    lock_t exit_lock;

    pthread_cond_t vfork_done;
};

// current will always give the process that is currently executing
// if I have to stop using __thread, current will become a macro
extern __thread struct process *current;

// Creates a new process, returns NULL in case of failure
struct process *process_create(void);
// Removes the process from the process table and frees it.
void process_destroy(struct process *proc);

struct pid {
    dword_t id;
    struct process *proc;
    struct list session;
    struct list group;
};

// these functions must be called with pids_lock
struct pid *pid_get(dword_t pid);
struct process *pid_get_proc(dword_t pid);
extern lock_t pids_lock;

#define MAX_PID (1 << 10) // oughta be enough

// When a thread is created to run a new process, this function is used.
extern void (*process_run_hook)(void);
// TODO document
void process_start(struct process *proc);

extern void (*exit_hook)(int code);

#endif
