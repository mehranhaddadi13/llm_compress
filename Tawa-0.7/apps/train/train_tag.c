/* Loads or creates a tag-based model, adds some text to the model,
   then writes out the changed model. For testing the loading and
   dumping model routines.

   Warning: This is not completed.

   Tasks to do to complete tagger:

   1. Train_tag
   2. User_model routines update_context etc.
   3. tag.c to tag a file. */

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
#include "ppm_model.h"

#define WORD_ALPHABET_SIZE 0         /* Size of the word model's alphabet */
#define CHAR_ALPHABET_SIZE 256       /* Size of the character model's alphabet */
#define PERFORMS_EXCLS TRUE          /* The model performs exclusions */
#define MAX_FILENAME_SIZE 128        /* Maximum size of a filename */

#define DEFAULT_WORD_MAX_ORDER 1     /* Default max order for word model */
#define DEFAULT_CHAR_MAX_ORDER 4     /* Default max order for character model */
#define DEFAULT_WORD_ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for word model */
#define DEFAULT_CHAR_ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for character model */

char *Model_title;

unsigned int Word_table;
unsigned int Nonword_table;

unsigned int Word_model;
unsigned int Word_model_max_order = DEFAULT_WORD_MAX_ORDER;
unsigned int Word_model_escape_method = DEFAULT_WORD_ESCAPE_METHOD;
unsigned int Nonword_model;
unsigned int Nonword_model_max_order = DEFAULT_WORD_MAX_ORDER;
unsigned int Nonword_model_escape_method = DEFAULT_WORD_ESCAPE_METHOD;

unsigned int Char_model;
unsigned int Char_model_max_order = DEFAULT_CHAR_MAX_ORDER;
unsigned int Char_model_escape_method = DEFAULT_CHAR_ESCAPE_METHOD;
unsigned int Nonchar_model;
unsigned int Nonchar_model_max_order = DEFAULT_CHAR_MAX_ORDER;
unsigned int Nonchar_model_escape_method = DEFAULT_CHAR_ESCAPE_METHOD;

boolean Input_found = FALSE, Title_found = FALSE;

char *Title = NULL;

unsigned int Words_in_file;

unsigned int Static_Model = 0;
unsigned int Max_input_size = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options] <input-text\n"
	     "\n"
	     "options:\n"
             "  -d n\tdebugging level=n\n"
	     "  -m fn\twords model input filename=fn (optional argument)\n"
	     "  -1 n\tmax order of word model=n (optional; default=1)\n"
	     "  -2 n\tmax order of character model=n (optional; default=4)\n"
	     "  -3 n\tescape method of word model=c (optional; default=D)\n"
	     "  -4 n\tescape method of character model=c (optional; default=D)\n"
	     "  -p n\tprogress report every n chars.\n"
	     "  -t n\ttruncate input file after n words.\n"
	     "  -S\twrite out the model as a static model\n"
	     "  -T n\tlong description (title) of model (required argument)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;
    int escape;

    /* get the argument options */

    Input_found = FALSE;
    Title_found = FALSE;
    Debug.level = 0;
    while ((opt = getopt (argc, argv, "ST:d:m:p:t:1:2:3:4:")) != -1)
	switch (opt)
	{
	case 'S':
	    Static_Model = TRUE;
	    break;
	case 'T':
	    Title_found = TRUE;
	    Model_title = (char *) malloc (strlen (optarg)+1);
	    strcpy (Model_title, optarg);
	    break;
	case 'd':
	    Debug.level = atoi (optarg);
	    break;
	case 'm':
	    Input_found = TRUE;
	    Words_in_file = TXT_open_file
	        (optarg, "r", "Loading words model from file",
		 "Train_word: can't open words model file" );
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 't':
	    Max_input_size = atoi (optarg);
	    break;
	case '1':
	    Word_model_max_order = atoi (optarg);
	    break;
	case '2':
	    Char_model_max_order = atoi (optarg);
	    break;
	case '3':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Word_model_escape_method = escape;
	    break;
	case '4':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Char_model_escape_method = escape;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!Title_found)
      {
        fprintf (stderr, "\nFatal error: missing title of the model\n\n");
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

unsigned int
check_model (unsigned int model, int max_order)
/* Checks the model exists and that it's maximum order matches that
   of max_order. Returns the model type. */
{
    unsigned int model_type;
    unsigned int model_form;
    unsigned int max_order1;
    int model_max_order;
    char *model_title;

    if (!TLM_get_model_type (model, &model_type, &model_form, &model_title))
      {
	fprintf (stderr, "Fatal error: Invalid character model number\n");
	exit (1);
      }
    else if (model_form == TLM_Static)
      {
	fprintf (stderr, "Fatal error: This implementation does not permit further training when\n");
	fprintf (stderr, "a static model has been loaded\n");
	exit (1);
      }

    /* Check for consistency of parameters between the loaded model and
       the model to be written out */
    TLM_get_model (model, PPM_Get_Max_Order, &max_order);
    model_max_order = (int) max_order1;
    if (max_order1 != model_max_order)
      {
	fprintf (stderr, "\nFatal error: max order of output character model does not match input model\n\n");
	exit (1);
      }
    return (model_form);
}


int
main (int argc, char *argv[])
{
    unsigned int model_form, words_model, model, table;
    unsigned int nonword_model, word_model, nonchar_model, char_model;
    unsigned int nonword_table, word_table;
    unsigned int text;

    init_arguments (argc, argv);

    if (Static_Model)
        model_form = TLM_Static;
    else
        model_form = TLM_Dynamic;

    if (Input_found)
      {
	words_model = TLM_load_words_model (Words_in_file);

	TLM_get_tags_model
	    (tags_model, &char_model, &word_table);
	check_model (Word_model, Word_model_max_order);
	check_model (Char_model, Char_model_max_order);
      }
    else
      {
	words_model = TLM_create_words_model ();

	table = TXT_create_table (TLM_Dynamic, 0);
	TLM_set_words_model (words_model, TLM_Words_Nonword_Table, table);
	table = TXT_create_table (TLM_Dynamic, 0);
	TLM_set_words_model (words_model, TLM_Words_Word_Table, table);

        model = TLM_create_model
	  (TLM_PPM_Model, Model_title, WORD_ALPHABET_SIZE,
	   Nonword_model_max_order, Nonword_model_escape_method, PERFORMS_EXCLS);
	TLM_set_words_model (words_model, TLM_Words_Nonword_Model, model);

        model = TLM_create_model
	  (TLM_PPM_Model, Model_title, WORD_ALPHABET_SIZE,
	   Word_model_max_order, Word_model_escape_method, PERFORMS_EXCLS);
	TLM_set_words_model (words_model, TLM_Words_Word_Model, model);

        model = TLM_create_model
	  (TLM_PPM_Model, Model_title, CHAR_ALPHABET_SIZE,
	   Nonchar_model_max_order, Nonword_model_escape_method, PERFORMS_EXCLS);
	TLM_set_words_model (words_model, TLM_Words_Nonchar_Model, model);

        model = TLM_create_model
	  (TLM_PPM_Model, Model_title, CHAR_ALPHABET_SIZE,
	   Char_model_max_order, Word_model_escape_method, PERFORMS_EXCLS);
	TLM_set_words_model (words_model, TLM_Words_Char_Model, model);

      }

    text = TXT_load_text (Stdin_File);
    TLM_process_word_text (text, words_model, UPDATE_TYPE, NIL /* no coder */);

    TLM_write_words_model (Stdout_File, words_model, model_form);

    if (Debug.level > 4)
      {
	fprintf (stderr, "\nDump of word table:\n" );
	TXT_dump_table (Stderr_File, Word_table);

	fprintf (stderr, "\nDump of nonword table:\n" );
	TXT_dump_table (Stderr_File, Nonword_table);

	fprintf (stderr, "\nDump of word model:\n" );
        TLM_dump_model (Stderr_File, Word_model, NULL);

	fprintf (stderr, "\nDump of character model:\n" );
        TLM_dump_model (Stderr_File, Char_model, NULL);

	fprintf (stderr, "\nDump of nonword model:\n" );
        TLM_dump_model (Stderr_File, Nonword_model, NULL);

	fprintf (stderr, "\nDump of non-character model:\n" );
        TLM_dump_model (Stderr_File, Nonchar_model, NULL);
      }

    TXT_release_text (text);

    exit (0);
}
