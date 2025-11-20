/* Writes out the probabilities for each symbol. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

unsigned int Model, Context, Context1;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: probs [options] training-model\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -r\tdebug coding ranges\n"
	);
    exit (2);
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
    while ((opt = getopt (argc, argv, "m:r")) != -1)
	switch (opt)
	{
	case 'm':
	    Model = TLM_read_model (optarg, "Loading model from file",
				    "Encode: can't open model file");
	    model_found = 1;
	    break;
	case 'r':
	    Debug.range = TRUE;
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
getText (FILE *fp, unsigned int *text, int max)
/* Read from FP into TEXT; return its length (maximum length = MAX). */
{
    int i;
    int cc;

    i = 0;
    cc = '\0';
    while ((--max > 0) && ((cc = getc(fp)) != EOF) && (cc != '\n'))
        text [i++] = cc;
    text [i] = TXT_sentinel_symbol ();
    text [i+1] = '\0';
    if (cc == EOF)
      return (EOF);
    else
      return (i+1);
}

void
probsText (unsigned int model, unsigned int *text, unsigned int textlen)
/* Returns the probabilities for encoding the text using the ppm model. */
{
    unsigned int p, sym;
    /* unsigned int coderanges; */

    TLM_set_context_operation (TLM_Get_Codelength);

    /* Now encode each symbol */
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
        fprintf (stderr, "Position = %d ", p);

	while (TLM_next_symbol (Model, Context, &sym))
	{
	    fprintf (stderr, "Sym = %3d codelength = %8.3f\n", sym,
		     TLM_Codelength);
	}
	TLM_reset_symbol (Model, Context);

	/* This will do exactly the same thing if you feel like
	   you want to experience a bit of deja vu:

        while (TLM_next_symbol (Model, Context, &sym))
	{
	    fprintf (stderr, "Sym = %d codelength = %8.3f\n", sym,
	             TLM_Codelength);
	}
	TLM_reset_symbol (Model, Context);
	*/

	/* Now pick one of the symbols (let's say it was the one in the input text)
	   then move on to the next one by updating the model */
	sym = text [p];

	/* TLM_set_context_operation (TLM_Get_Coderanges); */
	TLM_update_context (model, Context, sym);

	/* The following has a bug in it: (Coderanges needs to be reset)

	coderanges = TLM_Coderanges;
	fprintf (stderr, "Codelength = %.3f ",
		 TLM_codelength_coderanges (coderanges));
	TLM_dump_coderanges (Stderr_File, coderanges);
	TLM_release_coderanges (coderanges);
	*/
    }
}

int
main (int argc, char *argv[])
{
    unsigned int text [1024];
    int textlen;

    init_arguments (argc, argv);

    Context = TLM_create_context (Model);
    Context1 = TLM_create_context (Model);

    printf ("\nEnter text:\n");
    while ((textlen = getText (stdin, text, 1024)) != EOF)
    {
	probsText (Model, text, textlen);
	printf ("\nEnter text:\n");
    }        

    TLM_release_context (Model, Context);
    TLM_release_context (Model, Context1);
    TLM_release_model (Model);

    exit (0);
}
