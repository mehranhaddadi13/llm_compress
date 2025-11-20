/* Writes out the codelength for the model for each line of the input text file. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#if defined (SYSTEM_LINUX)
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_ORDER 5       /* Maximum order of the model */

#define MAX_ALPHABET 256  /* Default max. alphabet size for all PPM models */

#define MAX_TAG 256       /* Maximum length of a tag string */
#define MAX_FILENAME 256  /* Maximum length of a tag string */

int Entropy = 0;          /* For calculating cross-entropies and not codelengths */
int Maximum = 0;          /* For listing the maximum codelength per line */
float Maximum_Codelength; /* Maximum codelength for each line */
unsigned int Model = 0;   /* The model being loaded */

void
usage (void)
{
    fprintf (stderr,
	     "Usage: codelength [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -d n\tdebug model=n\n"
	     "  -e\tcalculate cross-entropy and not codelength\n"
	     "  -M\tlist maximum codelength\n"
	     "  -m fn\tmodel filename=fn\n"
	);
    exit (2);
}

int
getline1( FILE *fp, char *s, int max )
/* Read line from FP into S; return its length (maximum length = MAX). */
{
    int i;
    int cc;

    i = 0;
    cc = '\0';
    while ((--max > 0) && ((cc = getc(fp)) != EOF) && (cc != '\n'))
      s [i++] = (unsigned char) cc;
    s [i] = 0;
    if (cc == EOF)
        return (EOF);
    else
        return( i );
}

float
codelengthText (unsigned int model, unsigned int text)
/* Returns the codelength for encoding the text using the PPM model. */
{
    unsigned int context, len, pos, symbol;
    float codelength, total_codelength;

    len = TXT_length_text (text);
    if (len == 0)
        return (0.0);

    Maximum_Codelength = 0.0;

    TLM_set_context_operation (TLM_Get_Codelength);

    total_codelength = 0.0;
    context = TLM_create_context (model);

    /* Now encode each symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	TLM_update_context (model, context, symbol);
	codelength = TLM_Codelength;
	total_codelength += codelength;
	if (codelength > Maximum_Codelength)
	    Maximum_Codelength = codelength;
    }
    TLM_release_context (model, context);

    if (Entropy)
        return (total_codelength / len);
    else
        return (total_codelength);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int model_found;
    int opt;

    /* get the argument options */

    model_found = 0;
    while ((opt = getopt (argc, argv, "Mem:")) != -1)
	switch (opt)
	{
	case 'M':
	    Maximum = 1;
	    break;
	case 'e':
	    Entropy = 1;
	    break;
	case 'm':
	    Model = TLM_read_model
	      (optarg, NULL, 
	       "Codelength1: can't open model file");
	    model_found = 1;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!model_found)
    {
        fprintf (stderr, "\nFatal error: missing model\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int line;
    float codelength;

    init_arguments (argc, argv);

    if (TLM_numberof_models () < 1){
      usage();
    }

    line = TXT_create_text ();

    while (TXT_readline_text (Stdin_File, line) > 0)
      {
	codelength = codelengthText (Model, line);

	printf ("%9.3f ", codelength);
	if (Maximum)
	    printf ("%9.3f ", Maximum_Codelength);

	TXT_dump_text (Stdout_File, line, TXT_dump_symbol);
	printf ("\n");
      }

    TLM_release_models ();

    exit (0);
}
