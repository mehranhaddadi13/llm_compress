/* Writes out the codelength for the input file
   for the model specified by the -m argument. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

unsigned int Model;
float Total_Codelength = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: encode [options] training-model <input-text\n"
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
    while ((opt = getopt (argc, argv, "m:p:r")) != -1)
	switch (opt)
	{
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'm':
	    Model = TLM_read_model (optarg, "Loading model from file",
			    "Codelength1: can't open model file");
	    model_found = 1;
	    break;
	case 'r':
	    Debug.range = TRUE;
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
codelengthText (unsigned int model)
/* Encodes the text using the PPM model. */
{
    int cc, pos;
    unsigned int symbol;
    unsigned int context;

    context = TLM_create_context (model);
    TLM_set_context_operation (TLM_Get_Codelength); /* Compute codelength only */

    pos = 0;
    for (;;)
    {
        pos++;

        /* repeat until EOF */
        cc = getc (stdin);
	if (cc != EOF)
	    symbol = cc;
	else
	    symbol = TXT_sentinel_symbol ();

	if (Debug.range)
	    fprintf (stderr, "Symbol %d\n", symbol);
	TLM_update_context (model, context, symbol);
	/*printf ("pos %d symbol %d codelength %.4f\n", pos, symbol, TLM_Codelength);*/
	Total_Codelength += TLM_Codelength;

	if ((Debug.progress > 0) && ((pos % Debug.progress) == 0))
	    printf ("position %d codelength %.4f compress. ratio %.4f\n",
		    pos, Total_Codelength, Total_Codelength/pos);

	if (symbol == TXT_sentinel_symbol ())
	    break;
    }
    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);
    /*TLM_dump_model (Stdout_File, Model, NULL);*/

    codelengthText (Model);

    printf ("Total codelength = %.3f\n", Total_Codelength);

    TLM_release_model (Model);

    exit (0);
}
