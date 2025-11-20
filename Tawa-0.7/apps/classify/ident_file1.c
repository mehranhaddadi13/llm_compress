/* Works out the language of a string of text.
   Writes out the results one per line as: filename, model-tag, codelength. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
FILE *Test_fp;

boolean Entropy = FALSE;
boolean BothEntropy = FALSE;
boolean DynamicModelRequired = FALSE;
unsigned int DynamicAlphabetSize = 256;
unsigned int DynamicMaxOrder = 5;
unsigned int DynamicEscapeMethod = TLM_PPM_Method_D;
unsigned int MinLength = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -B\tcalculate both the cross-entropy and the codelength\n"
	     "  -C\tcalculate cross-entropy rather than codelength\n"
	     "  -D\tinclude dynamic model in list of models\n"
	     "  -L n\tminimum length of data file=n\n"
	     "  -a n\tsize of alphabet for dynamic model=n\n"
	     "  -e n\tescape method for model=c\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -o n\tmax order of dynamic model=n\n"
	     "  -r\tdebug arithmetic coding ranges\n"
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
    int opt, escape;

    /* get the argument options */

    models_found = FALSE;
    data_found = FALSE;
    while ((opt = getopt (argc, argv, "BCDL:a:e:m:o:rt:")) != -1)
	switch (opt)
	{
	case 'B':
	    BothEntropy = TRUE;
	    break;
	case 'C':
	    Entropy = TRUE;
	    break;
	case 'D':
	    DynamicModelRequired = TRUE;
	    break;
	case 'L':
	    MinLength = atoi (optarg);
	    break;
        case 'a':
	    DynamicAlphabetSize = atoi (optarg);
	    break;
	case 'e' :
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    DynamicEscapeMethod = escape;
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = TRUE;
	    break;
	case 'o':
	    DynamicMaxOrder = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	case 't':
	    fprintf (stderr, "Loading data files from file %s\n",
		     optarg);
	    if ((Test_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open models file %s\n",
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

float
codelengthText (unsigned int model, unsigned int text)
/* Returns the codelength for encoding the text using the PPM model. */
{
    unsigned int context, pos, symbol;
    float codelength;

    TLM_set_context_operation (TLM_Get_Codelength);

    codelength = 0.0;
    context = TLM_create_context (model);

    /* Now encode each symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	if (Debug.range != 0)
	  {
	    if (symbol == TXT_sentinel_symbol ())
	      fprintf (stderr, "Encoding sentinel symbol\n");
	    else
	      fprintf (stderr, "Encoding symbol %d (%c)\n", symbol, symbol);
	  }

	TLM_update_context (model, context, symbol);
	codelength += TLM_Codelength;
    }

    TLM_release_context (model, context);

    return (codelength);
}

void
identifyModels (char *filename, unsigned int text, unsigned int textlen)
/* Reports the number of bits required to compress each model. */
{
    unsigned int DynamicModel = NIL;
    float codelength, avg_codelength, min_avg_codelength;
    unsigned int model, min_model, m;

    if (DynamicModelRequired)
        DynamicModel = TLM_create_model
	   (TLM_PPM_Model, "Dynamic", DynamicAlphabetSize, DynamicMaxOrder,
	    DynamicEscapeMethod, TRUE);

    TLM_reset_modelno ();
    m = 0;
    min_model = 0;
    min_avg_codelength = 999.9;
    while ((model = TLM_next_modelno ()))
    {
        codelength = codelengthText (model, text);
	avg_codelength = codelength/textlen;

	if (!min_model || (avg_codelength < min_avg_codelength))
	  {
	    min_avg_codelength = avg_codelength;
	    min_model = model;
	  }
	if (BothEntropy)
	  {
	    fprintf (stderr, "%9.3f %6.3f %s %s\n", codelength,
		     avg_codelength, filename, TLM_get_tag (model));
	  }
        else
	  {
	    if (Entropy)
	        codelength = avg_codelength;
	    fprintf (stderr, "%6.3f %s %s\n", codelength, filename,
		     TLM_get_tag (model));
	  }

	m++;
    }
    fprintf (stderr, "Minimum codelength %6.3f for model %s\n", min_avg_codelength,
	     TLM_get_tag (min_model));

    if (DynamicModelRequired)
        TLM_release_model (DynamicModel);
}

void
identifyFiles (FILE * fp)
/* Load the test data files from file FP. */
{
    char filename [128];
    unsigned int file, text, textlen;

    while ((fscanf (fp, "%s", filename) != EOF))
    {
      /*dump_memory (stderr);*/

        file = TXT_open_file (filename, "r", NULL,
			      "Ident_file: can't open data file");
	text = TXT_load_text (file);
	textlen = TXT_length_text (text);
        if (!MinLength || (textlen >= MinLength))
	  {
	    /*fprintf (stderr, "Loading data file from file %s\n",
	               filename);
	    */
	    identifyModels (filename, text, textlen);
	  }
	TXT_release_text (text);
	TXT_close_file (file);
    }
}

int
main (int argc, char *argv[])
{
    int num_models;

    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    num_models = TLM_numberof_models();

    identifyFiles (Test_fp);

    return 0;
}
