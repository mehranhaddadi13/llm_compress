/* Static PPMD decoder (based on characters) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "coder.h"

unsigned int Model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: decode1 [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -p n\treport progress every n chars\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int model_found;
    int opt;

    /* get the argument options */

    model_found = 0;
    while ((opt = getopt (argc, argv, "d:m:r")) != -1)
	switch (opt)
	{
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'm':
	    Model = TLM_read_model (optarg, "Loading model from file",
				  "Decode: can't open model file");
	    model_found = 1;
	    break;
	case 'r':
	    Debug.range = 1;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!model_found)
    {
        fprintf (stderr, "\nFatal error: missing model\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}


void
decodeText (unsigned int model, unsigned int coder)
/* Decodes the text using the ppm model. */
{
    unsigned int context, symbol;

    context = TLM_create_context (model);

    for (;;) /* decode symbols until EOF found */
    {
        symbol = TLM_decode_symbol (model, context, coder);
	if (symbol == TXT_sentinel_symbol ())
	    break; /* EOF */
	
	fprintf (stdout, "%c", symbol);
    }
    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int Coder;

    Coder = TLM_create_arithmetic_coder ();

    init_arguments (argc, argv);

    arith_decode_start (Stdin_File);

    decodeText (Model, Coder);

    arith_decode_finish (Stdin_File);

    TLM_release_model (Model);

    return (0);
}
