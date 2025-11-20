/* Utility routines. */
#include <stdio.h>
#include "stdlib.h"
#include <strings.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "io.h"

#define FILES_SIZE 100        /* Default size of Files array */

/* The following definitions are used by routines that read/write compact integers from a buffer
   of varying length from 1 to 4 bytes. The first two bits in the first byte contains its length.
   The remaining bits (i.e. the six bits in the first byte plus the remaining 0-3 bytes) are used to
   store the integer (up to MAX_INTEGER). */
#define INT_BYTES 4           /* Lower 2 bits of first byte are used to store the number of bytes */
#define INT_FIRSTBYTE 64      /* Upper 6 bits of first byte are used to store the first
				 part of the actual number */

#define CLOCKS 100            /* Maximum number of clocks (used for debugging timing) */

clock_t ClockStartTimes [CLOCKS]; /* Used for debug timing */
float ClockElapsedTimes [CLOCKS]; /* Used for debug timing */

unsigned int debugIOLevel = 0;

unsigned int IOBytesIn = 0;
unsigned int IOBytesOut = 0;

int *Memory;          /* Statistics kept for memory usage */
int *Memory_mallocs;  /* Number of mallocs  */
int *Memory_reallocs; /* Number of reallocs  */
int *Memory_frees;    /* Number of frees  */
unsigned int Memory_alloc = 0; /* How much of the Memory array has been allocated */

FILE **Files;         /* Pointers to file pointers */
unsigned int Files_unused = 1; /* Next unused file in the Files array */
unsigned int Files_used = NIL; /* Next used file in the Files array */
unsigned int Files_alloc = 0;  /* How much of Files array has been allocated */

boolean File_Ignore_Errors_Flag = FALSE; /* if TRUE, file open errors will be ignored */

#define LN2 0.69314718055994530941

float
log_two (float x)
{
    return (log (x) / LN2);
}

float
Codelength (unsigned int lbnd, unsigned int hbnd, unsigned int totl)
/* Return the codelength of the coding range (lbnd,hbnd,totl). */
{
    if (!lbnd && (hbnd == totl))
        return (0.0);
    else
        return (-log_two ((float) (hbnd - lbnd) / (totl)));
}

void checkBytes (unsigned int n, unsigned int bytes)
/* Checks the number of bytes for number n is valid. */
{ 
    assert ((bytes > 0) && (bytes <= 4));
    switch (bytes)
    {
      case 1:
	assert (n < 256);
	break;
      case 2:
	assert (n < 65536);
	break;
      case 3:
	assert (n <= 16777216);
	break;
      case 4:
	break;
    }
}

unsigned int getBytes (unsigned int n)
/* Returns the number of bytes needed to store number n. */
{
    if (n < 256)
        return (1);
    else if (n < 65536)
        return (2);
    else if (n < 16777216)
        return (3);
    else
        return (4);
}

unsigned int
bsize_int (unsigned int n)
/* Return the number of bytes needed to store the number n. */
{
    unsigned int bsize;

    bsize = 0;
    if (n <= 1)
        n = 1;
    else
        n--;
    while (n != 0)
    {
        n /= 256;
	bsize++;
    }
    return (bsize);
}

unsigned int
bsize_int1 (unsigned int n)
/* Returns the number of bytes (from 1 to 4) that is needed to store integer n. */
{
    unsigned int nn, bytes;

    bytes = 1;
    if (n >= INT_FIRSTBYTE)
    { /* need 2 or more bytes to store n */
	nn = n / INT_FIRSTBYTE;
	while (nn && (bytes <= 4))
	{
	    nn /= 256;
	    bytes++;
	}
	if (bytes > 4)
	    fprintf (stderr, "Fatal error: integer %d exceeds maximum integer %d\n",
		     n, MAX_INTEGER-1);
    }
    assert ((bytes >= 0) && (bytes <= 4));
    return (bytes);
}

void
bcopy_int (unsigned char *buf, unsigned int p, unsigned char *buf1, unsigned int p1, unsigned int b)
/* Copies b bytes of the integer in buf1 at position p1 into buf at position p. */
{
    unsigned int i;

    for (i=0; i<b; i++)
        buf [p+i] = buf1 [p1+i];
}

unsigned int
bread_int (unsigned char *buf, unsigned int p, unsigned int bytes)
/* Read in the bytes of integer n from buffer buf starting from position p
   and return the number. */
{
    unsigned int n, i, power;

    assert ((bytes > 0) && (bytes <= 4));

    if (debugIOLevel > 4)
      {
        fprintf (stderr, "Reading from buffer %d bytes:", bytes);
	for (i = 0; i < bytes; i++)
	    fprintf (stderr, " %d", buf [p+i]);
	fprintf (stderr, "\n");
      }

    n = 0;
    power = 1;
    for (i = 0; i < bytes; i++)
    {
	n += buf[p+i] * power;
	power *= 256;
    }
    checkBytes (n, bytes);
    return (n);
}

void
bwrite_int (unsigned char *buf, unsigned int n, unsigned int p,
	    unsigned int bytes)
/* Write out the bytes of integer n to the buffer buf starting from
   position p. */
{
    unsigned int i;

    checkBytes (n, bytes);
    if (n == 0)
      { /* treat zero as a special case for speed */
	for (i = 0; i < bytes; i++)
	    buf [p+i] = 0;
      }
    else
      {
	for (i = 0; i < bytes; i++)
	  {
	    buf [p+i] = (unsigned char) n % 256;
	    n /= 256;
	  }
      }

    if (debugIOLevel > 4)
      {
	fprintf (stderr, "Writing to buffer %d bytes:", bytes);
	for (i = 0; i < bytes; i++)
	    fprintf (stderr, " %d", buf [p+i]);
	fprintf (stderr, "\n");
      }
}

unsigned int
fread_int (unsigned int file, unsigned int bytes)
/* Read in the bytes of integer from the file and return it. */
{
    unsigned char buf[4];
    unsigned int n, i, power;

    assert (TXT_valid_file (file));

    assert ((bytes > 0) && (bytes <= 4));
    fread (buf, bytes, 1, Files [file]);
    IOBytesIn += bytes;

    if (debugIOLevel > 1)
      {
        fprintf (stderr, "Reading %d bytes:", bytes);
	for (i = 0; i < bytes; i++)
	    fprintf (stderr, " %d", buf [i]);
	fprintf (stderr, "\n");
      }

    n = 0;
    power = 1;
    for (i = 0; i < bytes; i++)
    {
	n += buf [i] * power;
	power *= 256;
    }
    checkBytes (n, bytes);
    return (n);
}

void
fwrite_int (unsigned int file, unsigned int n, unsigned int bytes)
/* Write out the bytes of integer n to the file. */
{
    unsigned char buf[4];
    unsigned int i;

    assert (TXT_valid_file (file));

    checkBytes (n, bytes);
    if (n == 0)
      { /* treat zero as a special case for speed */
	for (i = 0; i < bytes; i++)
	    buf [i] = 0;
      }
    else
      {
	for (i = 0; i < bytes; i++)
	  {
	    buf [i] = (unsigned char) n % 256;
	    n /= 256;
	  }
      }
    fwrite (buf, bytes, 1, Files [file]);
    IOBytesOut += bytes;

    if (debugIOLevel > 1)
      {
	fprintf (stderr, "Writing %d bytes:", bytes);
	for (i = 0; i < bytes; i++)
	    fprintf (stderr, " %d", buf [i]);
	fprintf (stderr, "\n");
      }
}

unsigned int
fread_numbers1 (unsigned int file, unsigned int array_size,
		unsigned int *nstring)
/* Read in size unsigned integers from the file (one per line,
   character %u format). */
{
    unsigned int number, p;
    int result;

    p = 0;
    for (;;)
      { /* Repeat until EOF or array_size exceeded */
        result = fscanf (Files [file], "%u", &number);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    return (p);
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    exit (1);
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	    break;
	  }
	
	nstring [p++] = number;
      }

    return (p);
}

void
fread_numbers (unsigned int file, unsigned int array_size,
	       unsigned int *nstring)
/* Reads in size unsigned integers (in 4 bytes each) from the file. */
{
    unsigned int i, p;

    for (p = 0; p < array_size; p++)
      {
	i = fread_int (file, sizeof (unsigned int));
	nstring [p] = i;
      }
}

void
fwrite_numbers (unsigned int file, unsigned int array_size,
		unsigned int *nstring)
/* Write out size unsigned integers to the file. */
{
    unsigned int i, p;

    for (p = 0; p < array_size; p++)
      {
	i = nstring [p];
	fwrite_int (file, i, sizeof (unsigned int));
      }
}

unsigned int
bread_int1 (unsigned char *buf, unsigned int p, unsigned int *b)
/* Read an integer n from buffer buf starting from position p. Also return the number of bytes b
   that was used to store it. */
{
    unsigned int n, i, bytes, power;

    n = (buf [p] / INT_BYTES);
    bytes = (buf [p] % INT_BYTES) + 1;
    power = INT_FIRSTBYTE;
    for (i = 1; i < bytes; i++)
    {
	n += buf[p+i] * power;
	power *= 256;
    }
    assert ((bytes >= 0) && (bytes <= 4));
    *b = bytes;

    return (n);
}

void
bwrite_int1 (unsigned int n, unsigned char *buf, unsigned int p, unsigned int *b)
/* Write integer n to buffer buf starting from position p. Also return the number of bytes b
   that was used to store it. */
{
    unsigned int nn, bytes;

    bytes = 1;
    if (n == 0)
        buf [p] = 0;
    else if (n < INT_FIRSTBYTE)
        buf [p] = n * INT_BYTES; /* only using 1 byte to store n */
    else
    { /* need 2 or more bytes to store n */
        bytes = 1;
	buf [p] = n % INT_FIRSTBYTE;
	nn = n / INT_FIRSTBYTE;
	while (nn && (bytes <= 4))
	{
	    buf [p+bytes] = nn % 256;
	    nn /= 256;
	    bytes++;
	}
	if (bytes > 4)
	    fprintf (stderr, "Fatal error: integer %d exceeds maximum integer %d\n",
		     n, MAX_INTEGER-1);

	/* now store the number of bytes in the lowest byte */
	buf [p] = (bytes-1) + buf [p] * INT_BYTES; 
    }
    assert ((bytes >= 0) && (bytes <= 4));
    *b = bytes;
}


char *
fread_str (unsigned int file)
/* Read in the null terminated string from the file. */
{
    unsigned int size, p;
    char cc, *s;

    assert (TXT_valid_file (file));

    p = 0;
    size = 0;
    s = NULL;
    while (((cc = fgetc (Files [file])) != EOF) && (cc != '\0'))
    {
	if (p >= size)
	{
	    size += 128;
	    s = (char *) realloc (s, size);
	    if (s == NULL)
	    {
		fprintf (stderr, "Fatal error: out of string space\n");
		exit (1);
	    }
	}
	s [p] = cc;
	p++;
    }
    assert (s != NULL);

    s [p] = '\0';
    return (s);
}

void
fwrite_str (unsigned int file, char *s)
/* Write out the null-terminated string s to the file. */
{
    assert (TXT_valid_file (file));

    fputs (s, Files [file]);
    fputc (0, Files [file]);
}


void
add_memory (memoryType memory_type, unsigned int memory_id, int increment)
/* Adds the increment to the memory usage. */
{
    unsigned int p, old_alloc;

    if ((Memory_alloc == 0) || (memory_id > Memory_alloc))
      {
	old_alloc = Memory_alloc;
	Memory_alloc = memory_id * 2;
	Memory          = (int *) realloc (Memory,          (Memory_alloc+2) * sizeof (int));
	Memory_mallocs  = (int *) realloc (Memory_mallocs,  (Memory_alloc+2) * sizeof (int));
	Memory_reallocs = (int *) realloc (Memory_reallocs, (Memory_alloc+2) * sizeof (int));
	Memory_frees    = (int *) realloc (Memory_frees,    (Memory_alloc+2) * sizeof (int));
	for (p = old_alloc+1; p <= Memory_alloc; p++)
	  {
	    Memory          [p] = 0;
	    Memory_mallocs  [p] = 0;
	    Memory_reallocs [p] = 0;
	    Memory_frees    [p] = 0;
	  }
      }
    Memory [memory_id] += increment;
    switch (memory_type)
      {
      case MEM_MALLOC_TYPE:
	Memory_mallocs [memory_id]++;
	break;
      case MEM_REALLOC_TYPE:
	Memory_reallocs [memory_id]++;
	break;
      case MEM_FREE_TYPE:
	Memory_frees [memory_id]++;
	break;
      default:
	break;
      }
    Memory [0] += increment;
}

unsigned int
get_memory (unsigned int memory_id)
/* Returns the memory usage. */
{
    assert (memory_id < Memory_alloc);
    return (Memory [memory_id]);
}

void
dump_memory (unsigned int file)
/* Dumps out the memory usage. */
{
    unsigned int total, p;

    assert (TXT_valid_file (file));

    if (Memory_alloc == 0)
      {
        fprintf (Files [file], "Total memory usage = 0 bytes\n");
	return;
      }

    total = Memory [0];
    fprintf (Files [file], "Total memory usage = %d bytes\n\n", total);
    for (p = 1; p <= Memory_alloc; p++)
      if (Memory [p])
	fprintf (Files [file],
		 "%3d: %5.1f%% (%9d) mallocs %8d reallocs %8d free %8d\n",
		 p, (100.0 * (float) Memory [p]/total), Memory [p],
		 Memory_mallocs [p], Memory_reallocs [p], Memory_frees [p]);
}

void *
Malloc (unsigned int memory_id, size_t size)
{
  /*fprintf (stderr, "Mallocing id %d size = %d\n", memory_id, size);*/
    add_memory (MEM_MALLOC_TYPE, memory_id, size);
    return (malloc (size));
}

void *
Calloc (unsigned int memory_id, size_t number, size_t size)
{
  /*fprintf (stderr, "Callocing id %d number = %d size = %d\n", memory_id, number, size);*/
    add_memory (MEM_MALLOC_TYPE, memory_id, size);
    return (calloc (number, size));
}

void *
Realloc (unsigned int memory_id, void *ptr, size_t size, size_t old_size)
{
  /*fprintf (stderr, "Reallocating id %d ptr %p size = %d\n", memory_id, ptr, size);*/
    add_memory (MEM_REALLOC_TYPE, memory_id, (int) size - old_size);
    return (realloc (ptr, size));
}

void
Free (unsigned int memory_id, void *ptr, size_t size)
{
    add_memory (MEM_FREE_TYPE, memory_id, (int) - size);
    free (ptr);
}

void
TXT_init_files ()
/* Initialises the Files array and Stdin_File, Stdout_File and Stderr_Files. */
{
    if (Files_alloc == 0)
      {
	Files_alloc = FILES_SIZE;
	Files = (FILE **) malloc (Files_alloc * sizeof (FILE *));

	Files [Stdin_File] = stdin;
	Files [Stdout_File] = stdout;
	Files [Stderr_File] = stderr;

	Files_unused = Stderr_File + 1;
      }
}

boolean
TXT_valid_file (unsigned int file)
/* Returns TRUE if the table is valid, FALSE otherwize. */
{
    if (file == NIL)
        return (FALSE);

    TXT_init_files ();

    if (file >= Files_unused)
        return (FALSE);
    else
        return (TRUE);
}

void
TXT_set_file (unsigned int operation)
/* Sets various flags for the file functions

   File_Exit_On_Errors 1  (Default) Exits with a run-time error when a
                                    file error has been encountered
   File_Ignore_Errors 2             Inores any file errors
*/
{
    switch (operation)
      {
      case File_Exit_On_Errors:
	File_Ignore_Errors_Flag = FALSE;
	break;
      case File_Ignore_Errors:
	File_Ignore_Errors_Flag = TRUE;
	break;
      default:
	fprintf (stderr, "Io error: invalid file operation %d\n",
		 operation);
      }
}
   
unsigned int
TXT_open_file (const char *filename, const char *mode, const char *debug_line, const char *error_line)
/* Opens the file with the specified mode. Returns an unsigned integer
   as a reference to the file pointer. */
{
    unsigned int file;
    FILE *fp;

    TXT_init_files ();

    if (debug_line != NULL)
        fprintf (stderr, "%s %s\n", debug_line, filename);

    if ((fp = fopen (filename, mode)) == NULL)
      {
	if (error_line != NULL)
	    fprintf (stderr, "%s %s\n", error_line, filename);
	else
	    fprintf (stderr, "Error: Can't open file %s\n", filename);
	if (!File_Ignore_Errors_Flag)
	    exit (1);
	else
	    return (NIL);
      }

    if (Files_used != NIL)
      { /* Pop one off the used list first */
	file = Files_used;
	Files_used = (unsigned int) Files [Files_used];
      }
    else
      {
	file = Files_unused++;
	if (file >= Files_alloc)
	  {
	    assert (Files_alloc != 0);
            Files_alloc *= 2;

	    Files = (FILE **) realloc (Files, Files_alloc * sizeof (FILE *));
	  }
      }

    Files [file] = fp;
    return (file);
}

void
TXT_close_file (unsigned int file)
/* Opens the file with the specified mode. Returns an unsigned integer
   as a reference to the file pointer. */
{
    assert (TXT_valid_file (file));

    TXT_init_files ();

    fclose (Files [file]);

    /* Place at the head of the used list */
    Files [file] = (FILE *) Files_used;
    Files_used = file;
}

void
TXT_write_file (unsigned int file, char *string)
/* Writes out the string to the file. */
{
    assert (TXT_valid_file (file));

    fprintf (Files [file], "%s", string);
}

void
startClock (int id)
/* Start up the clock timer for debug purposes. */
{
    assert (id < CLOCKS);

    ClockStartTimes [id] = clock ();
}

void
finishClock (int id)
/* Finish up the clock timer for debug purposes. */
{
    assert (id < CLOCKS);

    ClockElapsedTimes [id] += (float) (clock () - ClockStartTimes [id])/CLOCKS_PER_SEC;
}

void
initClocks ()
/* Initialize the clocks for debugging purposes. */
{
  int id;

    for (id = 0; id < CLOCKS; id++)
        ClockElapsedTimes [id] = 0;

    startClock (0);
}

void
dumpClocks (FILE *fp)
/* Dump out the elapsed times for all the clocks */
{
  int id;

    finishClock (0);
    for (id = 1; id < CLOCKS; id++)
      if (ClockElapsedTimes [id])
	fprintf (stderr, "Elapsed time for id %d = %.3f (%.2f%%)\n",
		 id, ClockElapsedTimes [id],
		 (100.0 * ClockElapsedTimes [id]/ClockElapsedTimes [0]));
}

/* Scaffolding for io module.
void
main (int argc, char *argv[])
{
    unsigned int n, n1, bytes, bytes1;
    unsigned char buf [4];

    for (n = 1071000000; n < MAX_INTEGER+1; n++)
    {
	bwrite_int1 (n, buf, 0, &bytes);
	n1 = bread_int1 (buf, 0, &bytes1);
        if ((n != n1) || (bytes != bytes1))
	    fprintf (stderr, "n = %d n1 = %d bytes = %d bytes1 = %d\n", n, n1, bytes, bytes1);
	assert ((n == n1) && (bytes == bytes1));
	if ((n % 1000000) == 0)
	    fprintf (stderr, "...%d\n", n);
    }
}
*/
