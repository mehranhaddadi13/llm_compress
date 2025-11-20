/* Loads a static model, then writes it out again. A pretty useless thing to
   do, but it needs testing anyway (unless you want to change a model from
   one version to another. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"

unsigned int Model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train1 [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -d n\tdebug level=n\n"
	     "  -m fn\tmodel filename=fn\n"
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
    while ((opt = getopt (argc, argv, "d:m:r")) != -1)
	switch (opt)
	{
	case 'd':
	    Debug.level = atoi (optarg);
	    break;
	case 'm':
	    Model =
	        TLM_read_model (optarg, "Loading model from file",
				"Train: can't open model file");
	        /* NULL above means don't change the model's title */
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
    init_arguments (argc, argv);

    TLM_write_model (Stdout_File, Model, TLM_Static);

    TLM_release_model (Model);

    exit (0);
}
