/* Tags text given a trained model. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#define TAG_START 128 /* Start at end of ASCII 7 bit characters */

#include "io.h"
#include "text.h"
#include "paths.h"
#include "model.h"
#include "transform.h"

unsigned int Language_model;
unsigned int Tags;
boolean Segment_Viterbi = FALSE;
unsigned int Segment_stack_depth = 0;

void
debug_play ()
/* Dummy routine for debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -D n\tstack algorithm only: stack depth=n\n"
	     "  -T n\tnumber of tags=n (required argument)\n"
	     "  -V\tsegment using Viterbi algorithm\n"
	     "  -l n\tdebug level=n\n"
	     "  -d n\tdebug paths=n\n"
	     "  -p n\tdebug progress=n\n"
	     "  -m fn\tmodel filename=fn\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    boolean Tags_found = FALSE;
    extern char *optarg;
    extern int optind; 
    int opt;

    /* set defaults */
    Debug.level = 0;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "D:T:Vd:l:m:p:")) != -1)
	switch (opt)
	{
	case 'D':
	    Segment_stack_depth = atoi (optarg);
	    break;
	case 'T':
	    Tags = atoi (optarg);
	    Tags_found = TRUE;
	    break;
	case 'V':
	    Segment_Viterbi = TRUE;
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'm':
	    Language_model =
	        TLM_read_model (optarg, "Loading model from file",
				"Segment: can't open model file");
	    break;
	default:
	    usage ();
	    break;
	}
    if (!Tags_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void
dump_tag_symbol (unsigned int file, unsigned int symbol)
/* Writes the ASCII symbol out in human readable form (excluding white space). */
{
    char line [12];

    assert (TXT_valid_file (file));

    sprintf (line, "%c", symbol);
    TXT_write_file (file, line);
}

boolean
is_not_space (unsigned int symbol)
/* Returns TRUE is symbol is not a space */
{
    return (!TXT_is_space (symbol));
}

int
main (int argc, char *argv[])
{
    unsigned int input;        /* The input text to correct */
    unsigned int transform_text;  /* The marked up text */
    unsigned int transform_model;
    unsigned int tag;
    char replace_text [12];

    init_arguments (argc, argv);

    if (TLM_numberof_models () < 1)
        usage();

    if (Segment_Viterbi)
        transform_model = TTM_create_transform (TTM_Viterbi);
    else
        transform_model = TTM_create_transform (TTM_Stack, TTM_Stack_type0, Segment_stack_depth, 0);

    TTM_add_transform (transform_model, 0.0, "%b", "%b", is_not_space); /* add transform for every character first, except for a space */
    for (tag = 0; tag < Tags; tag++)
      {
	sprintf (replace_text, "%c", tag + TAG_START);
	TTM_add_transform (transform_model, 0.0, " ", replace_text); /* also add each tag after a space */
      }

    /*TTM_dump_transform (stdout, transform_model);*/

    input = TXT_load_text (Stdin_File);

    TTM_start_transform (transform_model, TTM_transform_multi_context, input,
		      Language_model);

    transform_text = TTM_perform_transform (transform_model, input);
    /* Ignore the sentinel and model symbols that are inserted
       at the head of the marked up text. Also ignore trailing symbol. */
    TXT_setlength_text (transform_text, TXT_length_text (transform_text)-1);
    TXT_dump_text1 (Stdout_File, transform_text, 3, dump_tag_symbol);

    TXT_release_text (transform_text);
    TTM_release_transform (transform_model);

    exit (0);
}
