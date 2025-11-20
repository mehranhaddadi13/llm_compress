/* Produce statsistics for the marked up files. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "coder.h"
#include "model.h"
#include "transform.h"

#define MODELS_SIZE 256   /* Maximum number of models */
#define TAG_SIZE 256      /* Maximum size of a tag */
#define FILENAME_SIZE 256 /* Maximum size of a filename */

#define DYNAMIC_TITLE "Dynamic model"
#define PERFORMS_EXCLS TRUE
#define MAX_ORDER 5       /* Maximum order of the model */

#define ALPHABET_SIZE 256 /* Default max. alphabet size for all PPM models */

unsigned int Dynamic = FALSE;
unsigned int Dynamics = 0;

unsigned int Dynamic_alphabet_size = ALPHABET_SIZE;
unsigned int Dynamic_max_order = MAX_ORDER;

unsigned int Models_count = 0;
unsigned int Model_switches = 0;
char *Model_tags [MODELS_SIZE];
unsigned int *Model_counts = NULL;

void
debugEncode ()
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -D\tInclude single dynamic model in transform\n"
	     "  -M n\tInclude multiple dynamic models in transform up to order n\n"
	     "  -m fn\tmodel filename=fn\n"
	     "(One of -D, -M or -m must be present)\n"
	);
    exit (2);
}

void
set_model_tag (char *tag)
/* Sets the tag for the next model. */
{
        Models_count++;
        Model_tags [Models_count] = (char *) malloc (strlen (tag) + 1);
	strcpy (Model_tags [Models_count], tag);
}

void
load_models (char *filename)
/* Load the model tags from the specified file. */
{
    char tag [TAG_SIZE], model_filename [FILENAME_SIZE];
    FILE *fp;

    fprintf (stderr, "Loading models from file %s\n", filename);
    if ((fp = fopen (filename, "r")) == NULL)
      {
	fprintf (stderr, "TMT: can't open models file %s\n", filename);
	exit (1);
      }

    while ((fscanf (fp, "%s %s", tag, model_filename) != EOF))
    {
        set_model_tag (tag);
    }
    fclose (fp);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    unsigned int models_found = FALSE;
    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "DM:m:")) != -1)
	switch (opt)
	{
	case 'D':
	    Dynamic = TRUE;
	    break;
	case 'M':
	    Dynamics = atoi (optarg);
	    break;
	case 'm':
	    load_models (optarg);
	    models_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found && !Dynamics && !Dynamic)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

unsigned int
analyze_transform_text (unsigned int text)
/* Analyzes the marked up text. */
{
    unsigned int model, new_model, pos, p;
    unsigned int textlen, symbol, prevsymbol;
    unsigned int sentinel_symbol;

    assert (TXT_valid_text (text));
    sentinel_symbol = TXT_sentinel_symbol ();

    prevsymbol = sentinel_symbol;
    model = NIL;
    textlen = TXT_length_text (text);
    if (textlen) /* ignore sentinel symbol at end of file */
        TXT_setlength_text (text, textlen-1);

    pos = 0;
    for (p=0; p < textlen-1; p++)
    {
        TXT_get_symbol (text, p, &symbol);
	if (symbol == sentinel_symbol)
	  {
	    if (!TXT_get_symbol (text, ++p, &symbol))
	        break;
	    assert (symbol == TRANSFORM_MODEL_TYPE);

	    if (!TXT_get_symbol (text, ++p, &new_model))
	        break;

	    model = new_model;
	    Model_switches++;
	  }
	else
	  {
	    assert (model != NIL);

	    Model_counts [model]++;
	    pos++;
	  }
	prevsymbol = symbol;
    }

    return (pos);
}

int
main (int argc, char *argv[])
{
    unsigned int transform_text, textlen, model;
    int max_order;
    char title [TAG_SIZE];

    init_arguments (argc, argv);

    if (Dynamics)
      {
	for (max_order=-1; max_order <= (int) Dynamics; max_order++)
	  {
	    sprintf (title, "Dynamic, %2d", max_order);
	    set_model_tag (title);
	  }
      }
    else if (Dynamic)
      {
	set_model_tag ("Dynamic");
      }

    Model_counts = (unsigned int *)
        malloc ((Models_count+1) * sizeof (unsigned int));
    for (model = 1; model <= Models_count; model++)
      {
        Model_counts [model] = 0;
      }

    transform_text = TXT_load_file (Stdin_File);

    textlen = analyze_transform_text (transform_text);

    for (model = 1; model <= Models_count; model++)
      {
	fprintf (stderr, "Model %d (%s) : occurs %.1f per cent\n",
		 model, Model_tags [model],
		 (float) 100.0 * Model_counts [model]/textlen);
      }
    fprintf (stderr, "Number of switches = %d percentage = %.1f\n",
	     Model_switches, (float) 100.0 * Model_switches/textlen);

    exit (0);
}
