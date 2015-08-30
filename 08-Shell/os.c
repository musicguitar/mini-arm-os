#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"

void shell_thread(void* arg);
int shell_pid=0;

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)

void usart_init(void)
{
	*(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
	*(RCC_APB1ENR) |= (uint32_t) (0x00020000);

	/* USART2 Configuration, Rx->PA3, Tx->PA2 */
	*(GPIOA_CRL) = 0x00004B00;
	*(GPIOA_CRH) = 0x44444444;
	*(GPIOA_ODR) = 0x00000000;
	*(GPIOA_BSRR) = 0x00000000;
	*(GPIOA_BRR) = 0x00000000;

#ifdef USART_INT
	*(USART2_CR1) = 0x0000002C; /* Enable USART2 RXNE */

	/* Set IRQ priority */
        /* USART2 IRQ set to 16, higher than PendSV */
        *(NVIC_IPR9) = 0x000F0000;
	/* PendSV priority set to 17 */
	*(PRI_N2) = 0x00100000;

	/* Enable USART2 IRQ */
        *(NVIC_ISER1) = 0x00000040;
#else
	*(USART2_CR1) = 0x0000000C;
#endif
	*(USART2_CR2) = 0x00000000;
	*(USART2_CR3) = 0x00000000;
	*(USART2_CR1) |= 0x2000;
	
}

void print_str(const char *str)
{
	while (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
		str++;
	}
}

static void delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

static void busy_loop(void *str)
{
	while (1) {
		print_str(str);
		print_str(": Running...\n");
		delay(1000);
	}
}

void test1(void *userdata)
{
	busy_loop(userdata);
}


/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
	usart_init();

	shell_pid = thread_create(shell_thread, (void *) NULL);
	if(shell_pid == -1)
		print_str("shell_thread 1 creation failed\r\n");

	//thread_create(test1, (void*)"test1 ");


	/* SysTick configuration */
	*SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ /100) - 1UL;
	*SYSTICK_VAL = 0;
	*SYSTICK_CTRL = 0x07;

	thread_start();

	return 0;
}
