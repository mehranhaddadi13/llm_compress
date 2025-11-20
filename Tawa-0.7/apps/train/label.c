/* Re-generates the input files as a sequence of numbers by allocating
   a disjoint sub-alphabet of 256 symbols in the extended alphabet for
   each input file.
   This sequence can then be piped into train for training a single PPM
   model based on the extended "labelled" alphabet that encompasses all
   the input files. */
#include <stdio.h>
#include <stdlib.h>

#include "text.h"

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define ASCII_SIZE 256 /* Default ASCII alphabet size */

FILE *InputFiles_fp = NULL;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int inputfiles_found;
    int opt;

    /* get the argument options */

    inputfiles_found = 0;
    while ((opt = getopt (argc, argv, "m:")) != -1)
	switch (opt)
	{
	case 'm':
	    fprintf (stderr, "Loading list of input files from file %s\n",
		    optarg);
	    if ((InputFiles_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open input files list file %s\n",
			 optarg);
		exit (1);
	    }
	    inputfiles_found = 1;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!inputfiles_found)
    {
        fprintf (stderr, "\nFatal error: missing input files list filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

void
labelText (unsigned int label, unsigned int text, unsigned int textlen)
/* Regenerates the input text as a sequence of labelled numbers. */
{
    unsigned int symbol, p;

    for (p=0; p<textlen; p++)
      {
	TXT_get_symbol (text, p, &symbol);
	symbol += (label*ASCII_SIZE);
	printf ("%d\n", symbol);
      }
}

void
labelInputFiles (FILE * fp)
/* Load and generate a labelled sequence for the input files specified
   by file FP. */
{
    char filename [128];
    unsigned int label, text, textlen, file;

    label = 0;
    while ((fscanf (fp, "%s", filename) != EOF))
    {
        file = TXT_open_file (filename, "r", "Loading data file from file",
			      "Label: can't open input file");
	text = TXT_load_text (file);
	TXT_close_file (file);

	textlen = TXT_length_text (text);
	labelText (label, text, textlen);
	TXT_release_text (text);
	label++;
    }
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    labelInputFiles (InputFiles_fp);

    exit (0);
}
