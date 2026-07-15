#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>
#include <stdbool.h>

extern int _hartid[];


static u64 parse_number(char *str)
{
	u64 value = 0;

	while (*str >= '0' && *str <= '9') {
		value = value * 10 + (*str - '0');
		str++;
	}

	return value;
}


static void execute_command(char *cmd)
{
	if (strcmp(cmd, "uptime") == 0) {

		u64 seconds = timer_read() / TIMER_FREQ;
		printk(LOG_INFO, "%llus\n", seconds);

	} else if (strncmp(cmd, "echo ", 5) == 0) {

		serial_puts(cmd + 5);
		serial_puts("\n");

	} else if (strncmp(cmd, "alarm ", 6) == 0) {

		u64 seconds = parse_number(cmd + 6);

		if (seconds > 0) {
			timer_set_alarm(seconds);
		}

	} else if (strlen(cmd) != 0) {

		serial_puts("unknown command\n");
	}
}


void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	hart_irq_enable();
	char command[128];
	size_t command_len = 0;

	serial_puts("> ");
	
	while (1) {

		char buffer[64];
		size_t received = serial_read(buffer);
		for (size_t i = 0; i < received; i++) {

			char c = buffer[i];

			if (c == '\r') {
				serial_puts("\n");
				command[command_len] = '\0';
				execute_command(command);
				command_len = 0;
				serial_puts("> ");
			} 

			else if (c == '\b' || c == 0x7f) {

				if (command_len > 0) {

					command_len--;
					serial_puts("\b \b");
				}
			}

			else {

				if (command_len < sizeof(command)-1) {
					command[command_len++] = c;
					serial_putc(c);
				}
			}
		}
	}
}
