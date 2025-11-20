/* Works out the language of a string of text. Uses a word and
   non-word based model. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "word.h"
#include "table.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_TAG 64        /* Maximum length of a tag string */
#define MAX_FILENAME 256  /* Maximum length of a filename */
#define MAX_MODELS 10000  /* Maximum number of models; used when storing
			     the Tags */

unsigned int Ident_models [MAX_MODELS];
char Ident_models_tag [MAX_MODELS][MAX_TAG];
float *Ident_models_bits = NULL;

unsigned int Ident_models_count = 0;
FILE *Data_fp;

unsigned int Entropy = FALSE;
unsigned int MinLength = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_word [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -C\tcalculate cross-entropy rather than codelength\n"
	     "  -L n\tminimum length of data file=n\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -d fn\tlist of data files filename=fn (required)\n"
	);
    exit (2);
}

unsigned int
loadModels (FILE * fp)
/* Load the models from file FP. */
{
    char tag [MAX_TAG], filename [MAX_FILENAME];
    unsigned int count, file, words_model;

    count = 0;
    while ((fscanf (fp, "%s %s", tag, filename) != EOF))
    {
        strcpy (Ident_models_tag [count], tag);

	fprintf (stderr, "Loading training models from file %s (tag=%s)\n",
		 filename, tag);

        file = TXT_open_file
	    (filename, "r", "Loading words model from file",
	     "Ident_word: can't open words model file" );
	words_model = TLM_load_words_model (file);

	TXT_close_file (file);

	/*TXT_dump_table (stderr, Tables_nonword [count]);
	  TXT_dump_table (stderr, Tables_word [count]);*/

        count++;
    }
    Ident_models_count = count;
    Ident_models_bits = (float *) calloc (count+1, sizeof (float));

    return (count);
}

char *
get_tag (unsigned int model)
/* Return the tag associated with the model. */
{
    if (model <= Ident_models_count)
        return (Ident_models_tag [model]);
    else
        return (NULL);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    FILE *fp;
    unsigned int models_found;
    unsigned int data_found;
    int opt;

    /* get the argument options */

    models_found = 0;
    data_found = 0;
    while ((opt = getopt (argc, argv, "CL:d:m:")) != -1)
	switch (opt)
	{
	case 'C':
	    Entropy = TRUE;
	    break;
	case 'L':
	    MinLength = atoi (optarg);
	    break;
	case 'm':
	    fprintf (stderr, "Loading models from file %s\n",
		    optarg);
	    if ((fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Ident_word: can't open models file %s\n",
			 optarg);
		exit (1);
	    }
	    loadModels (fp);
	    models_found = 1;
	    break;
	case 'd':
	    fprintf (stderr, "Loading data files from file %s\n",
		     optarg);
	    if ((Data_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Ident_word: can't open test data file %s\n",
			 optarg);
		exit (1);
	    }
	    data_found = 1;
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

void
identifyModels (unsigned int text, unsigned int textlen)
/* Returns the number of bits required to compress each model. */
{
    unsigned int m;

    for (m = 0; m < Ident_models_count; m++)
    {
        Ident_models_bits [m] =
	  TLM_process_word_text (text, Ident_models [m], FIND_CODELENGTH_TYPE,
				 NIL /* no coder */);
	if (Entropy)
	    Ident_models_bits [m] /= textlen;
	m++;
    }
}

void
dump_model_bits()
{
    unsigned int m, bestmodel = 0, nextbestmodel = 0;
    float low=0, nextlow=0;

    for (m = 0; m < Ident_models_count; m++)
    {
        fprintf (stderr, "%6.3f %s\n", Ident_models_bits [m], get_tag (m));
	if ((bestmodel == 0) || (Ident_models_bits [m] < low))
	  {
	    if (bestmodel!=0)
	    {
	        nextlow = low;
		nextbestmodel = bestmodel;
	    }
	    low = Ident_models_bits [m];
	    bestmodel = m;
	  }
	else if ((nextbestmodel==0) || (Ident_models_bits [m] < nextlow))
	  {
	    nextlow = Ident_models_bits [m];
	    nextbestmodel = m;
	  }
	m++;
    }
    if (Entropy)
        fprintf (stderr,"The lowest cross-entropy of ");
    else
        fprintf (stderr,"The lowest codelength of ");
    fprintf (stderr,"%9.3f is for %s.\n",
	     low, get_tag(bestmodel));
    if (TLM_valid_model (nextbestmodel))
      {
	if (Entropy)
	    fprintf (stderr,"The next lowest cross-entropy of ");
	else
	    fprintf (stderr,"The next lowest codelength of ");
	fprintf (stderr,"%9.3f is for %s.\n",
		 nextlow, get_tag (nextbestmodel));
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
      /*dump_memory (stderr);*/

        file = TXT_open_file (filename, "r", NULL,
			      "Ident_word: can't open data file");
	text = TXT_load_text (file);
	textlen = TXT_length_text (text);
	if (!MinLength || (textlen >= MinLength))
	  {
	    fprintf (stderr, "Loading data file from file %s\n",
		     filename);
	    identifyModels (text, textlen);
	    dump_model_bits();
	  }
	TXT_release_text (text);
	TXT_close_file (file);
    }
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    identifyFiles (Data_fp);

    free (Ident_models_bits);

    return 0;
}
