/* Prints out the codelengths for some input files given a model. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

char *Args_model_filename;        /* The filename of an existing model */

FILE *Test_fp;

boolean Entropy = FALSE;
unsigned int Load_Numbers = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -C\tcalculate cross-entropy rather than codelength\n"
	     "  -N\tinput texts are sequences of unsigned numbers\n"
	     "  -m fn\tmodel filename=fn (required)\n"
	     "  -t fn\tlist of test data files filename=fn (required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean model_found;
    boolean data_found;
    int opt;

    /* get the argument options */

    model_found = FALSE;
    data_found = FALSE;
    while ((opt = getopt (argc, argv, "CNm:t:")) != -1)
	switch (opt)
	{
	case 'C':
	    Entropy = TRUE;
	    break;
	case 'N':
	    Load_Numbers = TRUE;
	    break;
	case 'm':
	    Args_model_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_model_filename, optarg);
	    model_found = TRUE;
	    break;
	case 't':
	    fprintf (stderr, "Loading test data files from file %s\n",
		     optarg);
	    if ((Test_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Codelengths: can't open test file %s\n",
			 optarg);
		exit (1);
	    }
	    data_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!model_found)
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
	TLM_update_context (model, context, symbol);
	codelength += TLM_Codelength;
    }
    TLM_release_context (model, context);

    return (codelength);
}

void
codelengthsFiles (FILE *fp, unsigned int model)
/* Calculates the compression codelengths for the test data files from file FP. */
{
    char filename [128];
    float codelength;
    unsigned int file, text, textlen;

    while ((fscanf (fp, "%s", filename) != EOF))
    {
        file = TXT_open_file (filename, "r", NULL,
			      "Codelengths: can't open test data file");
	if (Load_Numbers)
	    text = TXT_load_numbers (file);
	else
	    text = TXT_load_text (file);

	textlen = TXT_length_text (text);
	codelength = codelengthText (model, text);

	if (!Entropy)
	    fprintf (stderr, "%14.4f %s\n", codelength, filename);
	else
	    fprintf (stderr, "%14.4f %s\n", codelength / textlen, filename);

	TXT_release_text (text);
	TXT_close_file (file);
    }
}

int
main (int argc, char *argv[])
{
    unsigned int model;

    init_arguments (argc, argv);

    /* load existing model */
    model = TLM_read_model (Args_model_filename, NULL, NULL);

    codelengthsFiles (Test_fp, model);

    return 0;
}
