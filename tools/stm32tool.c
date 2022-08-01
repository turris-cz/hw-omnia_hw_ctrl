#define _GNU_SOURCE
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <signal.h>
#include <endian.h>

#define GPIO_TTY_MASK	0x40000000
#define GPIO_TTY_DTR	0x20000000
#define GPIO_TTY_FD(x)	((x) & ~0x60000000)

enum {
	CMD_GET_CMDS	= 0x00,
	CMD_GET_VER	= 0x01,
	CMD_GET_ID	= 0x02,
	CMD_READ	= 0x11,
	CMD_GO		= 0x21,
	CMD_WRITE	= 0x31,
	CMD_ERASE	= 0x43,
	CMD_EXT_ERASE	= 0x44,
	CMD_W_PROTECT	= 0x63,
	CMD_W_UNPROTECT	= 0x73,
	CMD_R_PROTECT	= 0x82,
	CMD_R_UNPROTECT	= 0x92,
	CMD_CHECKSUM	= 0xa1,

	INIT_ESCAPE	= 0x7f,

	REPLY_ACK	= 0x79,
	REPLY_NACK	= 0x1f,
};

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

__attribute__((noreturn)) void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n\n");

	exit(EXIT_FAILURE);
}

static int opentty(const char *ttypath);

static int gpio_open(const char *name, const char *gpio_spec, int ttyfd)
{
	printf("using %s as %s GPIO\n", gpio_spec, name);

	if (!strcasecmp(gpio_spec, "dtr")) {
		return (ttyfd | GPIO_TTY_MASK | GPIO_TTY_DTR);
	} else if (!strcasecmp(gpio_spec, "rts")) {
		return (ttyfd | GPIO_TTY_MASK);
	} else if (!strncmp(gpio_spec, "/dev/tty", 8)) {
		char *spec = strdupa(gpio_spec);
		int flags = GPIO_TTY_MASK;
		char *col;
		int fd;

		col = strchr(spec, ':');
		if (col) {
			*col++ = '\0';
			if (!strcasecmp(col, "dtr"))
				flags |= GPIO_TTY_DTR;
			else if (strcasecmp(col, "rts"))
				die("Invalid gpio spec %s", gpio_spec);
		}

		fd = opentty(spec);

		return (fd | flags);
	} else {
		char path[64];
		int gpio;
		int fd;

		fd = open("/sys/class/gpio/export", O_WRONLY);
		if (fd < 0)
			die("Cannot open gpio export file: %m");

		if (dprintf(fd, "%s\n", gpio_spec) < 0 && errno != EBUSY)
			die("Exporting gpio with spec %s failed: %m", gpio_spec);
		close(fd);

		gpio = atoi(gpio_spec);
		sprintf(path, "/sys/class/gpio/gpio%i/direction", gpio);
		fd = open(path, O_WRONLY);
		if (fd < 0)
			die("Cannot open gpio %i: %m", gpio);

		return fd;
	}
}

static void gpio_close(int gpio)
{
	int fd = GPIO_TTY_FD(gpio);

	if (gpio == -1)
		return;

	close(fd);

	/*
	we saved fd, not gpio number, so we can't unexport now
	if (!(gpio & GPIO_TTY_MASK)) {
		fd = open("/sys/class/gpio/unexport", O_WRONLY);
		if (fd < 0)
			die("Cannot open gpio unexport file: %m");

		if (dprintf(fd, "%d\n", gpio) < 0)
			die("Exporting gpio with spec %s failed: %m", gpio_spec);
		close(fd);
	}
	*/
}

static void gpio_set(int gpio, int val)
{
	int fd = GPIO_TTY_FD(gpio);

	if (gpio == -1)
		return;

	if (gpio & GPIO_TTY_MASK) {
		int arg = (gpio & GPIO_TTY_DTR) ? TIOCM_DTR : TIOCM_RTS;

		if (ioctl(fd, val ? TIOCMBIS : TIOCMBIC, &arg))
			die("Cannot ioctl %s: %m",
			    (gpio & GPIO_TTY_DTR) ? "DTR" : "RTS");
	} else {
		if (dprintf(fd, val ? "high\n" : "low\n") < 0)
			die("Cannot set gpio value: %m");

		if (lseek(fd, 0, SEEK_SET) < 0)
			die("Cannot lseek: %m");
	}
}

static void *xmalloc(size_t sz)
{
	void *res = malloc(sz);
	if (sz && !res)
		die("Out of memory");
	return res;
}

static double now(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) < 0)
		die("Cannot get time: %m");

	return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000L;
}

static void putcf(int c)
{
	putchar(c);
	fflush(stdout);
}

static inline int tcdrain(int fd)
{
	return ioctl(fd, TCSBRK, 1);
}

static int tcflush(int fd, int q)
{
	return ioctl(fd, TCFLSH, q);
}

static int fionread(int fd) {
	int n;

	if (ioctl(fd, FIONREAD, &n))
		die("Cannot ioctl: %m");

	return n;
}

static int xread(int fd, void *buf, size_t size)
{
	struct pollfd pfd;
	ssize_t rd, res;
	int try = 100;

	pfd.fd = fd;
	pfd.events = POLLIN;

	rd = 0;
	while (rd < size && try) {
		pfd.revents = 0;
		res = poll(&pfd, 1, 100);
		if (res < 0)
			die("Cannot poll: %m\n");

		if (res == 0) {
			--try;
			continue;
		}

		res = read(fd, buf + rd, size - rd);
		if (res < 0)
			die("Cannot read %zu bytes: %m", size);

		rd += res;
	}

	if (!try)
		return -1;

	return 0;
}

static void _xwrite(int fd, const void *buf, size_t size, const char *func)
{
	if (write(fd, buf, size) != size)
		die("%s: Cannot write: %m\n", func);
}

#define xwrite(f, b, s) _xwrite((f), (b), (s), __func__)

static int opentty(const char *ttypath)
{
	struct termios2 opts;
	int fd;

	fd = open(ttypath, O_RDWR | O_NOCTTY);

	if (fd < 0)
		die("Cannot open tty %s: %m", ttypath);

	memset(&opts, 0, sizeof(opts));
	ioctl(fd, TCGETS2, &opts);
	opts.c_cflag &= ~CBAUD;
	opts.c_cflag |= B115200;
	opts.c_cc[VMIN] = 0;
	opts.c_cc[VTIME] = 0;
	opts.c_iflag = 0;
	opts.c_lflag = 0;
	opts.c_oflag = 0;
	opts.c_cflag &= ~(CSIZE | PARODD | CSTOPB | CRTSCTS | HUPCL);
	opts.c_cflag |= CS8 | PARENB | CREAD | CLOCAL;
	ioctl(fd, TCSETS2, &opts);
	tcflush(fd, TCIFLUSH);

	return fd;
}

static void hex(const u8 *b, int s)
{
	int i;

	for (i = 0; i < s; ++i)
		printf("%02x%s", b[i], i + 1 == s ? "\n" : "");
}

static int recv_ack(int fd)
{
	int n, i, sz, res = 0;
	u8 rx[64];

	n = fionread(fd);
	if (!n)
		return 0;

	while (n) {
		sz = n > 64 ? 64 : n;

		if (read(fd, rx, sz) != sz)
			die("Cannot read: %m");

		n -= sz;

		for (i = 0; i < sz; ++i)
			if (rx[i] == REPLY_ACK)
				res = 1;

		n = fionread(fd);
	}

	return res;
}

static void consume(int fd, int c, int ms)
{
	u8 rx[64];
	int n, i;

	while (1) {
		n = fionread(fd);
		if (!n) {
			usleep(ms*1000);
			n = fionread(fd);
		}

		if (!n)
			break;

		if (n > 64)
			n = 64;

		if (read(fd, rx, n) != n)
			die("Cannot consume nacks: %m");

		hex(rx, n);

		if (c >= 0)
			for (i = 0; i < n; ++i)
				if (rx[i] != c)
					die("Expected 0x%02x, received 0x%02x", c, rx[i]);
	}
}

static int get_ver(int fd);

static void init(int fd, int power, int mboot)
{
	pid_t pid = 0;
	int i, j;
	u8 tx[64];

	memset(tx, INIT_ESCAPE, sizeof(tx));

	gpio_set(mboot, 0);

	printf("Sending escape sequence to BootROM");
	fflush(stdout);

	for (i = 0; i < 20; ++i) {
		gpio_set(power, 1);
		sleep(1);

		xwrite(1, ".", 1);

		if (power >= 0) {
			pid = fork();
			if (pid < 0)
				die("Fork failed");

			if (!pid) {
				usleep(10000);
				gpio_set(power, 0);
				exit(EXIT_SUCCESS);
			}
		}

		for (j = 0; j < 20; ++j) {
			xwrite(fd, tx, 64);
			tcdrain(fd);
			if (!recv_ack(fd))
				continue;

			printf("\nInit Acked, flushing...\n");
			usleep(100000);
			tcflush(fd, TCIOFLUSH);
			break;
		}

		if (pid) {
			kill(pid, SIGINT);
			waitpid(pid, NULL, 0);
		}

		if (j < 20)
			if (!get_ver(fd))
				break;
	}

	if (i == 20) {
		gpio_set(power, 1);
		gpio_set(mboot, 1);
		die("Did not receive Init Ack");
	}
}

static int read_ack(int fd)
{
	u8 rx;

	if (xread(fd, &rx, 1))
		return -1;

	if (rx != REPLY_ACK) {
		fprintf(stderr, "Expected ACK, received 0x%02x\n", rx);
		if (rx != REPLY_NACK)
			consume(fd, -1, 500);
		return -1;
	}

	return 0;
}

static int send_byte_cksum(int fd, u8 cmd)
{
	u8 buf[2];

	buf[0] = cmd;
	buf[1] = ~cmd;

	xwrite(fd, buf, 2);
	usleep(50000);

	return read_ack(fd);
}

static inline int send_cmd(int fd, u8 cmd)
{
	return send_byte_cksum(fd, cmd);
}

static void getreply(int fd, u8 *reply, int size)
{
	int tries = 10;
	u8 ack;

	while (fionread(fd) < size + 1 && tries-- > 0)
		usleep(10000);

	if (fionread(fd) != size + 1)
		die("Wrong reply size");

	xread(fd, reply, size);
	xread(fd, &ack, 1);

	if (ack != REPLY_ACK)
		die("Reply not acked");
}

static int get_cmds(int fd)
{
	u8 rx[64];
	u8 n, i;

	printf("Get Commands:\n");

	if (send_cmd(fd, CMD_GET_CMDS))
		return -1;

	xread(fd, &n, 1);
	getreply(fd, rx, n + 1);

	printf("  Bootloader v. = 0x%02x\n", rx[0]);
	for (i = 1; i < n + 1; ++i)
		printf("  Cmd 0x%02x\n", rx[i]);
}

static int get_ver(int fd)
{
	u8 rx[3];

	printf("Get Version:\n");

	if (send_cmd(fd, CMD_GET_VER))
		return -1;

	getreply(fd, rx, 3);

	printf("  Bootloader v. = 0x%02x\n", rx[0]);
	printf("  Option byte 1 = 0x%02x\n", rx[1]);
	printf("  Option byte 2 = 0x%02x\n", rx[2]);

	return 0;
}

static u8 cksum(const u8 *buf, int size)
{
	u8 res = 0;

	while (size--)
		res ^= *buf++;

	return res;
}

static int send_buf_cksum(int fd, const void *buf, int size)
{
	u8 sum = cksum(buf, size);

	if (write(fd, buf, size) != size)
		die("Cannot send data: %m");
	if (write(fd, &sum, 1) != 1)
		die("Cannot send cksum: %m");

	return read_ack(fd);
}

static u8 cmd_write(int fd, u32 addr, const u8 *_buf)
{
	u8 buf[257];
	u32 baddr;

	buf[0] = 0xff;
	memcpy(&buf[1], _buf, 256);

	if (send_cmd(fd, CMD_WRITE))
		return 'c';

	baddr = htobe32(addr);
	if (send_buf_cksum(fd, &baddr, sizeof(baddr)))
		return 's';
	if (send_buf_cksum(fd, buf, 257))
		return 's';

	return 0;
}

static u8 cmd_erase(int fd, int page)
{
	u16 tx[2];

	if (send_cmd(fd, CMD_EXT_ERASE))
		return 'c';

	tx[0] = 0;
	tx[1] = htobe16(page);
	if (send_buf_cksum(fd, tx, 4))
		return 's';

	return 0;
}

static u8 cmd_read(int fd, u32 addr, u8 *buf)
{
	u32 baddr;
	u8 s = 255;

	if (send_cmd(fd, CMD_READ))
		return 'c';

	baddr = htobe32(addr);
	if (send_buf_cksum(fd, &baddr, sizeof(baddr)))
		return 's';
	if (send_byte_cksum(fd, s))
		return 'S';

	if (fionread(fd) != 256)
		return 'N';

	xread(fd, buf, 256);

	return 0;
}

static u8 verified_read(int fd, u32 addr, u8 *buf)
{
	u8 tst[256];
	int res, i;

	res = cmd_read(fd, addr, buf);
	if (res)
		return res;

	res = cmd_read(fd, addr, tst);
	if (res)
		return res;

	if (memcmp(buf, tst, 256))
		return 'C';

	return 0;
}

static int read_flash(int fd, const char *path, unsigned int offset)
{
	u32 addr = 0x08000000, size = 0x10000;
	u8 buf[256];
	int i, wfd;

	if (offset >= 0x10000 || offset % 0x400)
		die("invalid offset %u\n", offset);
	addr += offset;
	size -= offset;

	printf("reading flash from offset 0x%x to file %s\n", offset, path);

	wfd = open(path, O_WRONLY | O_CREAT, 0644);
	if (wfd < 0)
		die("Cannot open file %s: %m", path);

	if (ftruncate(wfd, 0))
		die("Cannot truncate: %m");

	for (i = 0; size > 0; size -= 256, addr += 256, i++) {
		int tries = 5;

		for (; tries > 0; --tries) {
			u8 res = verified_read(fd, addr, buf);
			if (!res)
				res = 'r';
			putcf(res);
			if (res == 'r')
				break;
		}

		if (!tries) {
			putcf('\n');
			close(wfd);
			fprintf(stderr, "Cannot read flash page %i part %i/4\n",
				addr >> 10, ((addr & 0x3ff) >> 8) + 1);
			return -1;
		}

		xwrite(wfd, buf, 256);

		if (i % 32 == 31)
			putcf('\n');
	}

	close(wfd);

	return 0;
}

static u8 read_page(int fd, u8 *buf, int page)
{
	u32 addr;
	u8 res;
	int i;

	addr = 0x08000000 | (page << 10);

	for (i = 0; i < 4; ++i, addr += 256, buf += 256) {
		int tries = 5;

		for (; tries > 0; --tries) {
			res = verified_read(fd, addr, buf);
			if (!res)
				break;
		}

		if (tries)
			continue;
		else if (res == 'S')
			memset(buf, 0xff, 256);
		else if (res)
			return res;
	}

	return 0;
}

static u8 write_flash(int fd, const char *path, unsigned int offset)
{
	int rfd, res, page, i;
	u8 *buf, *tst;
	ssize_t rd;

	printf("writing flash at offset 0x%x from file %s\n", offset, path);

	buf = xmalloc(1024);
	tst = xmalloc(1024);

	rfd = open(path, O_RDONLY, path);
	if (rfd < 0)
		die("Cannot open file %s: %m", path);

	page = offset >> 10;
	do {
		memset(buf, 0xff, 1024);
		rd = read(rfd, buf, 1024);
		if (rd < 0)
			die("Cannot read file %s: %m");

		if (rd == 0)
			break;

		res = read_page(fd, tst, page);
		if (res) {
			fprintf(stderr, "\nCannot read flash page %i: '%c'\n", page, res);
			goto end;
		}

		if (!memcmp(buf, tst, rd)) {
			putcf('r');
			continue;
		}

		res = cmd_erase(fd, page);
		if (res) {
			fprintf(stderr, "\nCannot erase flash page %i: '%c'\n", page, res);
			goto end;
		}
		putcf('e');

		for (i = 0; i < 4; ++i) {
			res = cmd_write(fd, 0x08000000 | (page << 10) | (i << 8), &buf[i << 8]);
			if (res) {
				fprintf(stderr, "\nCannot write flash page %i part %i: '%c'\n", page, i, res);
				goto end;
			}
			putcf('w');
		}

		if (page % 8 == 7)
			putcf('\n');
	} while (rd == 1024 && ++page < 64);

end:
	putcf('\n');
	close(rfd);
	free(buf);
	free(tst);

	return res;
}

static void do_boot(int power, int mboot)
{
	printf("booting\n");
	gpio_set(power, 1);
	gpio_set(mboot, 1);
	sleep(1);
	gpio_set(power, 0);
}

static void repl(int fd, int power, int mboot)
{
	unsigned int offset, page;
	size_t line_size;
	ssize_t line_len;
	u8 buf[256];
	char *path;
	char *line;
	int istty;
	int res;

	line = NULL;
	line_size = 0;
	istty = isatty(STDIN_FILENO);

	while (1) {
		if (istty) {
			fputs("stm32cmd> ", stdout);
			fflush(stdout);
		}
		line_len = getline(&line, &line_size, stdin);
		if (line_len < 0)
			break;
		else if (!line_len)
			continue;

		switch (line[0]) {
		case 'h':
			printf("m - simple memory test\n"
			       "r - read flash to file\n"
			       "w - write flash from file\n"
			       "q - quit\n"
			       "b - boot MCU and quit\n"
			       "h - show this help\n"
			       "c - get list of supported commands\n"
			       "v - get version\n"
			       "R - read 256 Bytes of flash to stdout (as hex)\n"
			       "E - erase flash page\n");
			break;
		case 'c':
			get_cmds(fd);
			break;
		case 'v':
			get_ver(fd);
			break;
		case 'm':
			{
				u8 buf[256], res;

				printf("Memory test... ");
				fflush(stdout);

				res = verified_read(fd, 0x20001000, buf);
				if (res) {
					printf("read error: %c\n", res);
					break;
				}
				hex(buf, 16);

				memcpy(buf, "ahoj", 4);
				res = cmd_write(fd, 0x20001000, buf);
				if (res) {
					printf("write error: %c\n", res);
					break;
				}

				res = verified_read(fd, 0x20001000, buf);
				if (res) {
					printf("read error: %c\n", res);
					break;
				}
				hex(buf, 16);
			}
			break;
		case 'r':
			res = sscanf(line + 1, "%ms %x", &path, &offset);
			if (res < 1) {
				fprintf(stderr,
					"usage: r <file> [offset]\n\n"
					"       reads flash from offset [offset] (default 0) to file <file>\n"
					"       [offset] must be page aligned (1 KiB)\n\n");
				break;
			}
			if (res < 2)
				offset = 0;
			read_flash(fd, path, offset);
			free(path);
			break;
		case 'w':
			res = sscanf(line + 1, "%ms %x", &path, &offset);
			if (res < 1) {
				fprintf(stderr,
					"usage: w <file> [offset]\n\n"
					"       updates flash at offset [offset] (default 0) from file <file>\n"
					"       [offset] must be page aligned (1 KiB)\n\n");
				break;
			}
			if (res < 2)
				offset = 0;
			write_flash(fd, path, offset);
			free(path);
			break;
		case 'R':
			if (sscanf(line + 1, "%x", &offset) != 1) {
				fprintf(stderr,
					"usage: R <offset>\n\n"
					"       reads 256 B of flash from offset <offset> (1KiB aligned)\n\n");
				break;
			}

			offset &= 0xff;
			printf("Reading flash addr 0x%x... ", offset << 8);
			fflush(stdout);
			res = cmd_read(fd, 0x08000000 | (offset << 8), buf);
			if (res) {
				printf("error: %c\n", res);
				break;
			}
			hex(buf, 256);
			break;
		case 'E':
			if (sscanf(line + 1, "%u", &page) != 1) {
				fprintf(stderr,
					"usage: E <page>\n\n"
					"       erases flash page <page>\n\n");
				break;
			}

			page &= 0x3f;
			printf("Erasing flash page %u... ", page);
			fflush(stdout);
			res = cmd_erase(fd, page);
			if (res) {
				printf("error: %c\n", res);
				break;
			}

			printf("erased\n");
			break;
		case 'b':
			do_boot(power, mboot);
			printf("quitting\n\n");
			return;
		case 'q':
			printf("quitting\n\n");
			gpio_set(power, 1);
			return;
		}
	}
}

int main(int argc, char **argv)
{
	const char *ttypath = NULL, *rpath = NULL, *wpath = NULL, *pspec = NULL, *mspec = NULL;
	int fd, power = -1, mboot = -1, boot = 0;
	unsigned long offset = 0;
	char *end;

	while (1) {
		int c;

		c = getopt(argc, argv, "D:p:m:r:w:o:b");
		if (c == -1)
			break;

		switch (c) {
		case 'D':
			if (ttypath)
				die("-D already given");
			ttypath = optarg;
			break;
		case 'p':
			if (pspec)
				die("-p already given");
			pspec = optarg;
			break;
		case 'm':
			if (mspec)
				die("-m already given");
			mspec = optarg;
			break;
		case 'r':
			if (rpath || wpath)
				die("-r or -w already given");
			rpath = optarg;
			break;
		case 'w':
			if (rpath || wpath)
				die("-r or -w already given");
			wpath = optarg;
			break;
		case 'o':
			if (offset)
				die("-o already given");

			offset = strtoul(optarg, &end, 0);
			if (*end != '\0' || offset >= 0x10000 || offset % 0x400)
				die("invalid offset %s", optarg);
			break;
		case 'b':
			boot = 1;
			break;
		default:
			die("Error parsing command line");
		}
	}

	if (!ttypath)
		die("Must give tty with -D option!");

	fd = opentty(ttypath);

	if (pspec)
		power = gpio_open("power", pspec, fd);
	if (mspec)
		mboot = gpio_open("mboot", mspec, fd);

	init(fd, power, mboot);

	if (rpath) {
		if (read_flash(fd, rpath, offset))
			exit(EXIT_FAILURE);
		if (boot)
			do_boot(power, mboot);
	} else if (wpath) {
		if (write_flash(fd, wpath, offset))
			exit(EXIT_FAILURE);
		if (boot)
			do_boot(power, mboot);
	} else {
		repl(fd, power, mboot);
	}

	close(fd);
}
