/* Program to test encoding using probability tables. */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "coder.h"
#include "pt_ptable.h"
#include "pt_ctable.h"

unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;
unsigned int Nonchar_order = 4;
unsigned int Char_order = 4;
boolean Performs_exclusions = TRUE;

void
junk (void)
/* Used for debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options]\n"
	     "\n"
	     "options:\n"
	     "  -E\tdoes not perform exclusions\n"
	     "  -d n\tdebug level=n\n"
	     "  -p n\tdebug progress=n\n"
	     "  -r n\tdebug arithmetic coding ranges\n"
	     "  -1 n\torder of non-character model=n\n"
	     "  -2 n\torder of character model=n\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "Ed:p:r1:2:")) != -1)
	switch (opt)
	{
	case 'E':
	    Performs_exclusions = FALSE;
	    break;
	case 'd':
	    Debug_level = atoi (optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	case '1':
	    Nonchar_order = atoi (optarg);
	    break;
	case '2':
	    Char_order = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}
    for (; optind < argc; optind++)
	usage ();
}

void
encode_word (unsigned int output_file, unsigned int coder,
	     unsigned int context, unsigned int word,
	     struct PTc_table_type *order1_table, struct PTp_table_type *order0_table,
	     unsigned int char_model, unsigned int char_context, boolean performs_exclusions)
/* Encodes the word using the context. */
{
    struct PTp_table_type *exclusions_table;
    unsigned int lbnd, hbnd, totl, pos, symbol;

    if (context == NIL)
      lbnd = 0;
    else
      {
	PTc_encode_arith_range (order1_table, NULL, context, word, &lbnd, &hbnd, &totl);
	if (totl)
	  {
	    if (Debug.range || (Debug_level > 0))
	      fprintf (stderr, "order 1, lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	    arith_encode (output_file, lbnd, hbnd, totl);
	  }
      }

    if (!lbnd)
      { /* escape */
	if (!performs_exclusions)
	    exclusions_table = NULL;
	else
	    exclusions_table = PTc_findcontext_table ();

	PTp_encode_arith_range (order0_table, exclusions_table, word, &lbnd, &hbnd, &totl);
	if (Debug.range || (Debug_level > 0))
	    fprintf (stderr, "order 0, lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	arith_encode (output_file, lbnd, hbnd, totl);

	/* Check for inconsistancies - debug purposes only (it's quite slow - O(N^2)) */
	if ((Debug_level > 3) && performs_exclusions)
	  if (!PTp_check_arith_ranges (order0_table, exclusions_table, FALSE))
	    {
	      fprintf (stderr, "*** Error *** Invalid exclusions table ***\n");
	      PTp_check_arith_ranges (order0_table, exclusions_table, TRUE);
	    }

	if (!lbnd)
	  { /* escape to character level */
	    pos = 0;
	    while (TXT_get_symbol (word, pos++, &symbol))
	      {
	        TLM_encode_symbol (char_model, char_context, coder, symbol);
		if (Debug_level > 0)
		  fprintf (stderr, "Encoding char symbol %d\n", symbol);
	      }
	    /* Note that each end of word is marked by the end of sentinel symbol */
	    if (symbol != TXT_sentinel_symbol ())
	      {
	        TLM_encode_symbol (char_model, char_context, coder, TXT_sentinel_symbol ());
		if (Debug_level > 0)
		    fprintf (stderr, "Encoding char symbol <sentinel>\n");
	      }
	  }
      }
}

void
encode_text (unsigned int text, unsigned int output_file, unsigned int coder,
	     unsigned int nonchar_model, unsigned int char_model,
	     boolean performs_exclusions)
/* Processes the words in the text. */
{
    struct PTc_table_type Order1_NonTable;
    struct PTp_table_type Order0_NonTable;
    struct PTc_table_type Order1_Table;
    struct PTp_table_type Order0_Table;
    unsigned int word, nonword, prev_word, prev_nonword;
    unsigned int word_pos, word_text_pos, nonword_text_pos, text_pos;
    unsigned int nonchar_context, char_context;
    boolean eof, first_time;

    nonchar_context = TLM_create_context (nonchar_model);
    char_context = TLM_create_context (char_model);
    word = TXT_create_text ();
    nonword = TXT_create_text ();
    prev_word = NIL;
    prev_nonword = NIL;

    text_pos = 0;
    word_pos = 0;
    eof = FALSE;
    first_time = TRUE;

    PTp_init_table (&Order0_NonTable);
    PTc_init_table (&Order1_NonTable);
    PTp_init_table (&Order0_Table);
    PTc_init_table (&Order1_Table);

    prev_nonword = TXT_create_text ();
    prev_word = TXT_create_text ();

    for (;;)
    {
        word_pos++;
	if ((Debug_progress > 0) && ((word_pos % Debug_progress) == 0))
	    fprintf (stderr, "Processing word pos %d\n", word_pos);

        /* repeat until EOF or max input */
	eof = !TXT_getword_text1
	    (text, nonword, word, &text_pos, &nonword_text_pos, &word_text_pos);

	encode_word (output_file, coder, prev_nonword, nonword, &Order1_NonTable, &Order0_NonTable,
		     nonchar_model, nonchar_context, performs_exclusions);
	if (Debug_level > 0)
	  {
	    fprintf (stderr, "encoded non-word [");
	    TXT_dump_text (Stderr_File, nonword, TXT_dump_symbol);
	    fprintf (stderr, "]\n");
	  }
	if (!first_time)
	    PTc_update_table (&Order1_NonTable, prev_nonword, nonword, 1, NULL);
	PTp_update_table (&Order0_NonTable, nonword, 1);
	if (Debug_level > 3)
	  {
	    fprintf (stderr, "\nDump of order 1 nonword model:\n");
	    PTc_dump_table (Stderr_File, &Order1_NonTable);

	    fprintf (stderr, "\nDump of order 0 nonword model:\n");
	    PTp_dump_table (Stderr_File, &Order0_NonTable);
	  }

	encode_word (output_file, coder, prev_word, word, &Order1_Table, &Order0_Table,
		     char_model, nonchar_context, performs_exclusions);
	if (Debug_level > 0)
	  {
	    fprintf (stderr, "encoded word [");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "]\n");
	  }

	if (eof)
	    break;

	if (!first_time)
	    PTc_update_table (&Order1_Table, prev_word, word, 1, NULL);
	PTp_update_table (&Order0_Table, word, 1);
	if (Debug_level > 3)
	  {
	    fprintf (stderr, "\nDump of order 1 word model:\n");
	    PTc_dump_table (Stderr_File, &Order1_Table);

	    fprintf (stderr, "\nDump of order 0 word model:\n");
	    PTp_dump_table (Stderr_File, &Order0_Table);
	  }

	TXT_overwrite_text (prev_nonword, nonword);
	TXT_overwrite_text (prev_word, word);

	first_time = FALSE;
    }

    TLM_release_context (nonchar_model, nonchar_context);
    TLM_release_context (char_model, char_context);

    TXT_release_text (word);
    TXT_release_text (nonword);
    TXT_release_text (prev_word);
    TXT_release_text (prev_nonword);

    if (Debug_level > 0)
        fprintf (stderr, "Processed %d words\n", word_pos-1);
}

int
main (int argc, char *argv[])
{
    unsigned int nonchar_model, char_model;
    unsigned int input_text, coder;

    Coder = TLM_create_arithmetic_coder ();

    arith_encode_start (Stdout_File);

    init_arguments (argc, argv);

    nonchar_model = TLM_create_model (TLM_PPM_Model, "Test nonchar", 256, Nonchar_order,
				      TLM_PPM_Method_D, TRUE);
    char_model = TLM_create_model (TLM_PPM_Model, "Test char", 256, Char_order,
				   TLM_PPM_Method_D, TRUE);

    input_text = TXT_load_text (Stdin_File);
    bytes_input = TXT_length_text (input_text);
    encode_text (input_text, Stdout_File, coder, nonchar_model, char_model, Performs_exclusions);

    arith_encode_finish (Stdout_File);

    fprintf (stderr, "Bytes input %d, bytes output %d (%.3f bpc)\n", bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);


    return (1);
}
