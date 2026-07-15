#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <kernel/trap.h>
#include <kernel/serial.h>

volatile bool alarm_active = false;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);

	hart_irq_enable();

	csr_write(CSR_STIMECMP, -1ULL);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	alarm_active = true;
	u64 next = timer_read() + secs * TIMER_FREQ;

	csr_write(CSR_STIMECMP, next);
}

void timer_irq()
{
	if (alarm_active) {
		serial_puts("alarm\n");
		alarm_active = false;
	}
	
	csr_write(CSR_STIMECMP, -1ULL);
}
