/* Marks up the input file using multiple PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "model.h"
#include "transform.h"

#define DYNAMIC_TITLE "Dynamic model"
#define PERFORMS_EXCLS TRUE
#define MAX_ORDER 5       /* Maximum order of the model */
#define ALPHABET_SIZE 256 /* Default max. alphabet size for all PPM models */
#define ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for all PPM models */

unsigned int Viterbi = FALSE;
unsigned int Dynamic = FALSE;
unsigned int Formatted = FALSE;
unsigned int Dynamics = 0;
unsigned int Stack_depth = 0;
unsigned int Stack_path_extension = 0;

unsigned int Alphabet_size = ALPHABET_SIZE;
unsigned int Max_order = MAX_ORDER;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -V\tUse Viterbi algorithm\n"
	     "  -D\tInclude single dynamic model in transform\n"
	     "  -M n\tInclude multiple dynamic models in transform up to order n\n"
	     "  -F\tOutput format is transform (not TMT format)\n"
	     "  -l n\tdebug level=n\n"
	     "  -d n\tdebug paths=n\n"
	     "  -p n\tdebug progress=n\n"
	     "  -m fn\tmodel filename=fn (required argument)\n"
	     "  -o n\tmaximum order of dynamic model(s)\n"
	     "  -1 n\tDepth of stack=n\n"
	     "  -2 n\tPath extension=n\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    unsigned int models_found = FALSE;
    int opt;

    /* set defaults */
    Debug.level = FALSE;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "1:2:L:DF VM:a:o:d:l:m:p:")) != -1)
	switch (opt)
	{
	case '1':
	    Stack_depth = atoi (optarg);
	    break;
	case '2':
	    Stack_path_extension = atoi (optarg);
	    break;
	case 'V':
	    Viterbi = TRUE;
	    break;
	case 'D':
	    Dynamic = TRUE;
	    break;
	case 'F':
	    Formatted = TRUE;
	    break;
	case 'M':
	    Dynamics = atoi (optarg);
	    break;
	case 'a':
	    Alphabet_size = atoi (optarg);
	    break;
	case 'o':
	    Max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

void
dump_text (FILE *fp, unsigned int text)
{
    unsigned int textlen, symbol;
    unsigned int sentinel, model, p;

    assert (TXT_valid_text (text));

    sentinel = TXT_sentinel_symbol ();

    model = NIL;
    textlen = TXT_length_text (text);
    for (p = 0; p < textlen;)
    {
        TXT_get_symbol (text, p, &symbol);
	/*fprintf (fp, "[%d]", symbol);*/
	if (symbol == sentinel)
	  {
	    if (model)
	      if (Formatted)
	        fprintf (fp, "<\\%s>", TLM_get_tag (model));
	    TXT_get_symbol (text, ++p, &symbol);
	    if (symbol == TRANSFORM_MODEL_TYPE)
	      {
		TXT_get_symbol (text, ++p, &symbol);
		model = symbol;
		if (model)
		  {
		    if (Formatted)
		      fprintf (fp, "<%s>", TLM_get_tag (model));
		    else
		      fprintf (fp, "\\%d\\%c", sentinel, model);
		  }
	      }
	  }
	else if (symbol == '\n')
	  fprintf (fp, "\n");
	else if (symbol == '\t')
	  fprintf (fp, "\t");
	else if (!Formatted && (symbol == '\\'))
	  fprintf (fp, "\\\\");
	/*
	else if ((symbol <= 31) || (symbol >= 127))
	  fprintf (fp, "<%d>", symbol);
	*/
	else
	  fprintf (fp, "%c", symbol);
	p++;
    }
}

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
      TTM_add_transform (transform_model, 0.0, " %w", " %m%w", model); /* add transform for every model */

    /*TTM_dump_transform (stdout, transform_model);*/
    if (Debug.level1 > 2)
        TTM_debug_transform (dumpPathSymbols);

    transformFile (transform_model, Stdin_File);

    /*TTM_release_transform (transform_model); - fudge for segmentation fault */
    /*TLM_release_models (); - fudge for segmentation fault */

    /*dump_memory (stderr);*/

    exit (0);
}
