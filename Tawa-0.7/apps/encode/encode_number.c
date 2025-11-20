/* PPM encoder for numbers. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "coder.h"
#include "text.h"
#include "table.h"
#include "io.h"
#include "sss_model.h"
#include "model.h"

unsigned int Max_Order = 0;
unsigned int numbers_input = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options]\n"
	     "\n"
	     "options:\n"
	     "  -o n\torder of model=n (required argument)\n"
	     "  -p n\tdebug progress=n\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean max_order_found = FALSE;
    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "o:p:")) != -1)
	switch (opt)
	{
	case 'o':
	    Max_Order = atoi (optarg);
	    max_order_found = TRUE;
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!max_order_found)
      {
	fprintf (stderr, "Fatal error: max order argument not found\n");
	exit (2);
      }
    for (; optind < argc; optind++)
	usage ();
}

int
getNumber (FILE *fp, unsigned int *number)
/* Returns the next number from input stream fp. */
{
    unsigned int n;
    int result;

    n = 0;

    result = fscanf (fp, "%u", &n);
    switch (result)
      {
      case 1: /* one number read successfully */
	break;
      case EOF: /* eof found */
	break;
      case 0:
	fprintf (stderr, "Formatting error in file\n");
	exit (1);
	break;
      default:
	fprintf (stderr, "Unknown error (%i) reading file\n", result);
	exit (1);
      }

    *number = n;
    return (result);
}

void
encodeNumbers (FILE *fp, unsigned int model, unsigned int model1,
	       unsigned int coder, unsigned int table)
/* Encodes numbers using the characters in FP. */
{
    unsigned int number, number_text, number_id, number_count;
    unsigned int context, context1;
    boolean new_number;

    number_text = TXT_create_text ();

    context  = TLM_create_context (model);
    context1 = TLM_create_context (model1);

    numbers_input = 0;
    for (;;)
    {
	if ((Debug.progress > 0) && ((numbers_input % Debug.progress) == 0))
	    fprintf (stderr, "encoding pos %d bytes output %d (%.3f bpn)\n",
		     numbers_input, bytes_output, bytes_output * 8.0 / numbers_input);

        /* repeat until EOF */
        if (getNumber (fp, &number) == EOF)
	    break;

	numbers_input++;
	TXT_setlength_text (number_text, 0);
	TXT_sprintf_text (number_text, "%d", number);

	new_number = TXT_update_table
	    (table, number_text, &number_id, &number_count);
	TLM_encode_symbol (model, context, coder, number_id);
	if (new_number)
	    TLM_encode_symbol (model1, context1, coder, number);

	if (Debug.level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
    }
    /* Encode eof: */
    TLM_encode_symbol (model, context, coder, TXT_sentinel_symbol ());

    TXT_release_text (number_text);

    TLM_release_context (model, context);
    TLM_release_context (model1, context1);

    fprintf (stderr, "Encoded %d numbers\n", numbers_input);
}

int
main (int argc, char *argv[])
{
    unsigned int Model, Model1, Coder, Table;

    init_arguments (argc, argv);

    arith_encode_start (Stdout_File);

    Model  = TLM_create_model (TLM_PPM_Model, "Fred", 0, Max_Order, TLM_PPM_Method_D, TRUE);
    Model1 = TLM_create_model (TLM_SSS_Model, "Fred", 0, 2, 32);

    Coder = TLM_create_arithmetic_coder ();

    Table = TXT_create_table (TLM_Dynamic, 0);

    encodeNumbers (stdin, Model, Model1, Coder, Table);

    arith_encode_finish (Stdout_File);

    fprintf (stderr, "Numbers input %d, bytes output %d (%.3f bits per number)\n",
	     numbers_input, bytes_output,
	     (8.0 * bytes_output) / numbers_input);

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
