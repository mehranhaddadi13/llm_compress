/* Utility module definitions. */

#include "model.h"

#ifndef MODEL_IO_H
#define MODEL_IO_H

#define MAX_INTEGER (1 << 30) /* Maximum integer/pointer permitted in trie */
#define INT_SIZE (sizeof (unsigned int)) /* Number of bytes needed to store an integer */

extern unsigned int debugIOLevel;

extern unsigned int IOBytesIn;
extern unsigned int IOBytesOut;

#define Stdin_File 1            /* File associated with stdin */
#define Stdout_File 2           /* File associated with stdout */
#define Stderr_File 3           /* File associated with stderr */

/* The following constants define the operations for TXT_set_file (): */
#define File_Exit_On_Errors 1   /* Exits with a run-time error when a
				   file error has been encountered */
#define File_Ignore_Errors 2    /* Inores any file errors */

extern FILE **Files;         /* Pointers to file pointers */

typedef enum { MEM_MALLOC_TYPE, MEM_REALLOC_TYPE, MEM_FREE_TYPE } memoryType;

float
log_two (float x);

float
Codelength (unsigned int lbnd, unsigned int hbnd, unsigned int totl);

unsigned int getBytes (unsigned int n);
/* Returns the number of bytes needed to store number n. */

unsigned int
bsize_int (unsigned int n);
/* Return the number of bytes needed to store the number n. */

unsigned int
bsize_int1 (unsigned int n);
/* Returns the number of bytes (from 1 to 4) that is needed to store integer n. */

void
bcopy_int (unsigned char *buf, unsigned int p, unsigned char *buf1, unsigned int p1, unsigned int b);
/* Copies b bytes of the integer in buf1 at position p1 into buf at position p. */

unsigned int
bread_int (unsigned char *buf, unsigned int p, unsigned int bytes);
/* Read in the bytes of integer n from buffer buf starting from position p
   and return the number. */

void
bwrite_int (unsigned char *buf, unsigned int n, unsigned int p,
	    unsigned int bytes);
/* Write out the bytes of integer n to the buffer buf starting from
   position p. */

unsigned int
fread_int (unsigned int file, unsigned int bytes);
/* Read in the bytes of integer from the file and return it. */

void
fwrite_int (unsigned int file, unsigned int n, unsigned int bytes);
/* Write out the bytes of integer n to the file. */

char *
fread_str (unsigned int file);
/* Read in the null terminated string from the file. */

void
fwrite_str (unsigned int file, char *s);
/* Write out the null-terminated string s to the file. */

unsigned int
fread_numbers1 (unsigned int file, unsigned int array_size,
		unsigned int *nstring);
/* Read in size unsigned integers from the file (one per line,
   character %u format). */

void
fread_numbers (unsigned int file, unsigned int array_size,
	       unsigned int *nstring);
/* Read in size unsigned integers (in 4 bytes each) from the file. */

void
fwrite_numbers (unsigned int file, unsigned int array_size, unsigned int *nstring);
/* Write out size unsigned integers from the file. */

unsigned int
bread_int1 (unsigned char *buf, unsigned int p, unsigned int *b);
/* Read an integer n from buffer buf starting from position p. Also return the number of bytes b
   that was used to store it. */

void
bwrite_int1 (unsigned int n, unsigned char *buf, unsigned int p, unsigned int *b);
/* Write integer n to buffer buf starting from position p. Also return the number of bytes b
   that was used to store it. */

void
add_memory (memoryType memory_type, unsigned int memory_id, int increment);
/* Adds the increment to the memory usage. */

unsigned int
get_memory (unsigned int memory_id);
/* Returns the memory usage. */

void
dump_memory (unsigned int file);
/* Dumps out the memory usage. */

void *
Malloc (unsigned int memory_id, size_t size);

void *
Calloc (unsigned int memory_id, size_t number, size_t size);

void *
Realloc (unsigned int memory_id, void *ptr, size_t size, size_t old_size);

void
Free (unsigned int memory_id, void *ptr, size_t size);

void
TXT_init_files ();
/* Initialises the Files array and Stdin_File, Stdout_File and Stderr_Files. */

boolean
TXT_valid_file (unsigned int file);
/* Returns TRUE if the table is valid, FALSE otherwize. */

void
TXT_set_file (unsigned int operation);
/* Sets various flags for the file functions

   File_Exit_On_Errors 1  (Default) Exits with a run-time error when a
				    file error has been encountered
   File_Ignore_Errors 2             Inores any file errors
*/

unsigned int
TXT_open_file (const char *filename, const char *mode, const char *debug_line, const char *error_line);
/* Opens the file with the specified mode. Returns an unsigned integer
   as a reference to the file pointer. */

void
TXT_close_file (unsigned int file);
/* Opens the file with the specified mode. Returns an unsigned integer
   as a reference to the file pointer. */

void
TXT_write_file (unsigned int file, char *string);
/* Writes out the string to the file. */

void
startClock (int id);
/* Start up a clock timer for debug purposes. */

void
finishClock (int id);
/* Finish up the clock timer for debug purposes. */

void
initClocks ();
/* Initialize the clocks for debugging purposes. */

void
dumpClocks (FILE *fp);
/* Dump out the elapsed times for all the clocks */

#endif

