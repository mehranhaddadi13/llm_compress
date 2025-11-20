""" Marks up the input file using multiple PPM models. """

from pyTawa.TLM import *
from pyTawa.TXT import *
from pyTawa.TAR import *

import sys, getopt


def usage ():
    print(
	"Usage: markup [options] training-model <input-text",
	"",
	"options:",
	"  -V\tUse Viterbi algorithm\n",
	"  -F\tOutput format is transform (not TMT format)",
	"  -l n\tdebug level=n",
	"  -d n\tdebug paths=n",
	"  -p n\tdebug progress=n",
	"  -m fn\tmodel filename=fn (required argument)",
	"  -1 n\tDepth of stack=n",
	"  -2 n\tPath extension=n",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-1': ('Stack Depth', 'Int'),
        '-2': ('Stack Path Extension', 'Int'),
	'-V': ('Viterbi', True),
	'-D': ('Dynamic', True),
	'-F': ('Formatted', True),
        '-p': ('Debug Progress', 'Int'),
	'-l': ('Debug Level', 'Int'),
	'-d': ('Debug Paths', 'Int'),
	'-m': ('Models Filename', 'Str'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    ok = TAR_init_arguments (arguments,
        "1:2:FVa:d:l:m:o:p:", ["help"], opts_dict,
        "Markup: option not recognized")
    if (not ok):
        usage()
        sys.exit(2)

    if (not TAR_check_required_arguments (arguments, opts_dict,
        ['Models Filename'])):
        usage ()
        sys.exit(2)

def dump_text (fp, text, formatted = True):
    """ Dumps out the text to the file fp. """

    if not (TXT_valid_text (text)):
        print ("Error: Invalid text", file=sys.stderr)
        sys.exit (2);

    sentinel = TXT_sentinel_symbol ()

    model = NIL
    for p in range(TXT_length_text (text)):
        symbol = TXT_getsymbol_text (text, p)
        if (symbol == sentinel):
	    if (model):
	        if (formatted):
	            print (fp, "<\\%s>", TLM_get_tag (model))
            p += 1
	    symbol = TXT_getsymbol_text (text, p)
	    if (symbol == TRANSFORM_MODEL_TYPE):
                p += 1
		symbol = TXT_getsymbol_text (text, p)
		model = symbol
		if (model):
		    if (formatted):
		        fprintf (fp, "<%s>", TLM_get_tag (model));
		    else:
		        fprintf (fp, "\\%d\\%c", sentinel, model);
	elif (symbol == '\n') or (symbol == '\t'):
	    fprintf (fp, symbol);
	elif (!formatted and (symbol == '\\')):
	    fprintf (fp, "\\\\");
	"""
        elif ((symbol <= 31) or (symbol >= 127)):
	    fprintf (fp, "<%d>", symbol);
	"""
	else
	  fprintf (fp, "%c", symbol);

void
transformFile (unsigned int transform_model, unsigned int file)
/* Marks up the text from the file using the transform model and dynamic language model. */
{
    unsigned int input_text;    /* The input text to correct */
    unsigned int transform_text;   /* The marked up text */
    unsigned int model;         /* A language model */

    /*dump_memory (stderr);*/

    input_text = TXT_load_text (file);
    /*TXT_setlength_text (input_text, TXT_length_text (input_text)-1);*/

    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
        TTM_start_transform (transform_model, TTM_transform_multi_context, input_text,
			  model);

    transform_text = TTM_perform_transform (transform_model, input_text);

    /*
      printf ("Input  = ");
      TXT_dump_text (stdout, input_text, TXT_dump_symbol);
      printf ("\n");
    */

    dump_text (stdout, transform_text);

    TXT_release_text (transform_text);
    TXT_release_text (input_text);

    /*dump_memory (stderr);*/
}

int
main (int argc, char *argv[])
{
    unsigned int model, transform_model, dynamic_model;
    int max_order;
    char title [48];

    init_arguments (argc, argv);

    if (Dynamics)
      {
	/* Add extra symbols for each model; these will be used to signal
	   a switch into a new model */
	Alphabet_size += TLM_numberof_models () + Dynamics + 2;

	for (max_order=-1; max_order <= (int) Dynamics; max_order++)
	  {
	    dynamic_model = TLM_create_model
	      (TLM_PPM_Model, DYNAMIC_TITLE, Alphabet_size,
	       max_order, ESCAPE_METHOD, PERFORMS_EXCLS);
	    sprintf (title, "Dynamic, %2d", max_order);
	    TLM_set_tag (dynamic_model, title);
	  }
      }
    else if (Dynamic)
      {
	/* Add extra symbols for each model; these will be used to signal
	   a switch into a new model */
	Alphabet_size += TLM_numberof_models () + 1;

        dynamic_model = TLM_create_model
	  (TLM_PPM_Model, DYNAMIC_TITLE, Alphabet_size, Max_order,
	   ESCAPE_METHOD, PERFORMS_EXCLS);
	TLM_set_tag (dynamic_model, "Dynamic");
      }

    if (Viterbi)
        transform_model = TTM_create_transform (TTM_Viterbi);
    else
	transform_model = TTM_create_transform (TTM_Stack, TTM_Stack_type0,
					  Stack_depth, Stack_path_extension);

    TTM_add_transform (transform_model, 0.0, "%w", "%w"); /* add transform for every character first */

    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
      TTM_add_transform (transform_model, 0.0, "%w", "%m%w", model); /* add transform for every model */

    if (Debug.level1 > 2)
      TTM_debug_transform (dumpPathSymbols, NULL, NULL);

    if (Debug.level1 > 4)
        TTM_dump_transform (Stderr_File, transform_model);

    transformFile (transform_model, Stdin_File);

    /*TTM_release_transform (transform_model); - fudge for segmentation fault */
    /*TLM_release_models (); - fudge for segmentation fault */

    /*dump_memory (stderr);*/

    exit (0);
}
