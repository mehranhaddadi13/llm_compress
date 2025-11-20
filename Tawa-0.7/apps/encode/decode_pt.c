/* Program to test decoding using probability tables. */
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

unsigned int Decoded_Word = NIL; /* global text used by decode_word so that it doesn't have to create
				    new text regions for every new word, but instead overwrites old one */

unsigned int
decode_word (unsigned int input_file, unsigned int coder, unsigned int context,
	     struct PTc_table_type *order1_table, struct PTp_table_type *order0_table,
	     unsigned int char_model, unsigned int char_context, boolean performs_exclusions)
/* Decodes the word using the context. Note that the text region used to store the returned word may or
   may not be overwritten by a latter invocation of this routine. */
{
    struct PTp_table_type *exclusions_table;
    unsigned int lbnd, hbnd, totl, target, word, pos, symbol;

    lbnd = 0;
    if (context != NIL)
      {
	totl = PTc_decode_arith_total (order1_table, NULL, context);
	if (totl)
	  {
	    target = arith_decode_target (input_file, totl);
	    word = PTc_decode_arith_key (order1_table, NULL, context, target, totl, &lbnd, &hbnd);
	    if (Debug.range || (Debug_level > 0))
	        fprintf (stderr, "order 1, lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	    arith_decode (input_file, lbnd, hbnd, totl);
	    if (word != NIL)
	        return (word);
	  }
      }

    if (Decoded_Word == NIL)
        Decoded_Word = TXT_create_text ();
    if (!lbnd)
      { /* escape */
	if (!performs_exclusions)
	    exclusions_table = NULL;
	else
	    exclusions_table = PTc_findcontext_table ();

	totl = PTp_decode_arith_total (order0_table, exclusions_table);
	if (totl)
	  {
	    target = arith_decode_target (input_file, totl);
	    word = PTp_decode_arith_key (order0_table, exclusions_table, target, totl, &lbnd, &hbnd);
	    if (Debug.range || (Debug_level > 0))
	        fprintf (stderr, "order 0, lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	    arith_decode (input_file, lbnd, hbnd, totl);
	    if (word != NIL)
	        return (word);
	  }

	if (!lbnd || !totl)
	  { /* escape to character level */
	    pos = 0;
	    TXT_setlength_text (Decoded_Word, 0);
	    for (;;)
	      { /* while not end of word do */
	        symbol = TLM_decode_symbol (char_model, char_context, coder);
		if (symbol == TXT_sentinel_symbol ())
		  {
		    if (Debug_level > 0)
		        fprintf (stderr, "Decoded char symbol <sentinel>\n");
		    break;
		  }
		if (Debug_level > 0)
		  fprintf (stderr, "Decoded char symbol %d\n", symbol);
		TXT_append_symbol (Decoded_Word, symbol);
	      }
	  }
      }

    return (Decoded_Word);
}

void
decode_text (unsigned int input_file, unsigned int coder,
	     unsigned int nonchar_model, unsigned int char_model,
	     boolean performs_exclusions)
/* Processes the words in the text. */
{
    struct PTc_table_type Order1_NonTable;
    struct PTp_table_type Order0_NonTable;
    struct PTc_table_type Order1_Table;
    struct PTp_table_type Order0_Table;
    unsigned int word, nonword, prev_word, prev_nonword, word_pos, text_pos;
    unsigned int nonchar_context, char_context;
    boolean eof, first_time;

    nonchar_context = TLM_create_context (nonchar_model);
    char_context = TLM_create_context (char_model);
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

    prev_word = TXT_create_text ();
    prev_nonword = TXT_create_text ();

    for (;;)
    {
        word_pos++;
	if ((Debug_progress > 0) && ((word_pos % Debug_progress) == 0))
	    fprintf (stderr, "Processing word pos %d\n", word_pos);

	nonword = decode_word (input_file, coder, prev_nonword, &Order1_NonTable, &Order0_NonTable,
			       nonchar_model, nonchar_context, performs_exclusions);
	if (TXT_sentinel_text (nonword))
	    break; /* reached eof */

	TXT_dump_text (Stdout_File, nonword, NULL);
	if (TXT_length_text (nonword) == 0)
	    TXT_append_symbol (nonword, TXT_sentinel_symbol ());
	if (Debug_level > 0)
	  {
	    fprintf (stderr, "decoded nonword [");
	    TXT_dump_text (Stderr_File, nonword, NULL);
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
	TXT_overwrite_text (prev_nonword, nonword);

	word = decode_word (input_file, coder, prev_word, &Order1_Table, &Order0_Table,
			    char_model, char_context, performs_exclusions);
	TXT_dump_text (Stdout_File, word, NULL);
	if (Debug_level > 0)
	  {
	    fprintf (stderr, "decoded word [");
	    TXT_dump_text (Stderr_File, word, NULL);
	    fprintf (stderr, "]\n");
	  }

	if (TXT_length_text (word) == 0)
	    break; /* reached eof */

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

	TXT_overwrite_text (prev_word, word);

	first_time = FALSE;
    }

    TLM_release_context (nonchar_model, nonchar_context);
    TLM_release_context (char_model, char_context);

    TXT_release_text (prev_word);
    TXT_release_text (prev_nonword);

    if (Debug_level > 0)
        fprintf (stderr, "Processed %d words\n", word_pos-1);
}

int
main (int argc, char *argv[])
{
    unsigned int nonchar_model, char_model;
    unsigned int coder;

    Coder = TLM_create_arithmetic_coder ();

    arith_decode_start (Stdin_File);

    init_arguments (argc, argv);

    nonchar_model = TLM_create_model (TLM_PPM_Model, "Test nonchar", 256, Nonchar_order,
				      TLM_PPM_Method_D, TRUE);
    char_model = TLM_create_model (TLM_PPM_Model, "Test char", 256, Char_order,
				   TLM_PPM_Method_D, TRUE);

    decode_text (Stdin_File, coder, nonchar_model, char_model, Performs_exclusions);

    arith_decode_finish (Stdin_File);

    return (1);
}
