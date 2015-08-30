#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include "reg.h"

#define USART_FLAG_RXNE  ((uint16_t) 0x0020)
#define USART_FLAG_ORE  ((uint16_t) 0x0008)


#define PROMPT "STM32:~$ "
#define NEXT_LINE "\r\n\0"

#define CMDBUF_SIZE 64
#define RXBUF_SIZE 10
#define MAX_ARGC 19
#define MAX_CMDNAME 19
#define MAX_CMDHELP 1023
#define MAX_ENVNAME 15

char cmd[CMDBUF_SIZE];

char usart_rx_buf[RXBUF_SIZE];
char *rxp=usart_rx_buf;

/* Command handlers. */
void show_cmd_info(void *userdata);
void show_task_info(void *userdata);
void fib(void* n);
void print_str(const char *str);
int thread_create(void (*run)(void *), void *userdata);
void thread_sleep(int threadId);
void thread_wake(int threadId);
void register_console_rx(int pid);

/* Enumeration for command types. */
enum {
        CMD_HELP,
        CMD_PS,
	CMD_FIB,
        CMD_COUNT
} CMD_TYPE;

/* Structure for command handler. */
typedef struct {
        char cmd[MAX_CMDNAME + 1];
        void (*func)(void *);
        char description[MAX_CMDHELP + 1];
} hcmd_entry;

const hcmd_entry cmd_data[CMD_COUNT] = {
        [CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."},
        [CMD_PS] = {.cmd = "ps", .func = show_task_info, .description = "List all the processes."},
	[CMD_FIB] = {.cmd = "fib", .func = fib, .description = "Fibonacci function, usage: fib n"},
};


size_t strlen(const char *str)
{
	const char *s;

        for (s = str; *s; ++s);

        return (s - str);
}

int strcmp(const char *s1, const char *s2)
{
	while( (*s1!='\0') && (*s1==*s2) ){
        	s1++; 
        	s2++;
    	}

    	return (int8_t)*s1 - (int8_t)*s2;
}


#if 0
/* Fill in entire value of argument. */
int fill_arg(char *const dest, const char *argv)
{
        char env_name[MAX_ENVNAME + 1];
        char *buf = dest;
        char *p = NULL;

        for (; *argv; argv++) {
                if (isalnum((int)*argv) || *argv == '_') {
                        if (p)
                                *p++ = *argv;
                        else
                                *buf++ = *argv;
                }
                else { /* Symbols. */
                        if (p) {
                                *p = '\0';
                                p = find_envvar(env_name);
                                if (p) {
                                        strcpy(buf, p);
                                        buf += strlen(p);
                                        p = NULL;
                                }
                        }
                        if (*argv == '$')
                                p = env_name;
                        else
                                *buf++ = *argv;
                }
        }
        if (p) {
                *p = '\0';
                p = find_envvar(env_name);
                if (p) {
                        strcpy(buf, p);
                        buf += strlen(p);
                }
        }
        *buf = '\0';

        return buf - dest;
}
#endif

extern int shell_pid;

void wait_cmd_in(char *p)
{
	char put_ch[2]={'0','\0'};
	int key_count=0;

	while(1)
        {
#ifdef USART_INT

		if(rxp == usart_rx_buf)
		{
			/* sleep, wait for USART RX*/
			thread_sleep(shell_pid);
		}

                rxp--;
                put_ch[0]=*(rxp);

#else
		while(!(*(USART2_SR) & USART_FLAG_RXNE));
	        put_ch[0] = *(USART2_DR) & 0xFF;
#endif
                key_count++;
                if (put_ch[0] == '\r' || put_ch[0] == '\n') {
                        *p = '\0';
                        print_str(NEXT_LINE);
                        break;
                }
                else if (put_ch[0] == 127 || put_ch[0] == '\b') {
                       if (p > cmd && key_count) {
                                p--;
                                print_str("\b \b");
                                key_count--;
                       }
                }
                else if (p - cmd < CMDBUF_SIZE - 1) {
                        *p++ = put_ch[0];
                        print_str(put_ch);
                }
        }

}

//int isspace(const char ch) { return (ch==32)?1:0; };
char *cmdtok(char *cmd)
{
        static char *cur = NULL;
        static char *end = NULL;
        if (cmd) {
                char quo = '\0';
                cur = cmd;
                for (end = cmd; *end; end++) {
                        if (*end == '\'' || *end == '\"') {
                                if (quo == *end)
                                        quo = '\0';
                                else if (quo == '\0')
                                        quo = *end;
                                *end = '\0';
                        }
                        else if (isspace((int)*end) && !quo)
                                *end = '\0';
                }
        }
        else
                for (; *cur; cur++)
                        ;

        for (; *cur == '\0'; cur++)
                if (cur == end) return NULL;
        return cur;
}



void shell_thread(void* arg)
{
        char *p = NULL;
	int i;
	char *argv[MAX_ARGC + 1] = {NULL};
        int argc = 1;

	register_console_rx(shell_pid);
        while(1) {
		print_str(PROMPT);
		p = cmd;
		argc = 1;
		wait_cmd_in(p);
		argv[0] = cmdtok(cmd);

        	if (!argv[0])
                	continue;
		while (1) {
        	        argv[argc] = cmdtok(NULL);
        	        if (!argv[argc])
        	                break;
        	        argc++;
        	        if (argc >= MAX_ARGC)
        	                break;
        	}

		/* Check CMD list */
		for (i = 0; i < CMD_COUNT; i++) {
        	        if (!strcmp(cmd, cmd_data[i].cmd)) {
        	                //cmd_data[i].func(NULL);
				thread_create(cmd_data[i].func, (void *)argv[1]);
				thread_sleep(shell_pid);
        	                break;
        	        }
        	}
        	if (i == CMD_COUNT) {
        	        print_str(cmd);
        	        print_str(": command not found");
        	        print_str(NEXT_LINE);
        	}
        }
}

char *ultoa(unsigned long num, char *str, int radix) {
    char temp[33];  //an int can only be 16 bits long
                    //at radix 2 (binary) the string
                    //is at most 16 + 1 null long.
    int temp_loc = 0;
    int digit;
    int str_loc = 0;

    //construct a backward string of the number.
    do {
        digit = (unsigned long)num % radix;
        if (digit < 10) 
            temp[temp_loc++] = digit + '0';
        else
            temp[temp_loc++] = digit - 10 + 'A';
        num = ((unsigned long)num) / radix;
    } while ((unsigned long)num > 0);

    temp_loc--;


    //now reverse the string.
    while ( temp_loc >=0 ) {// while there are still chars
        str[str_loc++] = temp[temp_loc--];    
    }
    str[str_loc] = 0; // add null termination.

    return str;
}

//fibonacci
extern int fibonacci(int x);
void fib(void *n){
    unsigned long val;
    char str_val[100]={0};
    if(n == NULL)
    {
	print_str("Invalid argument\n");
	print_str("Usage: fin n\n");
    }

    val = strtol(n,NULL,10);
    print_str("input n is ");
    print_str(n);
    print_str("\r\n\0");

    print_str("calculating... ");
    val = fibonacci(val);
    print_str("\r\n\0");

    print_str("fib(n) : ");
    ultoa(val, str_val, 10);
    print_str(str_val);
    print_str("\r\n\n\0");
    return;
 
}

//help
void show_cmd_info(void* userdata)
{
        const char help_desp[] = "This system has commands as follow\n\r\0";
        int i;

        print_str(help_desp);
        for (i = 0; i < CMD_COUNT; i++) {
                print_str(cmd_data[i].cmd);
                print_str(": ");
                print_str(cmd_data[i].description);
                print_str(NEXT_LINE);
        }

	print_str(NEXT_LINE);
}

#ifdef USART_INT
int console_pid=-1;
void register_console_rx(int pid)
{
	console_pid = pid;
}

void usart2_rx_handler()
{
        if(*(USART2_SR) & USART_FLAG_RXNE)
        {
		if(rxp - usart_rx_buf < RXBUF_SIZE -1)
		{
	                *rxp = *(USART2_DR) & 0xFF;
			rxp++;
			if( console_pid != -1)
			{
				thread_wake(console_pid);
			}
		}
        }
}
#endif
