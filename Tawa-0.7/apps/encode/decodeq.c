/* Dynamic PPMD quick encoder (based on characters) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "coder.h"
#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_OPTIONS 32
#define ARGS_MAX_ORDER 5       /* Default maximum order of the model */
#define ARGS_ALPHABET_SIZE 256 /* Default size of the model's alphabet */
#define ARGS_TITLE_SIZE 32     /* Size of the model's title string */
#define ARGS_TITLE "Sample PPMD model" /* Title of model - this could be anything you choose */

int Args_max_order;               /* Maximum order of the models */
unsigned int Args_escape_method;  /* Escape method for the model */
unsigned int Args_alphabet_size;  /* The models' alphabet size */
unsigned int Args_performs_full_excls; /* Flag to indicate whether the models perform full exclusions or not */
unsigned int Args_performs_update_excls; /* Flag to indicate whether the models perform update exclusions or not */

char *Args_title;                 /* The title of the primary model */

void
usage ()
{
    fprintf (stderr,
	     "Usage: decode [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -a n\tsize of alphabet=n\n"
	     "  -e c\tescape method for the model=c\n"
	     "  -o n\tmax order of model=n\n"
	     "  -p n\treport progress every n chars\n"
	     "  -r\tdebug arithmetic coding ranges\n");
}

void
init_arguments (int argc, char *argv[])
/* Initializes the arguments from the arguments list. */
{
    extern char *optarg;
    extern int optind;
    int opt, escape;

    Args_alphabet_size = ARGS_ALPHABET_SIZE;
    Args_max_order = ARGS_MAX_ORDER;
    Args_escape_method = TLM_PPM_Method_D;
    Args_performs_full_excls = TRUE;
    Args_performs_update_excls = TRUE;
    Args_title = (char *) malloc (ARGS_TITLE_SIZE * sizeof (char));

    while ((opt = getopt (argc, argv, "a:e:o:p:r")) != -1)
	switch (opt)
	{
        case 'a':
	    Args_alphabet_size = atoi (optarg);
	    break;
	case 'e' :
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method = escape;
	    break;
	case 'o':
	    Args_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = 1;
	    break;
	default:
	    usage ();
	    break;
	}

    for (; optind < argc; optind++)
	usage ();

    strcpy (Args_title, ARGS_TITLE);  
}

void
decodeText (unsigned int model, unsigned int coder)
/* Decodes the text using the ppm model. */
{
    int pos;
    unsigned int context, symbol;

    context = TLM_create_context (model);

    pos = 0;
    for (;;) /* decode symbols until sentinel symbol found */
    {
        pos++;
	if ((Debug.progress > 0) && ((pos % Debug.progress) == 0))
	    fprintf (stderr, "position %d\n", pos);

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
    unsigned int Model, Coder;

    init_arguments (argc, argv);

    arith_decode_start (Stdin_File);

    Model = TLM_create_model (TLM_PPMq_Model, Args_title,
			      Args_alphabet_size, Args_max_order,
			      Args_escape_method, Args_performs_full_excls,
			      Args_performs_update_excls);

    Coder = TLM_create_arithmetic_coder ();

    decodeText (Model, Coder);

    arith_decode_finish (Stdin_File);

    TLM_release_model (Model);

    return (0);
}
