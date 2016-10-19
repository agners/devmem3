/*
 * devmem3.c: Simple program to read/write from/to any location in memory.
 *
 * Copyright (C) 2016, Stefan Agner (stefan.agner@toradex.com)
 * based on devmem2.c:
 * Copyright (C) 2000, Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
 
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

#define exit_error() \
{ \
	fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
		__LINE__, __FILE__, errno, strerror(errno)); \
	exit(1); \
}

#ifdef DEBUG
#define dprintf(fmt, ...)	printf(fmt, __VA_ARGS__)
#else
#define dprintf(fmt, ...)
#endif

void print_usage(const char* name)
{
	fprintf(stderr, "\nUsage:\t%s r { address } [ type [ count ] ]\n"
			"\t%s w { address } { type } { data } [ count ]\n"
			"\taddress : memory address to act upon\n"
			"\ttype    : access operation type : [b]yte, [h]alfword, [l]ong (default)\n"
			"\tdata    : data to be written\n\n"
			"\tcount   : count of accesses\n\n",
			name, name);
}

int main(int argc, char **argv) {
	int fd;
	void *map_base, *virt_addr; 
	unsigned long read_result, writeval;
	off_t target;
	int access_type = 'l';
	int access_length;
	int iswrite = 0;
	int count = 1;
	
	if (argc < 3) {
		print_usage(argv[0]);
		exit(1);
	}

	if (argv[1][0] == 'r') {
		iswrite = 0;
	} else if (argv[1][0] == 'w') {
		iswrite = 1;
	} else {
		print_usage(argv[0]);
		exit(1);
	}

	target = strtoul(argv[2], 0, 16);

	if(argc > 2)
		access_type = tolower(argv[3][0]);
	switch (access_type) {
	case 'b':
		access_length = 1;
		break;
	case 'h':
		access_length = 2;
		break;
	case 'l':
		access_length = 4;
		break;
	default:
		fprintf(stderr, "Illegal data type '%c'.\n", access_type);
		exit(2);
	}

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
		exit_error();

	/* Map one page */
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if (map_base == (void *) -1)
		exit_error();

	dprintf("Memory mapped at address %p.\n", map_base); 

	virt_addr = map_base + (target & MAP_MASK);

	if (argc > 4)
		count = strtoul(argv[4], 0, 0); 

	for (int i = 0; i < count; i++) {
		if (!((i * access_length) % 16))
			printf("\n%08x:", target);

		switch (access_length) {
		case 1:
			read_result = *((unsigned char *) virt_addr);
			printf(" %02x", read_result); 
			break;
		case 2:
			read_result = *((unsigned short *) virt_addr);
			printf(" %04x", read_result); 
			break;
		case 4:
			read_result = *((unsigned long *) virt_addr);
			printf(" %08x", read_result); 
			break;
		}
		virt_addr += access_length;
		target += access_length;
	}

	printf("\n\n");
	fflush(stdout);

	if(iswrite) {
		writeval = strtoul(argv[4], 0, 16);
		switch(access_type) {
			case 'b':
				*((unsigned char *) virt_addr) = writeval;
				read_result = *((unsigned char *) virt_addr);
				break;
			case 'h':
				*((unsigned short *) virt_addr) = writeval;
				read_result = *((unsigned short *) virt_addr);
				break;
			case 'w':
				*((unsigned long *) virt_addr) = writeval;
				read_result = *((unsigned long *) virt_addr);
				break;
		}
		printf("Written 0x%X; readback 0x%X\n", writeval, read_result); 
		fflush(stdout);
	}
	
	if (munmap(map_base, MAP_SIZE) == -1)
		exit_error();
	close(fd);
	return 0;
}

