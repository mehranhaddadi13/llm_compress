/* Loads or creates a word-based model using cumulative probability
   tables (pt), adds some text to the model, then writes out the
   changed model. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "word.h"
#include "text.h"
#include "table.h"
#include "model.h"
#include "pt_model.h"

#define CHAR_ALPHABET_SIZE 256   /* Size of the character model's alphabet */
#define MAX_FILENAME_SIZE 128    /* Maximum size of a filename */
#define DEFAULT_WORD_MAX_ORDER 1 /* Default max order for word model */
#define DEFAULT_CHAR_MAX_ORDER 4 /* Default max order for character model */
#define PERFORMS_EXCLS TRUE

char *Model_title;

unsigned int Word_model;
unsigned int Word_model_max_order = DEFAULT_WORD_MAX_ORDER;
unsigned int Char_model;
unsigned int Char_model_max_order = DEFAULT_CHAR_MAX_ORDER;

unsigned int Nonword_model;
unsigned int Nonword_model_max_order = DEFAULT_WORD_MAX_ORDER;
unsigned int Nonchar_model;
unsigned int Nonchar_model_max_order = DEFAULT_CHAR_MAX_ORDER;

boolean Output_found = FALSE, Title_found = FALSE;

char *Title = NULL;
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Word_model_out_file;
unsigned int Char_model_out_file;
unsigned int Nonword_model_out_file;
unsigned int Nonchar_model_out_file;

unsigned int Max_input_size = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options] <input-text\n"
	     "\n"
	     "options:\n"
             "  -d n\tdebugging level=n\n"
	     "  -o fn\tgeneric output filename=fn (required argument)\n"
	     "  -1 n\tmax order of word model=n (optional; default=1)\n"
	     "  -2 n\tmax order of character model=n (optional; default=4)\n"
	     "  -3 n\tmax order of non-word model=n (optional; default=1)\n"
	     "  -4 n\tmax order of non-character model=n (optional; default=4)\n"
	     "  -p n\tprogress report every n chars.\n"
	     "  -t n\ttruncate input file after n words.\n"
	     "  -T n\tlong description (title) of model (required argument)\n"
	);
    exit (2);
}

void
open_output_files (char *file_prefix)
/* Opens all the output files for writing. */
{
  char filename [MAX_FILENAME_SIZE];

  sprintf (filename, "%s_word.model", file_prefix);
  Word_model_out_file = TXT_open_file (filename, "w",
      "Writing word model into file",
      "Train: can't open word model file" );

  sprintf (filename, "%s_char.model", file_prefix);
  Char_model_out_file = TXT_open_file (filename, "w",
      "Writing character model into file",
      "Train: can't open character model file" );

  sprintf (filename, "%s_nonword.model", file_prefix);
  Nonword_model_out_file = TXT_open_file (filename, "w",
      "Writing non-word model into file",
      "Train: can't open non-word model file" );

  sprintf (filename, "%s_nonchar.model", file_prefix);
  Nonchar_model_out_file = TXT_open_file (filename, "w",
      "Writing non-character model into file",
      "Train: can't open non-character model file" );

  fprintf (stderr, "\n");
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    /* get the argument options */

    Output_found = FALSE;
    Title_found = FALSE;
    Debug.level = 0;
    while ((opt = getopt (argc, argv, "T:d:i:m:o:p:t:1:2:3:4:")) != -1)
	switch (opt)
	{
	case 'd':
	    Debug.level = atoi (optarg);
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 't':
	    Max_input_size = atoi (optarg);
	    break;
	case 'T':
	    Title_found = TRUE;
	    Model_title = (char *) malloc (strlen (optarg)+1);
	    strcpy (Model_title, optarg);
	    break;
	case '1':
	    Word_model_max_order = atoi (optarg);
	    break;
	case '2':
	    Char_model_max_order = atoi (optarg);
	    break;
	case '3':
	    Nonword_model_max_order = atoi (optarg);
	    break;
	case '4':
	    Nonchar_model_max_order = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    fprintf (stderr, "\nCreating new models\n\n");

    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Title_found)
        fprintf (stderr, "\nFatal error: missing title of the model\n\n");
    if (!Output_found || !Title_found)
      {
	usage ();
	exit (1);
      }
    for (; optind < argc; optind++)
	usage ();
}

void
process_word (unsigned int model, unsigned int *prev_words, unsigned int context, unsigned int word)
/* Trains the model from the word. */
{
    unsigned int p, q;

    /* Create the higher order contexts, and update the model with them */
    for (p = 0; p < Word_model_max_order; p++)
      {
	if (prev_words [p] == NIL)
	    break;

	TXT_setlength_text (context, 0);
	for (q = 0; q <= p; q++)
	  {
	    TXT_append_text (context, prev_words [q]);
	    TXT_append_symbol (context, TXT_sentinel_symbol ());
	    /* The sentinel symbol is used here to separate out the words in the context
	       since some "words" may in fact have spaces in them */
	  }
 
	TLM_update_context (model, context, word);
      }
}

void
process_file (unsigned int file,
	      unsigned int word_model, unsigned int nonword_model,
	      unsigned int char_model, unsigned int nonchar_model)
/* Trains the model from the words and characters in the text. */
{
    unsigned int word_context, char_context;
    unsigned int nonword_context, nonchar_context;
    unsigned int word, nonword, word_pos, p;
    unsigned int *prev_words, *prev_nonwords;
    boolean eof;

    TLM_set_context_operation (TLM_Get_Nothing);

    word = TXT_create_text ();
    nonword = TXT_create_text ();

    word_context = TXT_create_text ();
    nonword_context = TXT_create_text ();

    char_context = TLM_create_context (char_model);
    nonchar_context = TLM_create_context (nonchar_model);

    prev_nonwords = (unsigned int *) malloc (Nonword_model_max_order * sizeof (unsigned int));
    prev_words = (unsigned int *) malloc (Word_model_max_order * sizeof (unsigned int));
    for (p = 0; p < Nonword_model_max_order; p++)
        prev_nonwords [p] = NIL;
    for (p = 0; p < Word_model_max_order; p++)
        prev_words [p] = NIL;

    word_pos = 0;
    eof = FALSE;
    for (;;)
    {
        word_pos++;
	if ((Debug.progress > 0) && ((word_pos % Debug.progress) == 0))
	  {
	    fprintf (stderr, "Processing word pos %d\n", word_pos);
	    dump_memory (Stderr_File);
	  }
        /* repeat until EOF or max input */
	eof = !TXT_readword_text1 (file, nonword, word);

	/* Update the order 0 contexts */
	TLM_update_context (nonword_model, NIL, nonword);
	TLM_update_context (word_model, NIL, word);

	process_word (nonword_model, prev_nonwords, nonword_context, nonword);
	process_word (word_model, prev_words, word_context, word);

	if (Debug.range)
	  {
	    fprintf (stderr, "Processed word {");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "}\n");

	    fprintf (stderr, "Processed non-word {");
	    TXT_dump_text (Stderr_File, nonword, TXT_dump_symbol);
	    fprintf (stderr, "}\n");
	  }

	if (eof)
	    break;

	/* Rotate the words through the previous history buffers */
	for (p = 0; p < Nonword_model_max_order; p++)
	    prev_nonwords [p+1] = prev_nonwords [p];
	for (p = 0; p < Word_model_max_order; p++)
	    prev_words [p+1] = prev_words [p];
	prev_nonwords [0] = nonword;
	prev_words [0] = word;
    }

    TLM_release_context (word_model, word_context);
    TLM_release_context (char_model, char_context);
    TLM_release_context (nonword_model, nonword_context);
    TLM_release_context (nonchar_model, nonchar_context);

    TXT_release_text (nonword);
    TXT_release_text (word);
    TXT_release_text (nonword_context);
    TXT_release_text (word_context);

    if (Debug.range || (Debug.progress != 0))
        fprintf (stderr, "Processed %d words\n", word_pos);
}

int
main (int argc, char *argv[])
{
    unsigned int model_form;

    init_arguments (argc, argv);

    model_form = TLM_Static; /* TLM_Dynamic not yet implemented */

    Word_model = TLM_create_model (TLM_PT_Model, Model_title);
    Char_model = TLM_create_model
          (TLM_PPM_Model, Model_title, CHAR_ALPHABET_SIZE,
	   Char_model_max_order, PERFORMS_EXCLS);

    Nonword_model = TLM_create_model (TLM_PT_Model, Model_title);
    Nonchar_model = TLM_create_model
	  (TLM_PPM_Model, Model_title, CHAR_ALPHABET_SIZE,
	   Nonchar_model_max_order, PERFORMS_EXCLS);

    open_output_files (Output_filename);

    process_file (Stdin_File, Word_model, Nonword_model, Char_model, Nonchar_model);

    TLM_write_model (Word_model_out_file, Word_model, model_form);
    TLM_write_model (Char_model_out_file, Char_model, model_form);
    TLM_write_model (Nonword_model_out_file, Nonword_model, model_form);
    TLM_write_model (Nonchar_model_out_file, Nonchar_model, model_form);

    if (Debug.level > 4)
      {
	fprintf (stderr, "\nDump of word model:\n" );
        TLM_dump_model (Stderr_File, Word_model, NULL);

	fprintf (stderr, "\nDump of character model:\n" );
        TLM_dump_model (Stderr_File, Char_model, NULL);

	fprintf (stderr, "\nDump of nonword model:\n" );
        TLM_dump_model (Stderr_File, Nonword_model, NULL);

	fprintf (stderr, "\nDump of non-character model:\n" );
        TLM_dump_model (Stderr_File, Nonchar_model, NULL);
      }

    TLM_release_model (Word_model);
    TLM_release_model (Char_model);
    TLM_release_model (Nonword_model);
    TLM_release_model (Nonchar_model);

    exit (0);
}
