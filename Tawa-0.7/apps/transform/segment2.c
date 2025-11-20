/* Segments text (i.e. inserts spaces) given a trained model. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "model.h"
#include "transform.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

unsigned int Input_file;
char Input_filename [MAX_FILENAME_SIZE];

unsigned int Output_file;
char Output_filename [MAX_FILENAME_SIZE];

boolean Use_Numbers = FALSE;
unsigned int Language_model;
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
	     "  -C \tdebug coding ranges\n"
	     "  -d n\tdebug paths=n\n"
	     "  -D n\tstack algorithm only: stack depth=n\n"
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -l n\tdebug level=n\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -N\ttext stream is a sequence of unsigned numbers\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -p n\tdebug progress=n\n"
	     "  -V\tsegment using Viterbi algorithm\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    int opt;

    boolean Input_found = FALSE, Output_found = FALSE;

    /* set defaults */
    Debug.level = 0;
    Debug.level1 = 0;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "Cd:D:i:l:m:No:p:V")) != -1)
	switch (opt)
	{
	case 'C':
	    Debug.coder = TRUE;
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'D':
	    Segment_stack_depth = atoi (optarg);
	    break;
	case 'i':
	    Input_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'm':
	    Language_model =
	        TLM_read_model (optarg, "Loading model from file",
				"Segment: can't open model file");
	    /*TLM_dump_model (Stderr_File, Language_model, NULL);*/
	    break;
	case 'N':
	    Use_Numbers = TRUE;
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'V':
	    Segment_Viterbi = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

int
getSymbol (unsigned int file, unsigned int *symbol)
/* Returns the next symbol from the input file. */
{
    unsigned int sym;
    int result;

    sym = 0;
    if (Use_Numbers)
      {
        result = fscanf (Files [file], "%u", &sym);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	  }
      }
    else
      {
        sym = getc (Files [file]);
	result = sym;
      }
    *symbol = sym;
    return (result);
}

void
dump_transform_symbol (unsigned int file, unsigned int symbol)
/* Writes the ASCII symbol out in human readable form (excluding white space). */
{
    char line [20];

    assert (TXT_valid_file (file));

    if (Use_Numbers)
        sprintf (line, "%d\n", symbol);
    else
        sprintf (line, "%c", symbol);
    TXT_write_file (file, line);
}

int
main (int argc, char *argv[])
{
    unsigned int input;        /* The input text to correct */
    unsigned int transform_text;  /* The marked up text */
    unsigned int transform_model;

    init_arguments (argc, argv);

    Input_file = TXT_open_file
      (Input_filename, "r", "Encoding input file",
       "Encode_ppmo: can't open input file" );

    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Encode_ppmo: can't open output file" );

    if (TLM_numberof_models () < 1)
        usage();

    if (Segment_Viterbi)
        transform_model = TTM_create_transform (TTM_Viterbi);
    else
        transform_model = TTM_create_transform (TTM_Stack, TTM_Stack_type0, Segment_stack_depth, 0);

    TTM_add_transform (transform_model, 0.0, "%w", "%w"); /* add transform for every character first */
    TTM_add_transform (transform_model, 0.0, "ox", "o"); /* delete an x (for the Playfair cipher) */
    TTM_add_transform (transform_model, 0.0, "xe", "e"); /* delete an x (for the Playfair cipher) */

    if (Debug.level1 > 2)
        TTM_debug_transform (dumpPathSymbols, NULL, NULL);

    if (Debug.level1 > 4)
        TTM_dump_transform (Stderr_File, transform_model);

    if (Use_Numbers)
        input = TXT_load_numbers (Input_file);
    else
        input = TXT_load_text (Input_file);

    TTM_start_transform (transform_model, TTM_transform_multi_context, input,
		      Language_model);

    transform_text = TTM_perform_transform (transform_model, input);
    /* Ignore the sentinel and model symbols that are inserted
       at the head of the marked up text. */
    TXT_dump_text1 (Output_file, transform_text, 2, dump_transform_symbol);

    TXT_release_text (transform_text);
    TTM_release_transform (transform_model);

    exit (0);
}
