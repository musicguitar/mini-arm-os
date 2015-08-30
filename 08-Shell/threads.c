#include <stdint.h>
#include "threads.h"
#include "os.h"
#include "malloc.h"
#include "reg.h"

#define THREAD_PSP	0xFFFFFFFD

/* Thread Control Block */
typedef struct {
	void *stack;
	void *orig_stack;
	uint8_t in_use;
	char sleep;
} tcb_t;

static tcb_t tasks[MAX_TASKS];
static int lastTask;
static int first = 1;

/* FIXME: Without naked attribute, GCC will corrupt r7 which is used for stack
 * pointer. If so, after restoring the tasks' context, we will get wrong stack
 * pointer.
 */
void print_str(const char *str);
void __attribute__((naked)) pendsv_handler()
{
	/* Save the old task's context */
	asm volatile("mrs   r0, psp\n"
	             "stmdb r0!, {r4-r11, lr}\n"
	             "mov   %0, r0\n"
	             : "=r" (tasks[lastTask].stack));

	/* Find a new task to run */
	while (1) {
		lastTask++;
		if (lastTask == MAX_TASKS)
			lastTask = 0;
		if (tasks[lastTask].in_use && !tasks[lastTask].sleep) {
			/* Restore the new task's context and jump to the task */
			asm volatile("mov r0, %0\n"
			             "ldmia r0!, {r4-r11, lr}\n"
			             "msr psp, r0\n"
			             "bx lr\n"
			             : : "r" (tasks[lastTask].stack));
		}
	}
}

void systick_handler()
{
	*SCB_ICSR |= SCB_ICSR_PENDSVSET;
}

void thread_start()
{
	lastTask = 0;

	/* Save kernel context */
	asm volatile("mrs ip, psr\n"
	             "push {r4-r11, ip, lr}\n");

	/* Load user task's context and jump to the task */
	asm volatile("mov r0, %0\n"
	             "msr psp, r0\n"
	             "mov r0, #3\n"
	             "msr control, r0\n"
	             "isb\n"
	             "pop {r4-r11, lr}\n"
	             "pop {r0}\n"
	             "bx lr\n"
	             : : "r" (tasks[lastTask].stack));
}

int thread_create(void (*run)(void *), void *userdata)
{
	/* Find a free thing */
	int threadId = 0;
	uint32_t *stack;

	for (threadId = 0; threadId < MAX_TASKS; threadId++) {
		if (tasks[threadId].in_use == 0)
			break;
	}

	if (threadId == MAX_TASKS)
		return -1;

	/* Create the stack */
	stack = (uint32_t *) malloc(STACK_SIZE * sizeof(uint32_t));
	tasks[threadId].orig_stack = stack;
	if (stack == 0)
		return -1;

	stack += STACK_SIZE - 32; /* End of stack, minus what we are about to push */
	if (first) {
		stack[8] = (unsigned int) run;
		stack[9] = (unsigned int) userdata;
		first = 0;
	} else {
		stack[8] = (unsigned int) THREAD_PSP;
		stack[9] = (unsigned int) userdata;
		stack[14] = (unsigned) &thread_self_terminal;
		stack[15] = (unsigned int) run;
		stack[16] = (unsigned int) 0x21000000; /* PSR Thumb bit */
	}

	/* Construct the control block */
	tasks[threadId].stack = stack;
	tasks[threadId].in_use = 1;
	tasks[threadId].sleep = 0;

	return threadId;
}

void thread_sleep(int threadId)
{
	tasks[threadId].sleep = 1;
	*SCB_ICSR |= SCB_ICSR_PENDSVSET;
}

void thread_wake(int threadId)
{
	tasks[threadId].sleep = 0;
}

void thread_kill(int thread_id)
{
	tasks[thread_id].in_use = 0;

	/* Free the stack */
	free(tasks[thread_id].orig_stack);
}

void thread_self_terminal()
{
	/* This will kill the stack.
	 * For now, disable context switches to save ourselves.
	 */
	asm volatile("cpsid i\n");
	thread_kill(lastTask);
	asm volatile("cpsie i\n");

	/* And now wait for death to kick in */
	while (1);
}


void write_blank(int blank_num)
{
        int blank_count = 0;

        while (blank_count <= blank_num) {
                print_str(" ");
                blank_count++;
        }
}


//ps
void show_task_info(void *userdata)
{
        char ps_message[]="PID SLEEP\r\n\0";
        int task_i;

        print_str(ps_message);

        for (task_i = 0; task_i < MAX_TASKS; task_i++) {
                if(tasks[task_i].in_use==0)continue;
                char task_info_pid[2]={0,'\0'};
                char task_info_status[2]={0,'\0'};

                task_info_pid[0]='0'+task_i;
                task_info_status[0]='0'+tasks[task_i].sleep;

                print_str(task_info_pid);
                write_blank(3);
                print_str(task_info_status);

                print_str("\r\n\0");
        }

	print_str("\r\n\0");
}
