/* Dynamic PPMD encoder (based on characters) with special treatment for EOLs */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

char *Args_model_filename;        /* The filename of an existing model */
char *Args_title;                 /* The title of the primary model */

void
usage ()
{
    fprintf (stderr,
	     "Usage: encode1 [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -a n\tsize of alphabet=n\n"
	     "  -e c\tescape method for the model=c\n"
	     "  -o n\tmax order of model=n\n"
	     "  -m fn\tmodel filename=fn\n"
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

    while ((opt = getopt (argc, argv, "a:e:m:o:p:r")) != -1)
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
	case 'm':
	    Args_model_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_model_filename, optarg);  
	    break;
	case 'r':
	    Debug.range = TRUE;
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
encodeText (unsigned int model, unsigned int EOL_model, unsigned int coder)
/* Encodes the text using the ppm model. */
{
    int cc, pos;
    unsigned int context, EOL_context, symbol;

    context = TLM_create_context (model);
    EOL_context = TLM_create_context (EOL_model);

    pos = 0;
    for (;;)
    {
	bytes_input++;
        pos++;
	if ((Debug.progress > 0) && ((pos % Debug.progress) == 0))
	    fprintf (stderr, "position %d bytes input %d, bytes output %d (%.3f bpc)\n", pos, bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);

        /* repeat until EOF */
        cc = getc (stdin);
	if (cc != EOF)
	    symbol = cc;
	else
	    symbol = TXT_sentinel_symbol ();

	if (Debug.range != 0)
	  {
	    if (cc == EOF)
	      fprintf (stderr, "Encoding sentinel symbol\n");
	    else
	      fprintf (stderr, "Encoding symbol %d (%c)\n", symbol, symbol);
	  }

	if (symbol == '\n')
	  {
	    TLM_encode_symbol (model, context, coder, ' ');
	    TLM_encode_symbol (EOL_model, EOL_context, coder, 0);
	  }
	else if (symbol == ' ')
	  {
	    TLM_encode_symbol (model, context, coder, ' ');
	    TLM_encode_symbol (EOL_model, EOL_context, coder, 1);
	  }
	else
	    TLM_encode_symbol (model, context, coder, symbol);

	if (symbol == TXT_sentinel_symbol ())
	    break;
    }
    if (Debug.progress > 0)
        fprintf (stderr, "position %d ", pos);
    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int Model, EOL_Model, Coder;

    init_arguments (argc, argv);

    arith_encode_start (Stdout_File);

    EOL_Model = TLM_create_model (TLM_PPM_Model, "EOL model", 2, 0, TLM_PPM_Method_D, TRUE);
    if (!Args_model_filename)
        Model = TLM_create_model (TLM_PPM_Model,
				  Args_title, Args_alphabet_size, Args_max_order,
				  Args_escape_method, Args_performs_full_excls,
				  Args_performs_update_excls);
    else /* load existing model */
	Model = TLM_read_model (Args_model_filename, NULL, NULL);

    Coder = TLM_create_arithmetic_coder ();

    encodeText (Model, EOL_Model, Coder);

    arith_encode_finish (Stdout_File);

    fprintf (stderr, "Bytes input %d, bytes output %d (%.3f bpc)\n", bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);

    /*fprintf (stderr, "Size of model        = %d\n", TLM_sizeof_model (Model));
      fprintf (stderr, "Min. length of model = %d\n", TLM_minlength_model (Model));*/

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
