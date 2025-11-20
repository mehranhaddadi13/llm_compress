/* Creates a model in a "fast" manner, then writes out the changed
   model. For testing the loading and dumping model routines. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MODEL_ALPHABET_SIZE 256           /* Size of the model's alphabet */
#define MODEL_TITLE "Sample PPMD5 model"  /* Title of model - this could be anything you choose */
#define MODEL_MAX_ORDER 5                 /* Default max. order of the model */
#define BREAK_SYMBOL 10                   /* Symbol used for forcing a break (usually an eoln) */

unsigned int Model;
boolean Model_found = FALSE, Alphabet_found = FALSE, Escape_method_found = FALSE,
    Title_found = FALSE, Max_order_found = FALSE;
unsigned int Model_alphabet_size = MODEL_ALPHABET_SIZE;
unsigned int Model_escape_method = TLM_PPM_Method_C;
int Model_max_order = 0;
unsigned int Max_input_size = 0;

char *Model_title = NULL;
char *Model_parameters;


unsigned int Load_Numbers = 0;
unsigned int Break_Eoln = 0;
unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -a n\tsize of alphabet=n (required argument)\n"
             "  -d n\tdebugging level=n\n"
	     "  -o n\tmax order of model=n (required argument)\n"
	     "  -e c\tescape method of model=c (required argument)\n"
	     "  -p n\tprogress report every n chars.\n"
	     "  -t n\ttruncate input size after n bytes\n"
	     "  -B\tforce a break at each eoln\n"
	     "  -N\tinput text is a sequence of unsigned numbers\n"
	     "  -T n\tlong description (title) of model (required argument)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    int escape;
    int opt;
    extern char *optarg;
    extern int optind;

    /* get the argument options */

    Alphabet_found = FALSE;
    Title_found = FALSE;
    Max_order_found = FALSE;
    Escape_method_found = FALSE;
    Debug_level = 0;
    while ((opt = getopt (argc, argv, "BNT:a:d:e:o:p:t:")) != -1)
	switch (opt)
	{
	case 'a':
	    Alphabet_found = TRUE;
	    Model_alphabet_size = atoi (optarg);
	    break;
	case 'd':
	    Debug_level = atoi (optarg);
	    break;
	case 'e':
	    Escape_method_found = TRUE;
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Model_escape_method = escape;
	    break;
	case 'o':
	    Max_order_found = TRUE;
	    Model_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	case 't':
	    Max_input_size = atoi (optarg);
	    break;
	case 'B':
	    Break_Eoln = TRUE;
	    break;
	case 'N':
	    Load_Numbers = TRUE;
	    break;
	case 'T':
	    Title_found = (strlen (optarg) > 0);
	    if (Title_found)
	      {
		Model_title = (char *) malloc (strlen (optarg)+1);
		strcpy (Model_title, optarg);
	      }
	    break;
	default:
	    usage ();
	    break;
	}

    fprintf (stderr, "\nCreating new dynamic model\n\n");
    if (!Alphabet_found)
        fprintf (stderr, "\nFatal error: missing alphabet size of the model\n\n");

    if (!Escape_method_found)
        fprintf (stderr, "\nFatal error: missing escape method of the model\n\n");

    if (!Title_found)
        fprintf (stderr, "\nFatal error: missing title of the model\n\n");

    if (!Max_order_found)
        fprintf (stderr, "\nFatal error: missing max. order of the model\n\n");

    if (!Alphabet_found || !Escape_method_found || !Title_found || !Max_order_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

int
getSymbol (FILE *fp, unsigned int *symbol)
/* Returns the next symbol from input stream fp. */
{
    unsigned int sym;
    int result;

    sym = 0;
    if (Load_Numbers)
      {
        result = fscanf (fp, "%u", &sym);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	  }
      }
    else
      {
        sym = getc (fp);
	result = sym;
      }
    *symbol = sym;
    return (result);
}

void
train_model (FILE *fp, unsigned int model)
/* Trains the model from the characters in FP. */
{
    unsigned int p, sym, context;

    context = TLM_create_context (model);

    TLM_set_context_operation (TLM_Get_Nothing);

    if (Break_Eoln)
      { /* Start off the training with a sentinel symbol to indicate a break */
	TLM_update_context (model, context, TXT_sentinel_symbol ());
      }

    p = 0;
    for (;;)
    {
        p++;
	if ((Debug_progress > 0) && ((p % Debug_progress) == 0))
	    fprintf (stderr, "training pos %d\n", p);
        /* repeat until EOF or max input */
	if (Max_input_size && (p >= Max_input_size))
	  break;
        if (getSymbol (fp, &sym) == EOF)
	    break;

	if (Break_Eoln && (sym == BREAK_SYMBOL))
	    sym = TXT_sentinel_symbol ();

	TLM_update_context (model, context, sym);

	if (Debug_level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
    }
    TLM_release_context (model, context);

    fprintf (stderr, "Trained on %d symbols\n", p);
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    Model = TLM_create_model
      (TLM_PPMq_Model, Model_title, Model_alphabet_size, Model_max_order,
       Model_escape_method);

    train_model (stdin, Model);
    /*TLM_write_model (Stdout_File, Model, TLM_Static);*/

    if (Debug_level > 0)
        TLM_dump_model (Stderr_File, Model, NULL);

    TLM_release_model (Model);

    exit (0);
}
