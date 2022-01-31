#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/pci.h>
#include <stdarg.h>

struct socket_regs {
	uint32_t event;
	uint32_t mask;
	uint32_t state;
	uint32_t force;
	uint32_t ctrl;
	uint32_t res[3];
	uint32_t power;
};

struct __attribute__((packed)) exca_regs {
	uint8_t IDR;
	uint8_t ISR;
	uint8_t PCTRL;
	uint8_t IGC;
	uint8_t CSC;
	uint8_t CSCI;
	uint8_t AWEN;
	uint8_t IOWC;
	uint16_t IOWS0;
	uint16_t IOWE0;
	uint16_t IOWS1;
	uint16_t IOWE1;

	/* Memory window 0 */
	uint16_t MWS0;
	uint16_t MWE0;
	uint16_t MWO0;
	uint8_t CDC;
	uint8_t res0;

	/* Memory window 1 */
	uint16_t MWS1;
	uint16_t MWE1;
	uint16_t MWO1;
	uint8_t GC;
	uint8_t res1;

	/* Memory window 2 */
	uint16_t MWS2;
	uint16_t MWE2;
	uint16_t MWO2;
	uint8_t res2;
	uint8_t res3;

	/* Memory window 2 */
	uint16_t MWS3;
	uint16_t MWE3;
	uint16_t MWO3;
	uint8_t res4;
	uint8_t res5;

	/* Memory window 2 */
	uint16_t MWS4;
	uint16_t MWE4;
	uint16_t MWO4;
	uint16_t IOWO0;
	uint16_t IOWO1;

	uint8_t res8[6];
	uint8_t MWP0;
	uint8_t MWP1;
	uint8_t MWP2;
	uint8_t MWP3;
	uint8_t MWP4;
};

struct ti1410_s {
	const char *fname;
	int mem_fd;
	void *regs;
	volatile struct socket_regs *sock;
	volatile struct exca_regs *exca;
	uint8_t *mem;
};

void die(const char *fmt, ...)
{
	fprintf(stderr, "ERROR: %s\n", fmt);
	fprintf(stderr, "ERROR: system reports: %s\n", strerror(errno));
	exit(1);
}

int report(const char *fmt, ...)
{
	va_list v;

	va_start(v, fmt);
	vfprintf(stderr, fmt, v);
	va_end(v);
}



void ti_open(struct ti1410_s *t)
{
	t->mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (t->mem_fd < 0)
		die("unable to open /dev/mem");

	t->regs = mmap(0, 4096,PROT_READ | PROT_WRITE, MAP_SHARED,
			t->mem_fd, 0xfe400000);
	if (t->regs == (void *)-1)
		die("unable to mmap ti1410");

	t->sock = t->regs;
	t->exca = (void *)((uint8_t *)t->regs + 0x800);

	t->mem = mmap(0, 1024 * 1024, PROT_READ|PROT_WRITE, MAP_SHARED,
			t->mem_fd, 0xfd400000);

	if (t->mem == (void *)-1)
		die("unable to map /dev/mem");

	report("memory mapped into %p\n", t->mem);
}
int main(int argc, char **argv)
{
	struct ti1410_s ti;
	int i;
	int attr = 0;

	uint16_t offset;

	if (argc > 1 && strcmp(argv[1], "attr") == 0) {
		report("reading attribute memory\n");
		attr = 1;
	}

	ti_open(&ti);
	report("IDR=%02x\n", ti.exca->IDR);
	report("ISR=%02x\n", ti.exca->ISR);
	report("AWEN=%02x\n", ti.exca->AWEN);
	report("MWS0=%04x\n", ti.exca->MWS0);
	report("MWE0=%04x\n", ti.exca->MWE0);
	report("MWO0=%04x\n", ti.exca->MWO0);
	report("MWP0=%02x\n", ti.exca->MWP0);
	report("PCTRL=%02x\n", ti.exca->PCTRL);

	ti.exca->MWP0 = 0xfd;
	ti.exca->MWO0 = 0x8000;

	if (attr) 
		ti.exca->MWO0 |= 0x4000;

	ti.exca->MWS0 = 0x0400;
	ti.exca->MWE0 = 0x0500;
	ti.exca->AWEN |= 1;
	ti.exca->PCTRL = 0x90;

	/* Allow a delay for turn-on */
	usleep(1000);

	for (i=0;i<1024 * 1024;i+=2) {
		uint16_t data = *(uint16_t *)(ti.mem + i);
		fputc(data & 0xff, stdout);
		if (!attr)
			fputc(data >> 8, stdout);
	}

	/* Turn off */
	ti.exca->AWEN &= ~1;
	ti.exca->PCTRL = 0x00;
}



