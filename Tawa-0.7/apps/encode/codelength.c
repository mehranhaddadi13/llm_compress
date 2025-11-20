/* Writes out the codelength for the model. */
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

int Exclude_eolns = 0;    /* For incuding/excluding eolns in the input */
int Delete_last_eoln = 0; /* For deleting the last eoln read in from the input */
int Debug_model = 0;      /* For dumping out the model */
int Debug_chars = 0;      /* For dumping out the codelength character by character */
int Display_entropy = 0;  /* For displaying cross-entropies and not codelengths */

void
usage (void)
{
    fprintf (stderr,
	     "Usage: codelength [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -E\texclude eolns\n"
	     "  -L\tdelete last eoln\n"
	     "  -d n\tdebug model=n\n"
	     "  -c\tprint out codelengths for each character=n\n"
	     "  -e\tcalculate cross-entropy and not codelength\n"
	     "  -m fn\tlist of models filename=fn\n"
	     "  -r\tprint out arithmetic coding ranges\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int models_found;
    int opt;

    /* get the argument options */

    models_found = 0;
    Exclude_eolns = 0;
    Delete_last_eoln = 1;
    while ((opt = getopt (argc, argv, "ELcd:em:r")) != -1)
	switch (opt)
	{
	case 'E':
	    Exclude_eolns = 1;
	    break;
	case 'L':
	    Delete_last_eoln = 1;
	    break;
	case 'c':
	    Debug_chars = 1;
	    break;
	case 'd':
	    Debug_model = atoi (optarg);
	    break;
	case 'e':
	    Display_entropy = 1;
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = 1;
	    break;
	case 'r':
	    Debug.range = 1;
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

unsigned int *
getText (FILE *fp, unsigned int eoln, unsigned int del_last_eoln, unsigned int *len)
/* Read from FP into TEXT; return its length (maximum length = MAX).
   If eoln, then only read up to (not including) the first eoln.
   if del_last_eoln, then delete the last eoln, if it exists. */
{
    int i;
    int cc;
    unsigned int *text = NULL;
    int all=0;

    i = 0;
    while ( ((cc = getc(fp)) != EOF) && (!eoln || (cc != '\n')))
      {
	if (i>=all)
	  {
	    all += 1000;
	    text = (unsigned int *) realloc (text, sizeof (unsigned int) * all + 2);
	  }
	text [i++] = cc;
      }
    if (i>0)
      {
	if (del_last_eoln && (text [i-1] == '\n'))
	  i -= 1; /* Delete the last eoln */
	text [i] = 0;
      }

    (*len)=i;

    return text;
}

float
codelengthText (unsigned int model, unsigned int *text, unsigned int textlen)
/* Returns the codelength (in bits) for encoding the text using the ppm model. */
{
    unsigned int context, p;
    float codelen, codelength;

    context = TLM_create_context (model);
    TLM_set_context_operation (TLM_Get_Codelength);

    /* Insert the sentinel symbol at start of text to ensure first character
       is encoded using a sentinel symbol context rather than an order 0
       context */
    if (Debug.range)
        fprintf (stderr, "Coding ranges for the sentinel symbol (not included in overall total:\n");
    TLM_update_context (model, context, TXT_sentinel_symbol ());
    if (Debug.range)
        fprintf (stderr, "\n");

    codelength = 0.0;
    /* Now encode each symbol */
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
	TLM_update_context (model, context, text [p]);
	codelen = TLM_Codelength;
	if (Debug_chars)
	    fprintf (stderr, "Codelength for character %c = %7.3f\n", text [p], codelen);
	codelength += codelen;
    }
    /* Now encode the sentinel symbol again to signify the end of the text */
    TLM_update_context (model, context, TXT_sentinel_symbol ());
    codelen = TLM_Codelength;
    if (Debug_chars)
        fprintf (stderr, "Codelength for sentinel symbol = %.3f\n", codelen);
    codelength += codelen;

    TLM_release_context (model, context);

    return (codelength);
}

void
codelengthModels (unsigned int *text, unsigned int textlen)
/* Prints out the codelength for encoding the text for the models. */
{
    unsigned int model;
    float codelength, min_codelen = 0.0;
    char *tag, *min_tag;

    min_tag = NULL;
    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
    {
        tag = TLM_get_tag (model);
	codelength = codelengthText (model, text, textlen);
	if (Display_entropy)
	    codelength /= textlen;
	if ((min_codelen == 0.0) || (codelength < min_codelen))
	{
	    min_codelen = codelength;
	    min_tag = tag;
	}
	printf ("%-24s %9.3f\n", tag, codelength);
    }
    if (Display_entropy)
        printf ("\nMinimum cross-entropy for ");
    else
        printf ("\nMinimum codelength for ");
        printf ("%s = %9.3f\n", min_tag, min_codelen);
}

int
main (int argc, char *argv[])
{
    unsigned int *text = NULL;
    unsigned int textlen;

    init_arguments (argc, argv);

    if (Debug_model > 4)
        TLM_dump_models (Stdout_File, NULL);

    if (TLM_numberof_models () < 1){
      usage();
    }

    while ((text = getText (stdin, Exclude_eolns, Delete_last_eoln, &textlen)) != NULL)
      {
	codelengthModels (text, textlen);
	free(text);
      }        

    TLM_release_models ();

    exit (0);
}
