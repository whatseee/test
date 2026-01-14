#ifndef _Thread
#define _Thread

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

#ifndef MAX_SCPS_SOCKET

#define REMOVE_LATER 1
#ifdef GATEWAY_SELECT
#ifdef GATEWAY_LARGER 
#define MAX_SCPS_SOCKET   512	/* must be same size of (scps_fd_set) */
#else /* GATEWAY_LARGER  */
#define MAX_SCPS_SOCKET   256	/* must be same size of (scps_fd_set) */
#endif /* GATEWAY_LARGER  */
#else /* GATEWAY_SELECT */
#define MAX_SCPS_SOCKET         32	/* based on sizeof (int) */
#endif /* GATEWAY_SELECT */

#endif /* MAX_SCPS_SOCKET */

/* Must be divisible by 8 for a Sparc! */
#ifdef GATEWAY_LARGER 
#define THREAD_SWAP_STACK_SIZE   (131072*2)
#else /* GATEWAY_LARGER  */
#define THREAD_SWAP_STACK_SIZE   131072
#endif /* GATEWAY_LARGER  */

/* This is for the embedded test program, it can go away */

#define MAX_GLOBAL_ITERATION     100

/* These are the valid states for a thread/process */

typedef enum
  {
    Created,			/* Thread was just created, but never run      */
    Ready,			/* Ready to run, but currently not running     */
    Running,			/* Currently running process                   */
    Blocked,			/* Waiting on some resource                    */
    Preempted,			/* Was running, preempted by something else    */
    Sleep,			/* Processes is sleeping                       */
    Defunct			/* For cswap routines...                       */
  }
Th_Status;

struct sock_ent
  {
    caddr_t ptr;		/* pointer to the socket      */
    unsigned int read;		/* Amount needed to unblock read  */
    unsigned int write;		/* Amount needed to unblock write */
  };

struct threads
  {
    int pid;			/* Unique process-Id             */
    int SCPS_errno;		/* Our own private Errno         */
    caddr_t stack;		/* Private stack per thread      */
    size_t stack_size;		/* Size of thread's stack (max)  */
    struct threads *next_thread;
    struct threads *prev_thread;
    Th_Status status;		/* Runable status of this thread        */
#ifdef GATEWAY 
    void *read_socks;		/* Sockets available for reading        */
    void *write_socks;		/* Sockets available for writing        */
#else /* GATEWAY  */
    unsigned int read_socks;	/* Sockets available for reading        */
    unsigned int write_socks;	/* Sockets available for writing        */
#endif /* GATEWAY  */
    uint32_t last_run;	/* CPU time used for previous execution */
    uint32_t last_wait;	/* CPU spent waiting since previous run */
    uint32_t switch_time;	/* When last context switch occurred    */
    uint32_t avg_run;	/* Average run time of thread           */
    uint32_t avg_wait;	/* Average wait time of thread          */
    uint32_t max_run;	/* Maximum run time of thread           */
    uint32_t max_wait;	/* Maximum run time of thread           */
    void (*function) (void);	/* Routine that this thread represents  */
  };

#define TIMER_VALID 0x1
#define TIMER_RESCHEDULE 0x2
#define TIMER_CLEAR 0x3

struct _timer
{
    struct _timer *prev;			/* Previous timer in list              */
    struct _timer *next;			/* Next timer in list                  */
    struct _timer *next_tobe_sched;	/* Timers waiting scheduling/clearing  */
    int immediate;					/* 1 if needs immediate service        */
    int expired;					/* 1 if timer expired, 0 if not.       */
    struct timeval expiration;		/* Expiration time in absolute ticks   */
    uint32_t ticks;					/* Same as expiration, but in ticks    */
    int spins;						/* Number of spins to take in queue    */
    int index;						/* Index in timer_wheel                */
    int set;						/* Binary indication whether timer set */
    int flags;
    void (*function) (void *);		/* Handler to be called for this timer */
    void *socket;					/* Socket "owning" this timer          */
    struct _timer_queue *queue;		/* Queue timer is occupying            */
    int type;						/*PDF XXX XXX */
};

struct _timer_queue
{
    int reset;
    struct _timer *head;
    struct _timer *tail;
};

struct _scheduler
  {
    int num_procs;		/* number procs currently in system   */
    int num_runable;		/* number of unblocked threads */
    int last_alloc;		/* pid of most recent entry           */
    int stack_growth;		/* Direction of stack growth          */
    int run_index;		/* Current Process Index in run-queue */
    int tp_ephemeral_next;	/* Next abitrary tp port to use    */
    int udp_ephemeral_next;	/* Next arbitrary udp port to use  */
    int service_interface_now;
    int interface_data;
    void *interface;
    struct sock_ent sockets[MAX_SCPS_SOCKET];	/* Pointers to allocated sockets      */
	 int num_of_socket;
    uint32_t switch_time;	/* Start time of current process      */
//    struct threads *run_queue[MAX_SCPS_SOCKET];	/* The hard-coded execution order     */
    struct threads *head,	/* head of process list               */
     *tail,			/* tail of process list               */
     *current;			/* Currently executing process        */
    struct
      {
	struct _timer_queue queue;
	struct _timer_queue expired_queue;
	struct _timer_queue to_be_sched;
      }
    timers;
    struct threads *run_queue[MAX_SCPS_SOCKET];	/* The hard-coded execution order     */
  };

#ifndef THREAD_C
extern struct _scheduler scheduler;
#endif /* THREAD_C*/

extern sigset_t alarmset;

extern struct threads dummyStartupThread;


/* 
 * Introduction of the _timer structure will change the way timers are 
 * handled by TP - A socket still maintains an array of timers, but now
 * they are of type struct _timer, the timer-maintenance routine will
 * handle the manipulation of the *prev and *next fields.
 * We need a few new functions however:
 * 
 * struct _timer *create_timer(void *handler_fcn, void *socket, 
 *               uint32_t expiration, struct _timer *timer_element)
 *    Create's a new timer element if timer_element is a NULL and 
 *    initializes it regardless. If the expiration time is not 0, it
 *    will be initialized and placed in the global timer queue.
 * 
 * int  delete_timer(struct _timer *timer_element)
 *    Delete's a timer, freeing it's allocated memory.
 * 
 * uint32_t set_timer(uint32_t expiration, 
 *                         struct _timer *timer_element)   
 *    Set a timer and place it in the global timer queue; 
 *    The expiration time should be the relative expiration 
 *    time (time from now), the return value is the absolute 
 *    expiration time scheduled (0 on failure)
 *
 * int   clear_timer(struct _timer *timer_element)
 *    Clears a timer; If the timer is the next pending timeout, reset 
 *    the Alarm timer for the next timer in the queue.
 *
 * void  Timer_Handler - handle the SIGALRM signal that does our timer
 *                       magic for us; Handle the timer that triggered 
 *                       the interrupt PLUS any other currently expired
 *                       timers, then reset the Alarm timer for the next
 *                       timer.      
 *
 */
/* Proper prototypes */
/* Scheduler Related */
void init_scheduler (void);
void setup_thread_globals (void);
void save_thread_globals (void);
void sched (void);
void start_threads (void);
struct threads *create_thread (void (*program) (void),...);
void chg_block_prg (void (*program) (void), Th_Status Status);
void chg_block_pid (int pid, Th_Status Status);
struct threads *get_thread (void (*program) (void));
void threadHandoffCPU (struct threads *t, Th_Status stat);
struct threads *next_runnable (void);
void threadExit (void);

/* Timer Related */

struct _timer *create_timer (void (*handler_fcn) (void *), void *socket,
			     int immediate,
			     struct timeval *expiration,
			     struct _timer *timer_element, int type);
int delete_timer (struct _timer *timer_element, int mask);
uint32_t set_timer (struct timeval *expiration, struct _timer
			 *timer_element, int mask);
int enqueue_timer (struct _timer_queue *queue, struct _timer
		   *timer_element);
void dequeue_timer (struct _timer *timer_element, int mask);
//void dequeue_timer (struct _timer *timer_element, int mask, struct _timer *head);
int clear_timer (struct _timer *timer_element, int mask);
void alarm_handler (void);
void alarm_handler_new (unsigned int sign);
void sigio_handler (void);
void service_timers ();

#ifdef REMOVE_LATER
#undef MAX_SCPS_SOCKET
#endif /* REMOVE LATER */
#endif /* _Thread */
