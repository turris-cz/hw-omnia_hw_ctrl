#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

static uint32_t crc32_one(uint32_t crc, uint8_t byte)
{
	int i;

	crc ^= byte << 24;
	for (i = 0; i < 8; ++i)
		crc = (crc << 1) ^ ((crc & 0x80000000) ? 0x04c11db7 : 0);

	return crc;
}

static uint32_t do_crc32(uint8_t *data, uint32_t len)
{
	uint32_t crc = 0, i;

	if (len & 3) {
		fprintf(stderr, "%s: len must be divisible by 4\n", __func__);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < len; i += 4, data += 4) {
		crc = crc32_one(crc, data[3]);
		crc = crc32_one(crc, data[2]);
		crc = crc32_one(crc, data[1]);
		crc = crc32_one(crc, data[0]);
	}

	return crc;
}

static uint8_t *read_mcu_code(const char *path, uint32_t *size)
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

int main (int argc, char **argv)
{
	unsigned long offset;
	char *end;
	uint32_t size;
	uint8_t *buf;

	if (argc != 3) {
		fprintf(stderr, "Usage: crc32tool <offset> omnia_hw_ctrl.bin.nocrc >omnia_hw_ctrl.bin\n\n");
		exit(EXIT_FAILURE);
	}

	if (isatty(STDOUT_FILENO)) {
		fprintf(stderr, "stdout must not by a tty\n");
		exit(EXIT_FAILURE);
	}

	buf = read_mcu_code(argv[2], &size);

	offset = strtoul(argv[1], &end, 0);
	if (*end != '\0' || offset > size - 8 || !offset || offset % 4) {
		fprintf(stderr, "Invalid offset %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	*(uint32_t *)&buf[offset] = htole32(size);
	*(uint32_t *)&buf[offset + 4] = htole32(0);
	*(uint32_t *)&buf[offset + 4] = htole32(do_crc32(buf, size));

	if (write(STDOUT_FILENO, buf, size) != size) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
