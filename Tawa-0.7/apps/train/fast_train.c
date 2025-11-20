/* fastPPM testing program; */
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "io.h"
#include "text.h"
#include "model.h"

unsigned int debugLevel = 0;
unsigned int debugProgress = 0;
unsigned int debugMemory = 0;
boolean Model_is_PPMq = TRUE;

void junk ()
/* For debugging purposes. */
{
  fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options]\n"
	     "\n"
	     "options:\n"
	     "  -C\tChange type of model to PPM (default is PPMq)\n"
	     "  -T str\tTitle of model=str (required argument)\n"
	     "  -a n\tsize of alphabet=n (required argument)\n"
	     "  -e n\tescape method=c (required argument)\n"
	     "  -o n\torder of model=n (required argument)\n"
	     "  -d n\tdebug level=n\n"
	     "  -m n\tdebug memory usage=n\n"
	     "  -p n\tdebug progress=n\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[], char **title,
		unsigned int *alphabet_size, unsigned int *order,
		unsigned int *escape_method)
{
    extern char *optarg;
    extern int optind; 
    int opt;
    int escape;
    boolean order_found;
    boolean escape_method_found;
    boolean title_found;
    boolean alphabet_size_found;

    /* get the argument options */

    *order = 0;
    alphabet_size_found = FALSE;
    order_found = FALSE;
    escape_method_found = FALSE;
    title_found = FALSE;
    while ((opt = getopt (argc, argv, "CT:a:d:e:o:p:m:")) != -1)
	switch (opt)
	{
	case 'C':
	    Model_is_PPMq = FALSE;
	    break;
	case 'T':
	    title_found = (strlen (optarg) > 0);
	    if (title_found)
	      {
		*title = (char *) malloc (strlen (optarg)+1);
		strcpy (*title, optarg);
	      }
	    break;
	case 'a':
	    alphabet_size_found = TRUE;
	    *alphabet_size = atol (optarg);
	    break;
	case 'd':
	    debugLevel = atol (optarg);
	    break;
	case 'e':
	    escape_method_found = TRUE;
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    *escape_method = escape;
	    break;
	case 'o':
	    order_found = TRUE;
	    *order = atol (optarg);
	    break;
	case 'p':
	    debugProgress = atol (optarg);
	    break;
	case 'm':
	    debugMemory = atol (optarg);
	    break;
	default:
	    usage ();
	    break;
	}
    if (!alphabet_size_found)
      {
	fprintf (stderr, "Train: missing argument (alphabet size)\n");
	usage ();
      }
    if (!escape_method_found)
      {
	fprintf (stderr, "Train: missing argument (escape method)\n");
	usage ();
      }
    if (!order_found)
      {
	fprintf (stderr, "Train: missing argument (order of model)\n");
	usage ();
      }
    if (!title_found)
      {
	fprintf (stderr, "Train: missing argument (title of model)\n");
	usage ();
      }
    for (; optind < argc; optind++)
	usage ();
}

void trainModel (unsigned int model)
/* Trains the model using the text from stdin. */
{
    unsigned int context, sym, count;
    int cc;

    context = TLM_create_context (model);

    /* dump_memory (Stdout_File); */

    count = 0;
    TLM_set_context_operation (TLM_Get_Nothing);
    while ((cc = getc(stdin)) != EOF)
      {
	sym = cc;
	if (debugLevel > 4)
	    fprintf (stderr, ">>> Input pos %d char %c (%d)\n", count, sym, cc);

	if (debugProgress && (count % debugProgress) == 0)
	  {
	    fprintf (stderr, "%d chars processed\n", count);

	    /*
	    finishClock(1);
	    dumpClocks (stdout);
	    startClock(1);
	    */
	  }
	if (debugMemory && (count % debugMemory) == 0)
	    fprintf (stderr, "%d chars processed, %d bytes memory used\n",
		     count, TLM_sizeof_model (model));

	/*startClock (2);*/
	TLM_update_context (model, context, sym);
	/*finishClock (2);*/

	if (debugLevel > 4)
	  {
	    TLM_check_model (Stderr_File, model, NULL);
	    TLM_dump_model (Stderr_File, model, NULL);
	  }

	count++;
      }

    TLM_update_context (model, context, TXT_sentinel_symbol ());
    TLM_release_context (model, context);
}

int
main(int argc, char *argv [])
{
    unsigned int alphabet_size, order, escape_method;
    unsigned int model;
    char *title = NULL;

    init_arguments (argc, argv, &title, &alphabet_size, &order, &escape_method);

    initClocks ();

    TXT_init_files ();

    model = TLM_create_model (TLM_PPMq_Model, title, alphabet_size, order, escape_method);

    startClock (1);
    trainModel (model);
    finishClock (1);

    dumpClocks (stdout);

    TLM_check_model (Stderr_File, model, NULL);

    if (debugLevel > 3)
      {
        TLM_dump_model (Stderr_File, model, NULL);
      }
    else
      {
	if (!Model_is_PPMq)
	    TLM_set_write_operation (TLM_Write_Change_Model_Type, TLM_PPM_Model);
	TLM_write_model (Stdout_File, model, TLM_Static);
      }
    exit (0);
}
