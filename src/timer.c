#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <kernel/trap.h>

volatile u64 uptime_seconds = 0;
volatile u64 alarm_target_second = 0;
volatile bool alarm_active = false;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);

	hart_irq_enable();

	timer_set_alarm(1);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 next = timer_read() + secs * TIMER_FREQ;

	csr_write(CSR_STIMECMP, next);
}

void timer_irq()
{
	uptime_seconds++;

	if (alarm_active &&
	    uptime_seconds >= alarm_target_second) {

		print("alarm\n");

		alarm_active = false;
	}

	timer_set_alarm(1);
}
