/* Works out the language of a string of text.
   Uses two models - one to predict each character given the previous characters
   and one to predict each class given the previous characters. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
FILE *Test_fp;

unsigned int Load_Numbers = 0;
unsigned int Debug_level = 0;

unsigned int Model_Count = 0;
unsigned int *Model_Ids=NULL;
unsigned int *Model_Contexts=NULL;
double *Model_Probabilities=NULL;
double *Model_Codelengths=NULL;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -N\tinput text is a sequence of unsigned numbers\n"
	     "  -d\tdebug level=n\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -t fn\tlist of test data files filename=fn (required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean models_found;
    boolean data_found;
    int opt;

    /* get the argument options */

    models_found = FALSE;
    data_found = FALSE;
    while ((opt = getopt (argc, argv, "Nd:m:t:")) != -1)
	switch (opt)
	{
	case 'N':
	    Load_Numbers = TRUE;
	    break;
	case 'd':
	    Debug_level = atol(optarg);
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = TRUE;
	    break;
	case 't':
	    fprintf (stderr, "Loading data files from file %s\n",
		     optarg);
	    if ((Test_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Ident_file4: can't open test file %s\n",
			 optarg);
		exit (1);
	    }
	    data_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found)
    {
        fprintf (stderr, "\nFatal error: missing models filename\n\n");
        usage ();
    }
    if (!data_found)
    {
        fprintf (stderr, "\nFatal error: missing data filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

double
codelength (double prob)
{
  return (- log2 (prob));
}

void
codelengthsText (unsigned int text)
/* Calculates the codelengths for encoding the text using the PPM models. */
{
    unsigned int pos, symbol;
    int model, m;

    TLM_set_context_operation (TLM_Get_Codelength);

    /* Allocate the contexts for each model that has been loaded */
    m = 0;
    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
    {
      Model_Ids [m] = model;
      Model_Contexts [m] = TLM_create_context (model);
      Model_Codelengths [m] = 0;
      m++;
    }

    /* Now encode each symbol in the text using the models */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
      if (Debug_level > 2)
	  fprintf (stderr, "Pos %5d Symbol %3d:", pos, symbol);
      for (m = 0; m < Model_Count; m++)
	{
	  model = Model_Ids [m];
	  TLM_update_context (model, Model_Contexts [m], symbol);
	  if (Debug_level > 2)
	      fprintf (stderr, " %-7s %5.2f", TLM_get_tag (model), TLM_Codelength);
	  Model_Probabilities [m] = pow (2, - TLM_Codelength);
	  Model_Codelengths [m] += TLM_Codelength;
	}
      if (Debug_level > 2)
	  fprintf (stderr, "\n");
    }

    /* Dump out total codelengths for each model.
       Also release the contexts for each model that has been loaded */
    for (m = 0; m < Model_Count; m++)
      {
	if (Debug_level > 1)
	    fprintf (stderr, "Total %d Model %-7s codelength %7.3f\n",
		     m, TLM_get_tag (model), Model_Codelengths [m]);

	TLM_release_context (model, Model_Contexts [m]);
      }
}

void
identifyFiles (FILE * fp)
/* Load the test data files from file FP. */
{
    char filename [128];
    char file_tag [128];
    unsigned int file, text, textlen;

    while ((fscanf (fp, "%s %s", file_tag, filename) != EOF))
    {
      file = TXT_open_file (filename, "r", NULL,
			    "Ident_file: can't open data file");
      fprintf (stderr, "\nProcessing test file %s:\n", filename);
      if (Load_Numbers)
	  text = TXT_load_numbers (file);
      else
	  text = TXT_load_text (file);

      textlen = TXT_length_text (text);
      codelengthsText (text);

      TXT_release_text (text);
      TXT_close_file (file);
    }
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    Model_Count = TLM_numberof_models();
    Model_Ids = (unsigned int *) calloc (Model_Count+1, sizeof(unsigned int));
    Model_Contexts = (unsigned int *) calloc (Model_Count+1, sizeof(unsigned int));
    Model_Probabilities = (double *) calloc (Model_Count+1, sizeof(double));
    Model_Codelengths = (double *) calloc (Model_Count+1, sizeof(double));

    identifyFiles (Test_fp);

    free (Model_Ids);
    free (Model_Contexts);
    free (Model_Probabilities);
    free (Model_Codelengths);

    return 0;
}
