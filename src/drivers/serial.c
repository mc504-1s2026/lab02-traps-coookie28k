#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/plic.h>
#include <arch/csr.h>
#include <arch/spinlock.h>
#include <arch/io.h>
#include <kernel/mm.h>

#define RING_BUF_SIZE 512


static char rx_buf[RING_BUF_SIZE];

static size_t rx_head = 0;
static size_t rx_tail = 0;

static struct spinlock serial_lock;

static inline void *serial_reg_addr(u64 offset)
{
	phys_addr_t pa = (phys_addr_t)SERIAL_BASE + offset;
	return (void *)pa_to_va(pa);
}


static inline u8 serial_read_reg(u64 offset)
{
	return ioread8(serial_reg_addr(offset));
}


static inline void serial_write_reg(u64 offset, u8 value)
{
	iowrite8(value, serial_reg_addr(offset));
}


void serial_init()
{
	spin_init(&serial_lock);

	rx_head = 0;
	rx_tail = 0;

	serial_write_reg(SERIAL_IER, 0x00);

	serial_write_reg(SERIAL_LCR, 0x03);

	serial_write_reg(
		SERIAL_FCR,
		SERIAL_FCR_FIFO_ENABLE |
		SERIAL_FCR_RX_FIFO_CLEAR |
		SERIAL_FCR_TX_FIFO_CLEAR
	);
}

void serial_irq_enable()
{
	serial_write_reg(SERIAL_IER, SERIAL_IER_ERBFI);

	plic_irq_set_priority(IRQ_SERIAL, 1);

	plic_hart_set_threshold(0, 0);

	plic_hart_enable_irq(0, IRQ_SERIAL);

	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	serial_write_reg(SERIAL_IER, 0x00);

	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	spin_lock(&serial_lock);

	while (serial_read_reg(SERIAL_LSR) & SERIAL_LSR_DTR) {

		char c = serial_read_reg(SERIAL_RBR);

		size_t next =
			(rx_head + 1) % RING_BUF_SIZE;

		if (next != rx_tail) {

			rx_buf[rx_head] = c;
			rx_head = next;
		}
	}

	spin_unlock(&serial_lock);
}

size_t serial_read(char *buf)
{
	size_t count = 0;

	u64 flags = spin_lock_irqsave(&serial_lock);

	while (rx_tail != rx_head) {

		buf[count++] = rx_buf[rx_tail];

		rx_tail =
			(rx_tail + 1) % RING_BUF_SIZE;
	}

	spin_unlock_irqrestore(&serial_lock, flags);

	return count;
}

void serial_puts(char *str)
{
	while (*str) {

		if (*str == '\n') {
			serial_putc('\r');
		}

		serial_putc(*str);
		str++;
	}
}

void serial_putc(char c)
{
	while (!(serial_read_reg(SERIAL_LSR)
	         & SERIAL_LSR_THRE)) {
	}

	serial_write_reg(SERIAL_THR, c);
}
