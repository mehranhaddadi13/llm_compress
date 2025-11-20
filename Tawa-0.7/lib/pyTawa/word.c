/* Routines for processing words in text. */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include "io.h"
#include "word.h"
#include "coder.h"
#include "model.h"
#include "table.h"
#include "text.h"

#define WORDS_MODELS_SIZE 20     /* Initial size of words_models array */

struct words_model_type
{ /* words model record */
  unsigned int W_nonword_model; /* Model associated with sequence of non-words */
  unsigned int W_word_model;    /* Model associated with sequence of words */
  unsigned int W_nonchar_model; /* Model associated with sequence of novel non-word characters */
  unsigned int W_char_model;    /* Model associated with sequence of nodel word characters */
  unsigned int W_nonword_table; /* Table of non-words (for looking up nonword id nos.) */
  unsigned int W_word_table;    /* Table of words (for looking up word id nos.) */
  unsigned int W_next;          /* Used to find the "next" on the used list for deleted words */
  boolean W_valid;              /* True if this record is in use */
};

struct words_model_type *Words_models = NULL; /* List of words model records */
unsigned int Words_models_max_size = 0; /* Current max. size of the word models array */
unsigned int Words_models_used = NIL;   /* List of deleted word model records */
unsigned int Words_models_unused = 1;   /* Next unused word model record */

boolean
TLM_valid_words_model (unsigned int words_model)
/* Returns non-zero if the word model is valid, zero otherwize. */
{
    if (words_model == NIL)
        return (FALSE);
    else if (words_model >= Words_models_unused)
        return (FALSE);
    else
        return (Words_models [words_model].W_valid);
}

unsigned int
create_words_model ()
/* Creates a words_model record. */
{
    struct words_model_type *words_model;
    unsigned int t, old_size;

    if (Words_models_used != NIL)
    {	/* use the first list of words_models on the used list */
	t = Words_models_used;
	Words_models_used = Words_models [t].W_next;
    }
    else
    {
	t = Words_models_unused;
	if (Words_models_unused+1 >= Words_models_max_size)
	{ /* need to extend Words_models array */
	    old_size = Words_models_max_size * sizeof (struct words_model_type);
	    if (Words_models_max_size == 0)
		Words_models_max_size = WORDS_MODELS_SIZE;
	    else
		Words_models_max_size *= 2; /* Keep on doubling the array on demand */
	    Words_models = (struct words_model_type *) Realloc (171, Words_models,
		     Words_models_max_size * sizeof (struct words_model_type), old_size);

	    if (Words_models == NULL)
	    {
		fprintf (stderr, "Fatal error: out of word models space\n");
		exit (1);
	    }
	}
	Words_models_unused++;
    }

    if (t != NIL)
    {
      words_model = Words_models + t;

      words_model->W_nonword_model = NIL;
      words_model->W_word_model = NIL;
      words_model->W_nonchar_model = NIL;
      words_model->W_char_model = NIL;
      words_model->W_nonword_table = NIL;
      words_model->W_word_table = NIL;
      words_model->W_next = NIL;
      words_model->W_valid = TRUE;
    }
    return (t);
}

unsigned int
TLM_create_words_model ()
/* Creates a new words_model record and returns a unique number associated with it. */
{
    unsigned int words_model;

    words_model = create_words_model ();

    return (words_model);
}

void
TLM_release_words_model (unsigned int words_model)
/* Releases the memory allocated to the model's words_model and the words_model number
   (which may be reused in later create_words_model calls). */
{
    struct words_model_type *words_modelp;

    assert (TLM_valid_words_model (words_model));

    words_modelp = Words_models + words_model;

    TLM_release_model (words_modelp->W_nonword_model);
    TLM_release_model (words_modelp->W_word_model);
    TLM_release_model (words_modelp->W_nonchar_model);
    TLM_release_model (words_modelp->W_char_model);
    TXT_release_table (words_modelp->W_nonword_table);
    TXT_release_table (words_modelp->W_word_table);

    /* Append onto head of the used list */
    words_modelp->W_next = Words_models_used;
}

void
TLM_setup_words_model
(unsigned int words_model,
 unsigned int nonword_model, unsigned int word_model,
 unsigned int nonchar_model, unsigned int char_model,
 unsigned int nonword_table, unsigned int word_table)
/* Sets up the words_model record to use the specified word and nonword word models
   to predict the sequence of text. */
{
    struct words_model_type *words_modelp;

    assert (TLM_valid_words_model (words_model));

    assert (TLM_valid_model (word_model));
    assert (TLM_valid_model (nonword_model));
    assert (TLM_valid_model (char_model));
    assert (TLM_valid_model (nonchar_model));
    assert (TLM_valid_model (word_table));
    assert (TLM_valid_model (nonword_table));

    words_modelp = Words_models + words_model;

    words_modelp->W_nonword_model = nonword_model;
    words_modelp->W_word_model = word_model;
    words_modelp->W_nonchar_model = nonchar_model;
    words_modelp->W_char_model = char_model;
    words_modelp->W_nonword_table = nonword_table;
    words_modelp->W_word_table = word_table;
}

void
TLM_get_words_model
(unsigned int words_model,
 unsigned int *nonword_model, unsigned int *word_model,
 unsigned int *nonchar_model, unsigned int *char_model,
 unsigned int *nonword_table, unsigned int *word_table)
/* Returns the word and nonword models associated with the word model record. */
{
    struct words_model_type *words_modelp;

    assert (TLM_valid_words_model (words_model));

    words_modelp = Words_models + words_model;

    *nonword_model = words_modelp->W_nonword_model;
    *word_model    = words_modelp->W_word_model;
    *nonchar_model = words_modelp->W_nonchar_model;
    *char_model    = words_modelp->W_char_model;
    *nonword_table = words_modelp->W_nonword_table;
    *word_table    = words_modelp->W_word_table;

    assert (TLM_valid_model (*nonword_model));
    assert (TLM_valid_model (*word_model));
    assert (TLM_valid_model (*nonchar_model));
    assert (TLM_valid_model (*char_model));
    assert (TXT_valid_table (*nonword_table));
    assert (TXT_valid_table (*word_table));
}

void
TLM_set_words_model (unsigned int words_model, unsigned int type, unsigned int model_or_table)
/* Sets the words model or table of the specified type to model. */
{
    struct words_model_type *words_modelp;

    assert (TLM_valid_words_model (words_model));
    assert ((TLM_Words_Nonword_Model <= type) && (type <= TLM_Words_Word_Table));

    words_modelp = Words_models + words_model;

    switch (type)
      {
      case TLM_Words_Nonword_Model:
	assert (TLM_valid_model (model_or_table));
        words_modelp->W_nonword_model = model_or_table;
	break;
      case TLM_Words_Word_Model:
	assert (TLM_valid_model (model_or_table));
        words_modelp->W_word_model = model_or_table;
	break;
      case TLM_Words_Nonchar_Model:
	assert (TLM_valid_model (model_or_table));
        words_modelp->W_nonchar_model = model_or_table;
	break;
      case TLM_Words_Char_Model:
	assert (TLM_valid_model (model_or_table));
        words_modelp->W_char_model = model_or_table;
	break;
      case TLM_Words_Nonword_Table:
	assert (TXT_valid_table (model_or_table));
        words_modelp->W_nonword_table = model_or_table;
	break;
      case TLM_Words_Word_Table:
	assert (TXT_valid_table (model_or_table));
        words_modelp->W_word_table = model_or_table;
	break;
      default:
	break;
      }
}

void
TLM_write_words_model (unsigned int file, unsigned int words_model,
		       unsigned int model_form)
/* Writes out the model or table for the words model to the file. */
{
    struct words_model_type *words_modelp;

    assert (TLM_valid_words_model (words_model));

    words_modelp = Words_models + words_model;

    TLM_write_model (file, words_modelp->W_nonword_model, model_form);
    TLM_write_model (file, words_modelp->W_word_model, model_form);
    TLM_write_model (file, words_modelp->W_nonchar_model, model_form);
    TLM_write_model (file, words_modelp->W_char_model, model_form);
    TXT_write_table (file, words_modelp->W_nonword_table, model_form);
    TXT_write_table (file, words_modelp->W_word_table, model_form);
}

unsigned int
TLM_load_words_model (unsigned int file)
/* Loads the words models which has been previously saved to a file into memory
   and allocates a new words model number which is returned. */
{
    struct words_model_type *words_modelp;
    unsigned int words_model;

    words_model = TLM_create_words_model ();

    words_modelp = Words_models + words_model;

    words_modelp->W_nonword_model = TLM_load_model (file);
    words_modelp->W_word_model = TLM_load_model (file);
    words_modelp->W_nonchar_model = TLM_load_model (file);
    words_modelp->W_char_model = TLM_load_model (file);
    words_modelp->W_nonword_table = TXT_load_table (file);
    words_modelp->W_word_table = TXT_load_table (file);

    return (words_model);
}

unsigned int
TLM_read_words_model (char *filename, char *debug_line, char *error_line)
/* Reads in the words model directly by loading the model from the file
   with the specified filename. */
{
    unsigned int file, words_model;

    file = TXT_open_file (filename, "r", debug_line, error_line);
    assert (TXT_valid_file (file));

    words_model = TLM_load_words_model (file);
    assert (TLM_valid_words_model (words_model));

    TXT_close_file (file);

    return (words_model);
}

float
TLM_process_symbol (unsigned int symbol, unsigned int model,
		    unsigned int context, codingType coding_type,
		    unsigned int coder)
/* Process the symbol in the context depending on the coding_type of operation. */
{
  float codelength;

  if ((coding_type == ENCODE_TYPE) || (coding_type == DECODE_TYPE))
      assert (coder = TLM_valid_coder (coder));

  codelength = 0.0;
  switch (coding_type)
    {
    case UPDATE_TYPE:
    case UPDATE1_TYPE:
    case UPDATE2_TYPE:
      TLM_update_context (model, context, symbol);
      break;
    case ENCODE_TYPE:
      TLM_encode_symbol (model, context, coder, symbol);
      break;
    case FIND_CODELENGTH_TYPE:
    case UPDATE_CODELENGTH_TYPE:
    case UPDATE1_CODELENGTH_TYPE:
    case FIND_MAXORDER_TYPE:
    case UPDATE_MAXORDER_TYPE:
      TLM_set_context_operation (TLM_Get_Codelength);
      TLM_update_context (model, context, symbol);
      codelength = TLM_Codelength;
      break;
    case FIND_CODERANGES_TYPE:
    case UPDATE_CODERANGES_TYPE:
      TLM_set_context_operation (TLM_Get_Coderanges);
      TLM_update_context (model, context, symbol);
      codelength = TLM_codelength_coderanges (TLM_Coderanges);
      break;
    default:
      coding_type = ENCODE_TYPE; /* force an assert error below */
      assert (coding_type == DECODE_TYPE); /* Decoding is treated separately */
      break;
    }
  return (codelength);
}

float
TLM_process_word (unsigned int text, unsigned int word,
		  unsigned word_model, unsigned word_context,
		  unsigned char_model, unsigned char_context,
		  codingType coding_type, unsigned int coder, 
		  unsigned int table, boolean eof)
/* Returns the codelength for processing the word using the PPM contexts. */
{
  unsigned int table_type, types, tokens;
  unsigned int symbol, count, pos;

  if ((coding_type == ENCODE_TYPE) || (coding_type == DECODE_TYPE))
      assert (coder = TLM_valid_coder (coder));

  TXT_getinfo_table (table, &table_type, &types, &tokens);

  if (coding_type == DECODE_TYPE)
  {
    symbol = TLM_decode_symbol (word_model, word_context, coder);
    if (symbol < types)
      { /* word existed before */
	TXT_overwrite_text (word, TXT_getkey_table (table, symbol));
	TXT_append_text (text, word);
      }
    else
      { /* escape encoded - decode the new word character by character */
	TXT_setlength_text (word, 0);
	if (symbol == TXT_sentinel_symbol ())
	    TXT_append_symbol (word, TXT_sentinel_symbol ());
	else
	  {
	    for (;;)
	      { /* keep decoding until sentinel symbol is found -
		   this marks the end of a word */
		symbol = TLM_decode_symbol (char_model, char_context, coder);
		if (symbol == TXT_sentinel_symbol ())
		    break;
		TXT_append_symbol (word, symbol);
	      }

	    if (table_type == TLM_Dynamic)
	        TXT_update_table (table, word, &symbol, &count);
	    TXT_append_text (text, word);
	  }
      }
    
    return (0.0);
  }
  else
  {
    boolean new_word;
    float codelength;

    if (!eof && (word != NIL))
      if (table_type == TLM_Dynamic)
	new_word = TXT_update_table (table, word, &symbol, &count);
      else
	new_word = !TXT_getid_table (table, word, &symbol, &count);
    else
      {
	new_word = FALSE;
	symbol = TXT_sentinel_symbol ();
      }

    codelength = TLM_process_symbol (symbol, word_model, word_context,
				     coding_type, coder);

    if (new_word)
      { /* escape encoded - now encode using character model */
	pos = 0;
	while (TXT_get_symbol (word, pos++, &symbol))
	  codelength += TLM_process_symbol (symbol, char_model, char_context,
					    coding_type, coder);

	/* Each end of word is marked by the end of sentinel symbol */
	codelength +=
	    TLM_process_symbol (TXT_sentinel_symbol (), char_model,
				char_context, coding_type, coder);
      }
    return (codelength);
  }
}

float
TLM_process_word_text
(unsigned int text, unsigned int words_model, codingType coding_type, unsigned int coder)
/* Trains the words model from the characters in the text. */
{
    unsigned int word_table, nonword_table, word_model, nonword_model;
    unsigned int char_model, nonchar_model;
    unsigned int word_context, char_context;
    unsigned int nonword_context, nonchar_context;
    unsigned int word, nonword, word_pos, text_pos, p;
    unsigned int nonword_text_pos, word_text_pos;
    char full_stop1 [3], full_stop2 [3], full_stop3 [3];
    float codelength;
    boolean eof;

    assert (TLM_valid_words_model (words_model));
    TLM_get_words_model (words_model, &nonword_model, &word_model,
			 &nonchar_model, &char_model, &nonword_table, &word_table);

    if ((coding_type == ENCODE_TYPE) || (coding_type == DECODE_TYPE))
        assert (coder = TLM_valid_coder (coder));

    strcpy (full_stop1, ". ");
    strcpy (full_stop2, ".\n");
    strcpy (full_stop3, ".");

    word = TXT_create_text ();
    nonword = TXT_create_text ();

    word_context = TLM_create_context (word_model);
    char_context = TLM_create_context (char_model);
    nonword_context = TLM_create_context (nonword_model);
    nonchar_context = TLM_create_context (nonchar_model);

    codelength = 0.0;

    text_pos = 0;
    word_pos = 0;
    eof = FALSE;
    for (;;)
    {
        word_pos++;
	bytes_input = text_pos;
	if ((Debug.progress > 0) && ((word_pos % Debug.progress) == 0))
	  {
	    if (coding_type == DECODE_TYPE)
	      fprintf (stderr, "Processing word pos %d bytes output %d\n",
		       word_pos, TXT_length_text (text));
	    else
	      fprintf (stderr, "Processing word pos %d bytes input %d bytes output %d %.3f bpc\n",
		       word_pos, bytes_input, bytes_output,
		       (8.0 * bytes_output) / bytes_input);
	  }
        /* repeat until EOF or max input */
	if (coding_type != DECODE_TYPE)
	    eof = !TXT_getword_text
	        (text, nonword, word, &text_pos, &nonword_text_pos, &word_text_pos);

	codelength += TLM_process_word (text, nonword, nonword_model,
                      nonword_context, nonchar_model, nonchar_context,
		      coding_type, coder, nonword_table, FALSE);

	if (Debug.range)
	  {
	    fprintf (stderr, "Processed non-word {");
	    TXT_dump_text (Stderr_File, nonword, TXT_dump_symbol);
	    fprintf (stderr, "} text pos = %d\n", text_pos);
	  }

	/* insert a break at the end of a sentence */
	if (TXT_getstr_text (nonword, full_stop1, &p) ||
	    TXT_getstr_text (nonword, full_stop2, &p) ||
	    !TXT_strcmp_text (nonword, full_stop3)) /* found end of a sentence? */
	  { /* we'll assume so */
	    /* insert (but not encode) a break into the context
	       as this will improve prediction by around 1 per cent */
	    TLM_update_context (nonword_model, nonword_context,
				TXT_sentinel_symbol ());
	  }

	codelength += TLM_process_word (text, word, word_model, word_context,
		      char_model, char_context, coding_type, coder,
		      word_table, eof);

	if (Debug.range)
	  {
	    fprintf (stderr, "Processed word {");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "}\n");
	  }

	if (eof || ((coding_type == DECODE_TYPE) && TXT_sentinel_text (word)))
	    break;
    }

    TLM_release_context (word_model, word_context);
    TLM_release_context (char_model, char_context);
    TLM_release_context (nonword_model, nonword_context);
    TLM_release_context (nonchar_model, nonchar_context);

    TXT_release_text (word);
    TXT_release_text (nonword);

    if (Debug.range || (Debug.progress != 0))
      {
	if (coding_type == DECODE_TYPE)
	  fprintf (stderr, "Processing word pos %d bytes output %d\n",
		   word_pos, TXT_length_text (text));
	else
	  fprintf (stderr, "Processing word pos %d bytes input %d bytes output %d %.3f bpc\n",
		   word_pos, TXT_length_text (text) - 1, bytes_output,
		   (8.0 * bytes_output) / bytes_input);
      }
    return (codelength);
}
