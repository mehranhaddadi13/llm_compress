/* Test arithmetic coder */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "coder.h"

unsigned int Iterations = 0;
unsigned int Low1 = 0, High1 = 0, Total1 = 0;
unsigned int Low2 = 0, High2 = 0, Total2 = 0;

void
usage ()
{
    fprintf (stderr,
	     "Usage: encode_test1 [options] >output\n"
	     "\n"
	     "options:\n"
	     "  -n n\tnumber of encodings to perform=n\n"
	     "  -l n\tlow of arithmetic coding range 1=n\n"
	     "  -h n\thigh of arithmetic coding range 1=n\n"
	     "  -t n\ttotal of arithmetic coding range 1=n\n"
	     "  -L n\tlow of arithmetic coding range 2=n\n"
	     "  -H n\thigh of arithmetic coding range 2=n\n"
	     "  -T n\ttotal of arithmetic coding range 2=n\n");
}

void
init_arguments (int argc, char *argv[])
/* Initializes the arguments from the arguments list. */
{
    extern char *optarg;
    extern int optind;
    int opt;

    while ((opt = getopt (argc, argv, "n:l:h:t:L:H:T:")) != -1)
	switch (opt)
	{
        case 'n':
	    Iterations = atoi (optarg);
	    break;
        case 'l':
	    Low1 = atoi (optarg);
	    break;
        case 'h':
	    High1 = atoi (optarg);
	    break;
        case 't':
	    Total1 = atoi (optarg);
	    break;
        case 'L':
	    Low2 = atoi (optarg);
	    break;
        case 'H':
	    High2 = atoi (optarg);
	    break;
        case 'T':
	    Total2 = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int i;

    init_arguments (argc, argv);

    arith_encode_start (Stdout_File);

    for (i = 0; i < Iterations; i++)
      {
	if (Total1 != 0)
	    arith_encode (Stdout_File, Low1, High1, Total1);
	if (Total2 != 0)
	    arith_encode (Stdout_File, Low2, High2, Total2);
      }

    arith_encode_finish (Stdout_File);

    return (0);
}
