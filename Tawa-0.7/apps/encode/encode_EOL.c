/* Dynamic PPMD encoder (based on characters), plus special EOL encoding */
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

boolean Dump_Model = FALSE;

void
usage ()
{
    fprintf (stderr,
	     "Usage: encode1 [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -a n\tsize of alphabet=n\n"
	     "  -d\tdump model after every update\n"
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

    while ((opt = getopt (argc, argv, "a:de:m:o:p:r")) != -1)
	switch (opt)
	{
        case 'a':
	    Args_alphabet_size = atoi (optarg);
	    break;
	case 'd':
	    Dump_Model = TRUE;
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

unsigned int EOL_count = 0, EOL_count1 = 0;

void
encodeText (unsigned int model, unsigned int coder)
/* Encodes the text using the ppm model. */
{
    int cc, pos;
    unsigned int context, context1, context2, symbol, symbol1;

    context = TLM_create_context (model);
    context1 = TLM_clone_context (model, context);
    context2 = TLM_clone_context (model, context);

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
	symbol1 = symbol;
	if (symbol == '\n')
	  {
	    bytes_input++;
	    pos++;

	    cc = getc (stdin);
	    if (cc != EOF)
	      symbol1 = cc;
	    else
	      symbol1 = TXT_sentinel_symbol ();
	  }

	if (Debug.range != 0)
	  {
	    if (cc == EOF)
	      fprintf (stderr, "Encoding sentinel symbol\n");
	    else if (symbol != '\n')
	      fprintf (stderr, "Encoding symbol %d (%c)\n", symbol, symbol);
	    else
	      {
		fprintf (stderr, "Encoding EOL symbol %d (%c)\n", symbol, symbol);
		fprintf (stderr, "Encoding symbol %d (%c)\n", symbol1, symbol1);
	      }
	  }

	TLM_overlay_context (model, context, context1);/* Keep a copy of the context */
	TLM_overlay_context (model, context, context2);/* Keep another copy of the context */

	if (symbol != '\n')
	    TLM_encode_symbol (model, context, coder, symbol);
	else
	  {
	    float codelength;

	    TLM_find_symbol (model, context, symbol);
	    codelength =  TLM_Codelength;
	    TLM_find_symbol (model, context, ' ');
	    if (codelength < TLM_Codelength)
	      {
	        EOL_count++;
	        TLM_encode_symbol (model, context, coder, symbol);
	        TLM_encode_symbol (model, context, coder, symbol1);
	      }
	    else
	      {
	        EOL_count1++;
		TLM_encode_symbol (model, context, coder, '\r'); /* (fudge) encode speical EOL symbol */

		/* Replace the encoded special EOL symbol with a space in the copied context;
		   the model will then be updated twice for the "same" symbol */
		TLM_update_context (model, context1, ' '); /* extend copied context */

		TLM_overlay_context (model, context1, context); /* replace with extended context */
	        TLM_encode_symbol (model, context, coder, symbol1);

		/* Also update statistics for the EOL symbol as well;
		   the model will then be updated three times for the "same" symbol */
		TLM_update_context (model, context2, '\n');

	      }
	    /* fprintf (stderr, "Found EOL symbol at position %d EOL %.3f Space %.3f (%d/%d)\n",
	       pos, codelength, TLM_Codelength, EOL_count, (EOL_count + EOL_count1));*/
	  }

	if (Dump_Model)
	  {
	    fprintf (stderr, "Dump of model after position %d updated:\n", pos);
	    TLM_dump_model (Stderr_File, model, NULL);
	  }

	if ((symbol == TXT_sentinel_symbol ()) || (symbol1 == TXT_sentinel_symbol ()))
	    break;
    }
    if (Debug.progress > 0)
        fprintf (stderr, "position %d ", pos);

    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int Model, Coder;

    init_arguments (argc, argv);

    arith_encode_start (Stdout_File);

    if (!Args_model_filename)
        Model = TLM_create_model (TLM_PPM_Model,
				  Args_title, Args_alphabet_size, Args_max_order,
				  Args_escape_method, Args_performs_full_excls,
				  Args_performs_update_excls
				  );
    else /* load existing model */
	Model = TLM_read_model (Args_model_filename, NULL, NULL);

    Coder = TLM_create_arithmetic_coder ();

    encodeText (Model, Coder);

    arith_encode_finish (Stdout_File);

    fprintf (stderr, "Bytes input %d, bytes output %d (%.3f bpc)\n", bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);

    fprintf (stderr, "Percentage of EOL contexts with space better: %.3f\n",
	     100.0 * (float) EOL_count1 / (EOL_count + EOL_count1));

    /*fprintf (stderr, "Size of model        = %d\n", TLM_sizeof_model (Model));
      fprintf (stderr, "Min. length of model = %d\n", TLM_minlength_model (Model));*/

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
