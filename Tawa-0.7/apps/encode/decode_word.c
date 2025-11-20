/* Dynamic PPMD decoder for words. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "word_utils.h"
#include "word.h"
#include "text.h"
#include "table.h"
#include "coder.h"
#include "model.h"

int
main (int argc, char *argv[])
{
    unsigned int words_model, coder, text, type, model, table;
    unsigned int input_file, output_file, model_file;

    init_word_arguments (argc, argv);

    input_file = TXT_open_file
      (Args_input_filename, "r", "Reading from text file",
       "Decode_word: can't open input text file" );

    output_file = TXT_open_file
      (Args_output_filename, "w", "Writing to text file",
       "Decode_word: can't open output text file" );

    words_model = TLM_create_words_model ();

    coder = TLM_create_arithmetic_decoder (input_file, output_file);

    if (Args_model_filename)
      { /* Load existing models and filenames */
        model_file = TXT_open_file
	    (Args_model_filename, "r", "Loading words model from file",
	     "Encode_word: can't open words model file" );
	words_model = TLM_load_words_model (model_file);
      }
    else
      { /* Create new models and tables */
	for (type=0; type < MODELS; type++)
	  {
	    model = TLM_create_model
	      (TLM_PPM_Model, Args_title, Args_alphabet_size [type],
	       Args_max_order [type], Args_escape_method [type],
	       Args_performs_full_excls [type], Args_performs_update_excls [type]);
	    TLM_set_words_model (words_model, type, model);
	  }

	table = TXT_create_table (TLM_Dynamic, 0);
	TLM_set_words_model (words_model, TLM_Words_Nonword_Table, table);
	table = TXT_create_table (TLM_Dynamic, 0);
	TLM_set_words_model (words_model, TLM_Words_Word_Table, table);
      }

    arith_decode_start (input_file);

    text = TXT_create_text ();
    TLM_process_word_text (text, words_model, DECODE_TYPE, coder);
    TXT_dump_text (output_file, text, TXT_write_symbol);

    arith_decode_finish (input_file);

    return (0);
}
