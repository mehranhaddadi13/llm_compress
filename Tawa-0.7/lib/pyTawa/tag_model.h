/* TLM routines based on TAG models (parts of speech). */

#ifndef TAG_MODEL_H
#define TAG_MODEL_H

struct TAG_modelType
{ /* TAG model record */
    unsigned int TAG_tagset_table;   /* The list of tags table */
    unsigned int TAG_words_table;    /* The list of words (and their word ids) table */
    unsigned int TAG_words_index;    /* The list of tags associated with each word id */

    unsigned int TAG_tags_model;     /* The PPM tags model used to predict the tags */
    unsigned int TAG_words_model;    /* The PT words model used to predict the words */
    unsigned int TAG_chars_model;    /* Single PPM chars model used to predict the characters */
    unsigned int *TAG_chars_models;  /* Separate PPM chars models for each tag (rather than
				        using a single chars model; this is an option when
					that can be specified when creating the model) */
    boolean TAG_deleted;             /* Set when record has been deleted */
    unsigned int TAG_next;           /* Next in unused list (used for deleting) */
};

struct TAG_contextType
{ /* Record of current TAG context */
    unsigned int TAG_tags_context;   /* Current TAG tags context */
    unsigned int TAG_chars_context;  /* Current TAG characters context */
    unsigned int *TAG_chars_contexts;/* Current TAG characters contexts (if we have multiple
				        chars models for each tag) */
    unsigned int TAG_prev_word;      /* Previous word */
    boolean TAG_deleted;             /* Set when record has been deleted */
    unsigned int TAG_next;           /* Next in unused list (used for deleting) */
};

/* Global variables used for storing the TAG models and contexts */
extern struct TAG_modelType *TAG_Models;     /* List of TAG models */
extern struct TAG_contextType *TAG_Contexts; /* List of TAG contexts */

boolean
TAG_valid_model (unsigned int tag_model);
/* Returns non-zero if the TAG model is valid, zero otherwize. */

unsigned int
TAG_create_model (unsigned int model, unsigned int tagset_table,
		  int tags_model_max_order, int chars_model_max_order,
		  boolean has_multiple_chars_models);
/* Creates and returns a new pointer to a TAG model record. */

void
TAG_get_model (unsigned int tag_model, unsigned int *tagset_table,
	       unsigned int *words_table, unsigned int *words_index,
	       unsigned int *tags_alphabet_size, unsigned int *tags_model,
	       unsigned int *chars_model, unsigned int **chars_models);
/* Returns information about the TAG model. */

void
TAG_release_model (unsigned int tag_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later TAG_create_model or TAG_load_model calls). */

unsigned int
TAG_load_model (unsigned int file, unsigned int model, unsigned int model_form);
/* Loads the TAG model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
TAG_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form);
/* Writes out the TAG model to the file. */

void
TAG_dump_model (unsigned int file, unsigned int tag_model);
/* Dumps out the TAG model (for debugging purposes). */

boolean
TAG_valid_context (unsigned int tag_context);
/* Returns non-zero if the TAG context is valid, zero otherwize. */

unsigned int
TAG_create_context (unsigned int tag_model);
/* Creates and returns a new pointer to a TAG context record. */

void
TAG_release_context (unsigned int tag_model, unsigned int tag_context);
/* Releases the memory allocated to the context and the context number (which may
   be reused in later TAG_create_context call). */

unsigned int
TAG_copy_context (unsigned int model, unsigned int context);
/* Creates a new TAG context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the TAG context being copied is for a dynamic model. */

void
TAG_update_context (unsigned int model, unsigned int context_text,
		    unsigned int symbol_text);
/* Updates the TAG context record so that the current symbol becomes tag_word_text.
   tag_word_symbol is assumed to be a symbol from which the tag and word can be
   extracted by mod-ing it against the tag alphabet size to extract out the word
   (with the remainder being the tag_id). */

void
TAG_encode_symbol (unsigned int model, unsigned int context_text,
		   unsigned int coder, unsigned int tag_word_text);
/* Encodes and updates the TAG context record so that the current symbol becomes the
   encoded tag_word_text.
   tag_word_symbol is assumed to be a symbol from which the tag and word can be
   extracted by mod-ing it against the tag alphabet size to extract out the word
   (with the remainder being the tag_id). */

unsigned int
TAG_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder);
/* Decodes and Updates the TAG context record so that the current symbol becomes the
   decoded tag_word_text. Returns a text record; NIL on EOF.
   The decoded text record is a sequence of text symbols on
   two lines of text. On one line is a tag, and the next contains the word. */

unsigned int
TAG_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the TAG context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */

#endif
