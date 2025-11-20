/* Loads or creates a model, adds some text to the model, then writes out the changed
   model. For testing the loading and dumping model routines. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "ppm_model.h"

#define MODEL_ALPHABET_SIZE 256           /* Size of the model's alphabet */
#define MODEL_TITLE "Sample PPMD5 model"  /* Title of model - this could be anything you choose */
#define MODEL_MAX_ORDER 5                 /* Default max. order of the model */
#define BREAK_SYMBOL 10                   /* Symbol used for forcing a break (usually an eoln) */

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

unsigned int Input_file;
char Input_filename [MAX_FILENAME_SIZE];
unsigned int Output_file;
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Model;
boolean Model_found = FALSE, Alphabet_found = FALSE, Title_found = FALSE,
    Max_order_found = FALSE, Escape_method_found;
unsigned int Model_alphabet_size = MODEL_ALPHABET_SIZE;
unsigned int Model_escape_method = TLM_PPM_Method_D;
int Model_max_order = 0;
unsigned int Max_input_size = 0;
boolean Model_performs_full_excls = 1;
boolean Model_performs_update_excls = 1;

char *Model_title = NULL;
char *Model_filename = NULL;
char *Model_parameters;


unsigned int Load_Numbers = 0;
unsigned int Static_Model = 0;
unsigned int Break_Eoln = 0;
unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options]\n"
	     "\n"
	     "options:\n"
	     "  -B\tforce a break at each eoln\n"
	     "  -N\tinput text is a sequence of unsigned numbers\n"
	     "  -S\twrite out the model as a static model\n"
	     "  -T n\tlong description (title) of model (required argument)\n"
	     "  -U\tdo not perform update exclusions\n"
	     "  -X\tdo not perform full exclusions\n"
	     "  -a n\tsize of alphabet=n (required)\n"
		 "  -d n\tdebugging level=n\n"
		 "  -e n\tescape method=c\n");
    fprintf (stderr,
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -m fn\tmodel filename=fn (optional)\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -O n\tmax order of model=n (required)\n"
	     "  -p n\tprogress report every n chars.\n"
	     "  -t n\ttruncate input size after n bytes\n"
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

    boolean Input_found = FALSE, Output_found = FALSE;

    /* get the argument options */

    Model_found = FALSE;
    Alphabet_found = FALSE;
    Escape_method_found = FALSE;
    Title_found = FALSE;
    Max_order_found = FALSE;
    Model_performs_full_excls = TRUE;
    Model_performs_update_excls = TRUE;
    Debug_level = 0;
    while ((opt = getopt (argc, argv, "BNST:UXa:d:e:i:m:o:O:p:t:")) != -1)
	switch (opt)
	{
	case 'B':
	    Break_Eoln = TRUE;
	    break;
	case 'N':
	    Load_Numbers = TRUE;
	    break;
	case 'S':
	    Static_Model = TRUE;
	    break;
	case 'T':
	    Title_found = (strlen (optarg) > 0);
	    if (Title_found)
	      {
		Model_title = (char *) malloc (strlen (optarg)+1);
		strcpy (Model_title, optarg);
	      }
	    break;
	case 'U':
	    Model_performs_update_excls = FALSE;
	    break;
	case 'X':
	    Model_performs_full_excls = FALSE;
	    break;
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
	case 'i':
	    Input_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'm':
	    if (strlen (optarg) > 0)
	      {
		Model_found = TRUE;
		Model_filename = (char *) malloc (strlen (optarg)+1);
		strcpy (Model_filename, optarg);
	      }
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'O':
	    Max_order_found = TRUE;
	    Model_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	case 't':
	    Max_input_size = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}
    if (!Model_found)
      {
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
        fprintf (stderr, "\nCreating new model\n\n");
      }
    else
      {
	if (Title_found)
	    TLM_set_load_operation (TLM_Load_Change_Title, Model_title);
	Model =
	    TLM_read_model (Model_filename, "Loading model from file",
			    "Train: can't open model file");
      }

    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void
train_model (unsigned int file, unsigned int model)
/* Trains the model from the characters in the FILE. */
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
	  fprintf (stderr, "training pos %d size of model %d\n", p, TLM_sizeof_model (model));
        /* repeat until EOF or max input */
	if (Max_input_size && (p >= Max_input_size))
	    break;
        if (TXT_getsymbol_file (file, Load_Numbers))
	    sym = TXT_Input_Symbol;
	else
	    break;

	if (Break_Eoln && (sym == BREAK_SYMBOL))
	    sym = TXT_sentinel_symbol ();

	TLM_update_context (model, context, sym);

	if (Debug_level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
    }

    TLM_update_context (model, context, TXT_sentinel_symbol ());
    TLM_release_context (model, context);

    fprintf (stderr, "Trained on %d symbols\n", p);
}

int
main (int argc, char *argv[])
{
    unsigned int model_type;
    unsigned int model_form;
    unsigned int model_alphabet_size;
    unsigned int max_order;
    int model_max_order;
    char *model_title;

    init_arguments (argc, argv);

    Input_file = TXT_open_file
      (Input_filename, "r", "Reading input file",
       "Train: can't open input file" );
    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Train: can't open output file" );

    if (Model_found)
      {
	if (!TLM_get_model_type (Model, &model_type, &model_form, &model_title))
	  {
	    fprintf (stderr, "Fatal error: Invalid model number\n");
	    exit (1);
	  }
	else if (model_form == TLM_Static)
	  {
	    fprintf (stderr, "Fatal error: This implementation does not permit further training when\n");
	    fprintf (stderr, "a static model has been loaded\n");
	    exit (1);
	  }

	TLM_get_model (Model, PPM_Get_Alphabet_Size, &model_alphabet_size);
	/* Check for consistency of parameters between the loaded model and the model to be written out */
	if (Alphabet_found && (model_alphabet_size != Model_alphabet_size))
	  {
	    fprintf (stderr, "\nFatal error: alphabet sizes of output model does not match input model\n\n");
	    exit (1);
	  }

	TLM_get_model (Model, PPM_Get_Max_Order, &max_order);
	model_max_order = (int) max_order;
	if (Max_order_found && (model_max_order != Model_max_order))
	  {
	    fprintf (stderr, "\nFatal error: max order of output model does not match input model\n\n");
	    exit (1);
	  }
      }
    else
      {
        Model = TLM_create_model
	  (TLM_PPM_Model, Model_title, Model_alphabet_size, Model_max_order,
	   Model_escape_method, Model_performs_full_excls, Model_performs_update_excls);
      }

    train_model (Input_file, Model);
    if (Static_Model)
        TLM_write_model (Output_file, Model, TLM_Static);
    else
        TLM_write_model (Output_file, Model, TLM_Dynamic);

    if (Debug_level > 0)
        TLM_dump_model (Stderr_File, Model, NULL);

    TLM_release_model (Model);

    exit (0);
}
