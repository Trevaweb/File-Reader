/***********************************************************
 * Name of program: endian_test.c
 * Authors: Sarah Diesburg
 * Description: An example program to see how to use endian 
 * and file I/O functions. Requires example.txt to run.
 * Compile: $> gcc endian_test.c -o endian_test
 **********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* This program reads a number in little-endian format from a file,
 * converts it to host order, adds 1 to it, converts it back to 
 * little-endian, and writes it back to a file. */
int main() {

	uint16_t read_num = 0; /*little-endian num read from fat file*/
	uint16_t write_num = 0; /*little-endian num written to fat file*/
	uint16_t convert = 0; /*holds conversion to host architecture*/
	int fd; /*file descriptor*/
	int result; /*error checking*/


	/* What architecture are we? */
	if(__BYTE_ORDER == __LITTLE_ENDIAN) {
		printf("Host architecutre is little-endian.\n");
	} else {
		printf("Host architecture is big-endian.\n");
	}

	/* Open a file with read/write permissions. */
	fd = open("example.txt",O_RDWR);

	/* Error checking for write. I/O functions tend to return
       -1 if something is wrong. */
	if(fd == -1) {
		perror("example.txt");
		return -1;
	}
	
	/* Read a number from the file. This number is in little-endian. */
	result = read(fd,&read_num,sizeof(read_num));
	
	/* Error checking for read */
	if(result==-1) {
		perror("read");
		close(fd);
		return -1;
	}

	printf("We read little-endian number hex:0x%x, decimal:%i\n",
			read_num, read_num);

	/* Let's turn it into the host order (correct) number. This could
	 * be in little-endian or big-endian format, depending on our
	 * architecture.  The nice thing is that we don't have to care
	 * when we write our code - the functions already "know" our
	 * host order. */
	convert =  le16toh(read_num);
	printf("That number is actually ");
	printf("hex: 0x%x, decimal: %i\n", convert, convert);


	/* Add 1 to the number (just an example) */
	convert++;
	printf("New number we write to the file is:%i\n",
			convert);

	/* Convert this number back to little-endian for writing */
	write_num = htole16(convert);

	/* Let's re-write this number to the file at the beginning 
         * of the file. Look at 'man lseek' for more info. You
	 *  will probably want to change the write position to other areas
	 *  in your fat file. */
	result = lseek(fd, 0, SEEK_SET);

	/* Error checking for lseek */
	if(result == -1){
		perror("lseek");
		close(fd);
		return -1;
	}

	/* Write */
	result = write(fd, &write_num, sizeof(write_num));

	/* Error checking for write */
	if(result == -1){
		perror("write");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}
