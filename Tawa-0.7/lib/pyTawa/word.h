/* Routines for processing words in text. */

#ifndef WORD_H
#define WORD_H

#include "model.h"

#define TLM_Words_Nonword_Model 0
#define TLM_Words_Word_Model 1
#define TLM_Words_Nonchar_Model 2
#define TLM_Words_Char_Model 3
#define TLM_Words_Nonword_Table 4
#define TLM_Words_Word_Table 5

extern unsigned int Debug_progress; /* For debugging the progress of the program */

boolean
TLM_valid_words_model (unsigned int words_model);
/* Returns non-zero if the word model is valid, zero otherwize. */

unsigned int
TLM_create_words_model ();
/* Creates a new words_model record and returns a unique number associated with it. */

void
TLM_release_word_model (unsigned int word_model);
/* Releases the memory allocated to the model's word_model and the word_model number
   (which may be reused in later create_word_model calls). */

void
TLM_setup_words_model
(unsigned int words_model,
 unsigned int nonword_model, unsigned int word_model,
 unsigned int nonchar_model, unsigned int char_model,
 unsigned int nonword_table, unsigned int word_table);
/* Sets up the words_model record to use the specified word and nonword word models
   to predict the sequence of text. */

void
TLM_get_words_model
(unsigned int words_model,
 unsigned int *nonword_model, unsigned int *word_model,
 unsigned int *nonchar_model, unsigned int *char_model,
 unsigned int *nonword_table, unsigned int *word_table);
/* Returns the word and nonword models associated with the word model record. */

void
TLM_set_words_model (unsigned int words_model, unsigned int type, unsigned int model_or_table);
/* Sets the words model or table of the specified type to model. */

void
TLM_write_words_model (unsigned int file, unsigned int words_model,
		       unsigned int model_form);
/* Writes out the model or table for the words model to the file. */

unsigned int
TLM_load_words_model (unsigned int file);
/* Loads the words models which has been previously saved to a file into memory
   and allocates a new words model number which is returned. */

unsigned int
TLM_read_words_model (char *filename, char *debug_line, char *error_line);
/* Reads in the words model directly by loading the model from the file
   with the specified filename. */

float
TLM_process_symbol (unsigned int symbol, unsigned int model,
		    unsigned int context, codingType coding_type,
		    unsigned int coder);
/* Process the symbol in the context depending on the coding_type of operation. */

float
TLM_process_word (unsigned int text, unsigned int word,
		  unsigned word_model, unsigned word_context,
		  unsigned char_model, unsigned char_context,
		  codingType coding_type, unsigned int coder, 
		  unsigned int table, boolean eof);
/* Returns the codelength for processing the word using the PPM contexts. */

float
TLM_process_word_text
(unsigned int text, unsigned int words_model, codingType coding_type, unsigned int coder);
/* Trains the model from the characters in the text. */

#endif
