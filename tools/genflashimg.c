#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

static uint8_t *read_file(const char *path, uint32_t *size)
{
	struct stat st;
	uint8_t *buf;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	if (fstat(fd, &st) < 0) {
		perror("fstat");
		exit(EXIT_FAILURE);
	}

	if ((st.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "%s is not a regular file\n", path);
		exit(EXIT_FAILURE);
	}

	*size = ((uint32_t)st.st_size + 3) & ~3UL;
	buf = aligned_alloc(4, *size);
	memset(buf, 0xff, *size);

	if (!buf) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	if (read(fd, buf, st.st_size) != st.st_size) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	close(fd);

	return buf;
}

static void xwrite(void *buf, size_t size)
{
	if (write(STDOUT_FILENO, buf, size) != size) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

int main (int argc, char **argv)
{
	uint8_t *boot_buf, *app_buf, bufFF[0x400];
	uint32_t boot_filesize, app_filesize, pos;
	unsigned long boot_partsize;
	char *end;

	if (argc != 4) {
		fprintf(stderr, "Usage: genflashimg <boot.bin> <app.bin> <boot_part_size> >flash.bin\n\n");
		exit(EXIT_FAILURE);
	}

	if (isatty(STDOUT_FILENO)) {
		fprintf(stderr, "stdout must not by a tty\n");
		exit(EXIT_FAILURE);
	}

	boot_buf = read_file(argv[1], &boot_filesize);

	boot_partsize = strtoul(argv[3], &end, 0);
	if (*end != '\0' || boot_partsize % 0x400) {
		fprintf(stderr, "Invalid boot partition size %s\n", argv[1]);
		exit(EXIT_FAILURE);
	} else if (boot_partsize < boot_filesize) {
		fprintf(stderr, "Boot file too large (%u > %lu)\n", (unsigned int)boot_filesize, boot_partsize);
		exit(EXIT_FAILURE);
	}

	/* write boot.bin to stdout */
	xwrite(boot_buf, boot_filesize);
	free(boot_buf);

	/* write 0xFF bytes to stdout until boot_partsize bytes are filled */
	memset(bufFF, 0xFF, sizeof(bufFF));
	pos = boot_filesize;
	while (pos < boot_partsize) {
		size_t wr = boot_partsize - pos;

		if (wr > sizeof(bufFF))
			wr = sizeof(bufFF);

		xwrite(bufFF, wr);

		pos += wr;
	}

	/* write app.bin to stdout */
	app_buf = read_file(argv[2], &app_filesize);
	xwrite(app_buf, app_filesize);
	free(app_buf);
	pos += app_filesize;

	/* write 0xFF until output aligned to 0x400 bytes */
	if (pos % 0x400)
		xwrite(bufFF, 0x400 - (pos % 0x400));

	exit(EXIT_SUCCESS);
}
