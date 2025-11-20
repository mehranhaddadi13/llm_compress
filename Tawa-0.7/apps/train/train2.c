/* Creates a single dynamic model and multiple contexts that point to it,
   then tries to simultaneously update it for each context using text
   contained on successive lines just to test if it can be done, even
   though it will more then likely create one big mess (which may in fact
   be useful - e.g. for unsupervised learning of word segmentation.) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"

#define MODEL_ALPHABET_SIZE 256           /* Size of the model's alphabet */
#define MODEL_TITLE "Sample PPMD5 model"  /* Title of model - this could be anything you choose */
#define MODEL_MAX_ORDER 5                 /* Default max. order of the model */

unsigned int Model;
boolean Model_found = FALSE, Alphabet_found = FALSE, Escape_method_found = FALSE,
    Title_found = FALSE, Max_order_found = FALSE, Contexts_found = FALSE;
unsigned int Model_alphabet_size = MODEL_ALPHABET_SIZE;
unsigned int Model_contexts_size = 1;
int Model_max_order = 0;
unsigned int Model_escape_method = TLM_PPM_Method_D;
unsigned int Max_input_size = 0;
boolean Model_performs_full_excls = TRUE;
boolean Model_performs_update_excls = TRUE;

char *Model_title = NULL;
char *Model_parameters;
FILE *Model_fp;

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
	     "  -c n\tnumber of contexts=n (required argument)\n"
             "  -d n\tdebugging level=n\n"
	     "  -o n\tmax order of model=n (required argument)\n"
	     "  -p n\tprogress report every n chars.\n"
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

    Model_found = FALSE;
    Alphabet_found = FALSE;
    Contexts_found = FALSE;
    Title_found = FALSE;
    Max_order_found = FALSE;
    Escape_method_found = FALSE;
    Model_performs_full_excls = 1;
    Model_performs_update_excls = 1;
    Debug_level = 0;
    while ((opt = getopt (argc, argv, "T:UXa:c:d:e:o:p:t:")) != -1)
	switch (opt)
	{
	case 'a':
	    Alphabet_found = 1;
	    Model_alphabet_size = atoi (optarg);
	    break;
	case 'c':
	    Contexts_found = 1;
	    Model_contexts_size = atoi (optarg);
	    break;
	case 'd':
	    Debug_level = atoi (optarg);
	    break;
	case 'e':
	    Escape_method_found = 1;
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Model_escape_method = escape;
	    break;
	case 'o':
	    Max_order_found = 1;
	    Model_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	case 't':
	    Max_input_size = atoi (optarg);
	    break;
	case 'T':
	    Title_found = 1;
	    Model_title = (char *) malloc (strlen (optarg)+1);
	    strcpy (Model_title, optarg);
	    break;
	default:
	    usage ();
	    break;
	}
    if (!Model_found)
      {
        fprintf (stderr, "\nCreating new dynamic model\n\n");
	if (!Alphabet_found)
	    fprintf (stderr, "\nFatal error: missing alphabet size of the model\n\n");

	if (!Contexts_found)
	    fprintf (stderr, "\nFatal error: missing number of contexts for the model\n\n");

	if (!Escape_method_found)
	    fprintf (stderr, "\nFatal error: missing escape method for the model\n\n");

	if (!Title_found)
	    fprintf (stderr, "\nFatal error: missing title of the model\n\n");

	if (!Max_order_found)
	    fprintf (stderr, "\nFatal error: missing max. order of the model\n\n");

	if (!Alphabet_found || !Contexts_found || !Title_found ||
	    !Max_order_found)
	  {
	    usage ();
	    exit (1);
	  }
      }

    for (; optind < argc; optind++)
	usage ();
}

int
getline( FILE *fp, unsigned int *s, int max )
/* Read line from FP into S; return its length (maximum length = MAX). */
{
    int i;
    int cc;

    i = 0;
    cc = '\0';
    while ((--max > 0) && ((cc = getc(fp)) != EOF) && (cc != '\n'))
        s [i++] = cc;
    s [i] = 0;
    if (cc == EOF)
        return (EOF);
    else
        return( i );
}

void
train_context (unsigned int model, unsigned int context, unsigned int *line)
/* Trains the context using the text line. */
{
    unsigned int symbol, p;

    TLM_set_context_operation (TLM_Get_Nothing);

    p = 0;
    while ((symbol = line [p++]))
        TLM_update_context (model, context, symbol);
}

void
train_model (FILE *fp, unsigned int model)
/* Trains the model from the characters in FP. */
{
    unsigned int *contexts, context, line [256], p;

    contexts = (unsigned int *) malloc ((Model_contexts_size + 2) *
					sizeof (unsigned int));
    for (context = 0; context < Model_contexts_size; context++)
        contexts [context] = TLM_create_context (model);

    p = 0;
    for (;;)
    {
        if (getline (stdin, line, 256) == EOF)
	    break;
	train_context (model, contexts [p % Model_contexts_size], line);
	if (Debug_level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
	p++;
    }
    for (context = 0; context < Model_contexts_size; context++)
        TLM_release_context (model, contexts [context]);
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    Model = TLM_create_model
        (TLM_PPM_Model, Model_title, Model_alphabet_size, Model_max_order,
	 Model_escape_method, Model_performs_full_excls, Model_performs_pdate_excls);

    train_model (stdin, Model);
    TLM_write_model (Stdout_File, Model, TLM_Dynamic);

    if (Debug_level > 0)
        TLM_dump_model (Stderr_File, Model, NULL);

    TLM_release_model (Model);

    exit (0);
}
