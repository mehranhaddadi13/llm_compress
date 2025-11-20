/* Works out the language of a string of text. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_TAG 64        /* Maximum length of a tag string */
#define MAX_MODELS 128    /* Maximum number of models; used when storing
			     the Tags */

char Tags [MAX_MODELS][MAX_TAG];
unsigned int Tags_count = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_lang [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	);
    exit (2);
}

unsigned int
loadModels (FILE * fp)
/* Load the models from file FP. */
{
    char tag [MAX_TAG], filename [128];
    unsigned int count;

    count = 0;
    while ((fscanf (fp, "%s %s", tag, filename) != EOF))
    {
        count++;
        strcpy (Tags [count], tag);

	TLM_read_model (filename, "Loading training model from file",
			"Ident_lang: can't open training file");
    }

    /*TLM_dump_models (Stderr_File, NULL);*/

    Tags_count = count;
    return (count);
}

char *
get_tag (unsigned int model)
/* Return the tag associated with the model. */
{
    if ((model > 0) && (model <= Tags_count))
        return (Tags [model]);
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
    int opt;

    /* get the argument options */

    models_found = 0;
    while ((opt = getopt (argc, argv, "m:r")) != -1)
	switch (opt)
	{
	case 'm':
	    fprintf (stderr, "Loading models from file %s\n",
		    optarg);
	    if ((fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open models file %s\n",
			 optarg);
		exit (1);
	    }
	    loadModels (fp);
	    models_found = 1;
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found)
    {
        fprintf (stderr, "\nFatal error: missing models\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

int
getline1( FILE *fp, unsigned int *s, int max )
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

float
codelengthText (unsigned int model, unsigned int *text)
/* Returns the model id for encoding the text using the ppm model. */
{
    unsigned int context;
    int p;
    float codelength;

    TLM_set_context_operation (TLM_Get_Codelength);

    codelength = 0.0;
    context = TLM_create_context (model);

    /* Now encode each symbol */
    for (p=0; (text [p]); p++) /* encode each symbol */
    {
	TLM_update_context (model, context, text [p]);
	codelength += TLM_Codelength;
    }
    TLM_release_context (model, context);

    return (codelength);
}

void
identifyModels (unsigned int *text, unsigned int textlen)
/* Returns the model id associated with the model that compresses the
   text best. */
{
    unsigned int model;
    float codelength = 0.0, min_ent = 0.0;
    char *title, *min_title = NULL;

    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
    {
	codelength = codelengthText (model, text);
	title = TLM_get_title (model);
	if ((min_ent == 0.0) || (codelength < min_ent))
	{
	    min_ent = codelength;
	    min_title = title;
	}
	printf ("%-14s %.3f bits (%.3f bits per character)\n",
		title, codelength, (codelength/textlen));
    }
    printf ("\nMinimum codelength for %s\n\n", min_title);
}

int
main (int argc, char *argv[])
{
    unsigned int line [256];
    int len;

    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    printf ("\nEnter text: ");
    while ((len = getline1 (stdin, line, 2000)) != EOF)
    {
        printf ("\nLength of text = %d characters\n\n", len);

	identifyModels (line, len);
	printf ("Enter text: ");
    }

    exit (0);
}
