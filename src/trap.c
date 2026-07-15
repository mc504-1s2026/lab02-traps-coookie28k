#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <arch/plic.h>
#include <kernel/printf.h>
#include <kernel/serial.h>
#include <arch/timer.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
	u32 irq = plic_hart_claim_irq(0);

	if (irq == 0) {
		return;
	}

	switch (irq) {
	case IRQ_SERIAL:
		serial_irq();
		break;

	default:
		error("Unknown external interrupt: %d\n", irq);
		break;
	}

	plic_hart_complete_irq(0, irq);
}

void handle_exception()
{
	u64 scause = csr_read(CSR_SCAUSE);
	u64 sepc = csr_read(CSR_SEPC);
	u64 stval = csr_read(CSR_STVAL);

	critical("EXCEPTION TRIGGERED!\n");
	critical("scause: %p, sepc: %p, stval: %p\n",
	         scause, sepc, stval);

	switch (scause) {

	case EXCEPTION_INST_PAGE_FAULT:
		critical("Instruction Page Fault\n");
		break;

	case EXCEPTION_LOAD_PAGE_FAULT:
		critical("Load Page Fault\n");
		break;

	case EXCEPTION_STORE_PAGE_FAULT:
		critical("Store/AMO Page Fault\n");
		break;

	default:
		critical("Unhandled Exception\n");
		break;
	}

	BUG();
}

void trap_setup()
{
	csr_write(CSR_STVEC, (u64)trap_entry);
	hart_irq_disable();

}

void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE);

	if (scause & TRAP_IRQ_BIT) {

		switch (scause) {

		case TRAP_TIMER_IRQ:
			timer_irq();
			break;

		case TRAP_EXTERNAL_IRQ:
			handle_irq();
			break;

		default:
			panic("Unknown IRQ: %ld\n",
			      scause & ~TRAP_IRQ_BIT);
		}

	} else {
		handle_exception();
	}
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 sstatus = csr_read_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	return sstatus & CSR_SSTATUS_SIE;
}

void hart_irq_restore(u64 flags)
{
	if (flags) {
		csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
	} else {
		csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	}
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
