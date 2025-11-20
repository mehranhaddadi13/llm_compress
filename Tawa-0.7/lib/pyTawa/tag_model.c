/* TLM routines based on TAG models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "sets.h"
#include "text.h"
#include "list.h"
#include "table.h"
#include "index.h"
#include "model.h"
#include "ppm_model.h"
#include "pt_model.h"
#include "pt_ptable.h"
#include "pt_ctable.h"
#include "tag_model.h"

#define TAG_MODELS_SIZE 4          /* Initial max. number of models */
#define TAG_CONTEXTS_SIZE 8        /* Initial max. number of contexts */

#define DEFAULT_TAGS_ESCAPE_METHOD TLM_PPM_Method_D  /* Default escape method for tags model */
#define DEFAULT_TAGS_PERFORMS_EXCLS TRUE             /* Perform exclusions for tags model */

#define DEFAULT_CHARS_ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for chars model */
#define DEFAULT_CHARS_PERFORMS_EXCLS TRUE             /* Perform exclusions for chars model */
#define DEFAULT_CHARS_ALPHABET_SIZE 256              /* Default size of the character model's alphabet */

/* Global variables used for storing the TAG models */
struct TAG_modelType *TAG_Models = NULL; /* List of TAG models */
unsigned int TAG_Models_max_size = 0;   /* Current max. size of models array */
unsigned int TAG_Models_used = NIL;     /* List of deleted model records */
unsigned int TAG_Models_unused = 1;     /* Next unused model record */

/* Global variables used for storing the TAG contexts */
struct TAG_contextType *TAG_Contexts = NULL; /* List of TAG contexts */
unsigned int TAG_Contexts_max_size = 0;   /* Current max. size of contexts array */
unsigned int TAG_Contexts_used = NIL;     /* List of deleted context records */
unsigned int TAG_Contexts_unused = 1;     /* Next unused context record */

/* Global variables used by TAG_update_context, TAG_encode_symbol and TAG_decode_symbol. */
unsigned int TAG_Words_Context = NIL;     /* Word context - note: this is a text record */

void
TAG_debug ()
/* For debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

boolean
TAG_valid_model (unsigned int model)
/* Returns non-zero if the TAG model is valid, zero otherwize. */
{
    if (model == NIL)
        return (FALSE);
    else if (model >= TAG_Models_unused)
        return (FALSE);
    else if (TAG_Models [model].TAG_deleted)
        /* This gets set to TRUE when the model gets deleted */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_TAG_model (void)
/* Creates and returns a new pointer to a TAG model record. */
{
    unsigned int model, old_size;

    if (TAG_Models_used != NIL)
    { /* use the first record on the used list */
        model = TAG_Models_used;
	TAG_Models_used = TAG_Models [model].TAG_next;
    }
    else
    {
	model = TAG_Models_unused;
        if (TAG_Models_unused >= TAG_Models_max_size)
	{ /* need to extend TAG_Models array */
	    old_size = TAG_Models_max_size * sizeof (struct modelType);
	    if (TAG_Models_max_size == 0)
	        TAG_Models_max_size = TAG_MODELS_SIZE;
	    else
	        TAG_Models_max_size = 10*(TAG_Models_max_size+50)/9;

	    TAG_Models = (struct TAG_modelType *)
	        Realloc (124, TAG_Models, TAG_Models_max_size *
			 sizeof (struct TAG_modelType), old_size);

	    if (TAG_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of TAG models space\n");
		exit (1);
	    }
	}
	TAG_Models_unused++;
    }

    return (model);
}

void
TAG_init_model (void)
/* Used by TAG_create_model () and TAG_load_model to initialise
   some global variables. */
{
    if (TAG_Words_Context == NIL)
      {
        TAG_Words_Context = TXT_create_text ();
	TXT_init_sets (256, 3, 8, TRUE);
      }
}

unsigned int
TAG_create_model (unsigned int model, unsigned int tagset_table,
		  int tags_model_max_order, int chars_model_max_order,
		  boolean has_multiple_chars_models)
/* Creates and returns a new pointer to a TAG model record. */
{
    unsigned int tagset_type, tagset_types_count, tagset_tokens_count;
    unsigned int tag_model, tag;

    assert (TLM_valid_model (model));

    TAG_init_model ();

    tag_model = create_TAG_model ();

    TXT_getinfo_table (tagset_table, &tagset_type, &tagset_types_count,
		       &tagset_tokens_count);
    TAG_Models [tag_model].TAG_tagset_table = tagset_table;

    TAG_Models [tag_model].TAG_words_table = TXT_create_table (TLM_Dynamic, 0);
    TAG_Models [tag_model].TAG_words_index = TXT_create_index ();

    TAG_Models [tag_model].TAG_tags_model = TLM_create_model
      (TLM_PPM_Model, Models [model].M_title, tagset_types_count,
       tags_model_max_order, DEFAULT_TAGS_ESCAPE_METHOD, DEFAULT_TAGS_PERFORMS_EXCLS);

    TAG_Models [tag_model].TAG_words_model = TLM_create_model
      (TLM_PT_Model, Models [model].M_title);

    if (!has_multiple_chars_models)
      {
        TAG_Models [tag_model].TAG_chars_models = NULL;
	TAG_Models [tag_model].TAG_chars_model = TLM_create_model
	  (TLM_PPM_Model, Models [model].M_title, DEFAULT_CHARS_ALPHABET_SIZE,
	   chars_model_max_order, DEFAULT_CHARS_ESCAPE_METHOD, DEFAULT_CHARS_PERFORMS_EXCLS);
      }
    else
      { /* Create a separate chars model for each tag */
        TAG_Models [tag_model].TAG_chars_models = (unsigned int *)
	  Calloc (140, tagset_types_count+1, sizeof (unsigned int));

	for (tag = 0; tag < tagset_types_count; tag++)
	  {
	    TAG_Models [tag_model].TAG_chars_models [tag] = TLM_create_model
	      (TLM_PPM_Model, Models [model].M_title, DEFAULT_CHARS_ALPHABET_SIZE,
	       chars_model_max_order, DEFAULT_CHARS_ESCAPE_METHOD,
	       DEFAULT_CHARS_PERFORMS_EXCLS);
	  }
      }

    TAG_Models [tag_model].TAG_deleted = FALSE;
    TAG_Models [tag_model].TAG_next = NIL;

    return (tag_model);
}

void
TAG_get_model (unsigned int tag_model, unsigned int *tagset_table,
	       unsigned int *words_table, unsigned int *words_index,
	       unsigned int *tags_alphabet_size, unsigned int *tags_model,
	       unsigned int *chars_model, unsigned int **chars_models)
/* Returns information about the TAG model. */
{
    assert (TAG_valid_model (tag_model));

    *tagset_table = TAG_Models [tag_model].TAG_tagset_table;
    *words_table = TAG_Models [tag_model].TAG_words_table;
    *words_index = TAG_Models [tag_model].TAG_words_index;
    *tags_model = TAG_Models [tag_model].TAG_tags_model;
    *chars_model = TAG_Models [tag_model].TAG_chars_model;
    *chars_models = TAG_Models [tag_model].TAG_chars_models;
    TLM_get_model (TAG_Models [tag_model].TAG_tags_model, PPM_Get_Alphabet_Size,
		   tags_alphabet_size);
}

void
TAG_release_model (unsigned int tag_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later TAG_create_model or TAG_load_model calls). */
{
    unsigned int tagset_types_count, tagset_tokens_count;
    unsigned int tagset_type, tag;

    assert (TAG_valid_model (tag_model));

    TLM_release_model (TAG_Models [tag_model].TAG_tags_model);
    TLM_release_model (TAG_Models [tag_model].TAG_words_model);

    if (TAG_Models [tag_model].TAG_chars_models == NULL)
        TLM_release_model (TAG_Models [tag_model].TAG_chars_model);
    else
      {
	TXT_getinfo_table (TAG_Models [tag_model].TAG_tagset_table,
			   &tagset_type, &tagset_types_count, &tagset_tokens_count);
	for (tag = 0; tag < tagset_types_count; tag++)
	    TLM_release_model (TAG_Models [tag_model].TAG_chars_models [tag]);

	Free (140, TAG_Models [tag_model].TAG_chars_models,
	      (tagset_types_count+1) * (sizeof (unsigned int)));
      }

    /* add model record at the head of the TAG_Models_used list */
    TAG_Models [tag_model].TAG_next = TAG_Models_used;
    TAG_Models_used = tag_model;

    TAG_Models [tag_model].TAG_deleted = TRUE; /* Used for testing if model no. is valid or not */
}

unsigned int
TAG_load_model (unsigned int file, unsigned int model, unsigned int model_form)
/* Loads the TAG model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int tag_model, tags_model, words_model;
    unsigned int words_table, words_index, max_words_index;
    unsigned int tagset_table, taglist, tag_id, pos;
    unsigned int tagset_table_type, tagset_types_count, tagset_tokens_count;
    unsigned int words_table_type, words_types_count, words_tokens_count;
    unsigned int has_multiple_chars_models;
    /*int tags_max_order, chars_max_order, max_order;*/

    assert (TXT_valid_file (file));
    assert (TLM_valid_model (model));

    /* Get next unused TAG model record */
    tag_model = create_TAG_model ();
    assert (tag_model != NIL);

    TAG_init_model ();

    tagset_table = TXT_load_table (file);
    TXT_getinfo_table (tagset_table, &tagset_table_type,
		       &tagset_types_count, &tagset_tokens_count);
    fprintf (stderr, "Reading in tagset table, types %d tokens %d\n",
	     tagset_types_count, tagset_tokens_count);

    tags_model = TLM_load_model (file);
    words_model = TLM_load_model (file);

    has_multiple_chars_models = fread_int (file, INT_SIZE);
    if (!has_multiple_chars_models)
      {
	TAG_Models [tag_model].TAG_chars_models = NULL;
        TAG_Models [tag_model].TAG_chars_model = TLM_load_model (file);
      }
    else
      {
        TAG_Models [tag_model].TAG_chars_models = (unsigned int *)
	  Calloc (140, tagset_types_count+1, sizeof (unsigned int));

	for (tag_id = 0; tag_id < tagset_types_count; tag_id++)
	  {
	    TAG_Models [tag_model].TAG_chars_models [tag_id] = TLM_load_model (file);
	  }
      }

    /*
    PPM_get_model (tags_model, PPM_Get_Max_Order, &max_order);
    tags_max_order = (int) max_order;
    PPM_get_model (chars_model, PPM_Get_Max_Order, &max_order);
    chars_max_order = (int) max_order;
    */

    /* Read in the list of words and the taglist associated with them */
    words_table = TXT_load_table (file);
    TXT_getinfo_table (words_table, &words_table_type,
		       &words_types_count, &words_tokens_count);
    fprintf (stderr, "Reading in words table, types %d tokens %d\n",
	     words_types_count, words_tokens_count);

    words_index = TXT_create_index ();

    max_words_index = fread_int (file, INT_SIZE);
    fprintf (stderr, "Reading in index %d words\n", max_words_index);

    for (pos = 0; pos <= max_words_index; pos++)
      {
	taglist = TXT_load_list (file);

	/*
	fprintf (stderr, "List at pos %d = ", pos);
	TXT_dump_list (Stderr_File, taglist, NULL);
	fprintf (stderr, "\n");
	*/

	assert (taglist != NIL);
	TXT_put_index (words_index, pos, taglist);
      }

    TAG_Models [tag_model].TAG_tagset_table = tagset_table;
    TAG_Models [tag_model].TAG_tags_model = tags_model;
    TAG_Models [tag_model].TAG_words_model = words_model;
    TAG_Models [tag_model].TAG_words_table = words_table;
    TAG_Models [tag_model].TAG_words_index = words_index;
    TAG_Models [tag_model].TAG_deleted = FALSE;
    TAG_Models [tag_model].TAG_next = NIL;

    return (tag_model);
}

void
TAG_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form)
/* Writes out the TAG model to the file as a static TAG model (note: not fast TAG) 
   (which can then be loaded by other applications later). */
{
    unsigned int tag_model, taglist, words_index, max_words_index, pos;
    unsigned int tagset_table_type, tagset_types, tagset_tokens, tag_id;
    unsigned int words_table_type, words_types, words_tokens;

    tag_model = TLM_verify_model (model, TLM_TAG_Model, TAG_valid_model);

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    /* write out the tables and models */
    TXT_write_table (file, TAG_Models [tag_model].TAG_tagset_table, model_form);
    TXT_getinfo_table (TAG_Models [tag_model].TAG_tagset_table,
		       &tagset_table_type, &tagset_types, &tagset_tokens);
    fprintf (stderr, "Writing out tagset table, types %d tokens %d\n",
	     tagset_types, tagset_tokens);

    TLM_write_model (file, TAG_Models [tag_model].TAG_tags_model, model_form);
    TLM_write_model (file, TAG_Models [tag_model].TAG_words_model, model_form);

    if (TAG_Models [tag_model].TAG_chars_models == NULL)
      {
        fwrite_int (file, FALSE, INT_SIZE);
        TLM_write_model (file, TAG_Models [tag_model].TAG_chars_model, model_form);
      }
    else
      {
        fwrite_int (file, TRUE, INT_SIZE);
	for (tag_id = 0; tag_id < tagset_types; tag_id++)
	  {
	    TLM_write_model (file, TAG_Models [tag_model].TAG_chars_models [tag_id],
			     model_form);
	  }
      }

    /* Write out the list of words and the taglist associated with them */
    TXT_write_table (file, TAG_Models [tag_model].TAG_words_table, model_form);
    TXT_getinfo_table (TAG_Models [tag_model].TAG_words_table,
		       &words_table_type, &words_types, &words_tokens);
    fprintf (stderr, "Writing out words table, types %d tokens %d\n",
	     words_types, words_tokens);

    words_index = TAG_Models [tag_model].TAG_words_index;
    max_words_index = TXT_max_index (words_index);
    fprintf (stderr, "Writing out index %d words\n", max_words_index);
 
    fwrite_int (file, max_words_index, INT_SIZE);
    for (pos = 0; pos <= max_words_index; pos++)
      {
	assert (TXT_get_index (words_index, pos, &taglist));
	assert (taglist != NIL);

	/*
	fprintf (stderr, "List at pos %d = ", pos);
	TXT_dump_list (Stderr_File, taglist, NULL);
	fprintf (stderr, "\n");
	*/

	TXT_write_list (file, taglist);
      }
}

unsigned int TAG_Dump_Tag_Model = NIL;

void
TAG_dump_tag_symbol (unsigned int file, unsigned int tag_id)
{
    unsigned int tagset_table, tags_alphabet_size, tag;
    unsigned int tags_model, chars_model, *chars_models; 
    unsigned int words_table, words_index;

    TLM_get_model (TAG_Dump_Tag_Model, &tagset_table, &words_table,
		   &words_index, &tags_alphabet_size, &tags_model,
		   &chars_model, &chars_models);
    tag = TXT_getkey_table (tagset_table, tag_id);

    fprintf (Files [file], "<");
    TXT_dump_text (file, tag, NULL);
    fprintf (Files [file], ">");
}

unsigned int TAG_Dump_Tagset_Table = NIL;

void
TAG_dump_words_taglist
(unsigned int file, unsigned int tag_id, unsigned int tag_count)
/* Dumps out the tag in the words taglist. (for debugging purposes) */
{
    unsigned int tag;

    tag = TXT_getkey_table (TAG_Dump_Tagset_Table, tag_id);
    fprintf (Files [file], " ");
    TXT_dump_text (file, tag, NULL);
    fprintf (Files [file], " (%d) ", tag_count);
}

unsigned int TAG_Dump_Words_Table = NIL;

void
TAG_dump_words_index (unsigned int file, unsigned int index_pos,
		      unsigned int index_taglist)
/* Dumps out the words index i.e. the set of tags associated with each word id.
   (for debugging purposes) */
{
    unsigned int word;

    assert (index_taglist != NIL);
    assert (TXT_valid_list (index_taglist));
    fprintf (Files [file], "Index %d : word = ", index_pos);
    word = TXT_getkey_table (TAG_Dump_Words_Table, index_pos);
    TXT_dump_text (file, word, NULL);

    fprintf (Files [file], "; tags =");
    TXT_dump_list (file, index_taglist, TAG_dump_words_taglist);
    fprintf (Files [file], "\n");
}
   
void
TAG_dump_model (unsigned int file, unsigned int tag_model)
/* Dumps out the TAG model (for debugging purposes). */
{
    unsigned int tagset_type, tagset_types_count, tagset_tokens_count;
    unsigned int tag, tag_id;

    assert (TAG_valid_model (tag_model));

    TAG_Dump_Tagset_Table = TAG_Models [tag_model].TAG_tagset_table;
    TXT_getinfo_table (TAG_Dump_Tagset_Table, &tagset_type, &tagset_types_count,
		       &tagset_tokens_count);

    /*
    fprintf (Files [file], "Dump of tagset table:\n");
    TXT_dump_table (file, TAG_Dump_Tagset_Table);

    fprintf (Files [file], "Dump of words table:\n");
    TXT_dump_table (file, TAG_Models [tag_model].TAG_words_table);

    fprintf (Files [file], "Dump of words index:\n");
    TAG_Dump_Words_Table = TAG_Models [tag_model].TAG_words_table;
    TXT_dump_index (file, TAG_Models [tag_model].TAG_words_index,
		    TAG_dump_words_index);
    */
    TAG_Dump_Tag_Model = tag_model;

    fprintf (Files [file], "\nDump of tags model:\n");
    TLM_dump_model (file, TAG_Models [tag_model].TAG_tags_model, TAG_dump_tag_symbol);

    TAG_Dump_Tag_Model = NIL;

    fprintf (Files [file], "\nDump of words model:\n");
    TLM_dump_model (file, TAG_Models [tag_model].TAG_words_model, NULL);

    fprintf (Files [file], "\nDump of chars model:\n");
    if (TAG_Models [tag_model].TAG_chars_models == NULL)
        TLM_dump_model (file, TAG_Models [tag_model].TAG_chars_model, NULL);
    else
      {
	for (tag_id = 0; tag_id < tagset_types_count; tag_id++)
	  {
	    fprintf (Files [file], "\nDump of chars model for tag ");
	    tag = TXT_getkey_table (TAG_Models [tag_model].TAG_tagset_table, tag_id);
	    fprintf (stderr, " [");
	    TXT_dump_text (Stderr_File, tag, NULL);
	    fprintf (stderr, "] tag id: %d\n", tag_id);
	    TLM_dump_model (file, TAG_Models [tag_model].TAG_chars_models [tag_id], NULL);
	  }
      }
}

boolean
TAG_valid_context (unsigned int context)
/* Returns non-zero if the TAG context is valid, zero otherwize. */
{
    if (context == NIL)
        return (FALSE);
    else if (context >= TAG_Contexts_unused)
        return (FALSE);
    else if (TAG_Contexts [context].TAG_deleted)
        /* This gets set to TRUE when the context gets deleted */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_TAG_context (void)
/* Creates and returns a new pointer to a TAG context record. */
{
    unsigned int context, old_size;

    if (TAG_Contexts_used != NIL)
    { /* use the first record on the used list */
        context = TAG_Contexts_used;
	TAG_Contexts_used = TAG_Contexts [context].TAG_next;
    }
    else
    {
	context = TAG_Contexts_unused;
        if (TAG_Contexts_unused >= TAG_Contexts_max_size)
	{ /* need to extend TAG_Contexts array */
	    old_size = TAG_Contexts_max_size * sizeof (struct TAG_contextType);
	    if (TAG_Contexts_max_size == 0)
	        TAG_Contexts_max_size = TAG_CONTEXTS_SIZE;
	    else
	        TAG_Contexts_max_size = 10*(TAG_Contexts_max_size+50)/9;

	    TAG_Contexts = (struct TAG_contextType *)
	        Realloc (124, TAG_Contexts, TAG_Contexts_max_size *
			 sizeof (struct TAG_contextType), old_size);

	    if (TAG_Contexts == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of TAG contexts space\n");
		exit (1);
	    }
	}
	TAG_Contexts_unused++;
    }

    return (context);
}

unsigned int
TAG_create_context (unsigned int tag_model)
/* Creates and returns a new pointer to a TAG context record. */
{
    unsigned int tag_context, tagset_type, tag;
    unsigned int tagset_types_count, tagset_tokens_count;

    assert (TAG_valid_model (tag_model));

    TXT_getinfo_table (TAG_Models [tag_model].TAG_tagset_table,
		       &tagset_type, &tagset_types_count, &tagset_tokens_count);
    tag_context = create_TAG_context ();

    TAG_Contexts [tag_context].TAG_tags_context = TLM_create_context
      (TAG_Models [tag_model].TAG_tags_model);

    if (TAG_Models [tag_model].TAG_chars_models == NULL)
      {
	TAG_Contexts [tag_context].TAG_chars_contexts = NULL;
        TAG_Contexts [tag_context].TAG_chars_context = TLM_create_context
	  (TAG_Models [tag_model].TAG_chars_model);
      }
    else
      {
        TAG_Contexts [tag_context].TAG_chars_contexts = (unsigned int *)
	  Calloc (141, tagset_types_count+1, sizeof (unsigned int));

	for (tag = 0; tag < tagset_types_count; tag++)
	  {
	    TAG_Contexts [tag_context].TAG_chars_contexts [tag] = TLM_create_context
	      (TAG_Models [tag_model].TAG_chars_models [tag]);
	  }
      }

    TAG_Contexts [tag_context].TAG_prev_word = NIL;

    TAG_Contexts [tag_context].TAG_deleted = FALSE;
    TAG_Contexts [tag_context].TAG_next = NIL;

    return (tag_context);
}

void
TAG_release_context (unsigned int tag_model, unsigned int tag_context)
/* Releases the memory allocated to the context and the context number (which may
   be reused in later TAG_create_context call). */
{
    unsigned int tagset_types_count, tagset_tokens_count;
    unsigned int tags_model, tagset_type, tag;
    /*unsigned int prev_word;*/

    assert (TAG_valid_model (tag_model));
    assert (TAG_valid_context (tag_context));

    tags_model = TAG_Models [tag_model].TAG_tags_model;

    TLM_release_context (tags_model, TAG_Contexts [tag_context].TAG_tags_context);

    if (TAG_Contexts [tag_context].TAG_chars_contexts == NULL)
        TLM_release_context (TAG_Models [tag_model].TAG_chars_model,
			     TAG_Contexts [tag_context].TAG_chars_context);
    else
      {
	TXT_getinfo_table (TAG_Models [tag_model].TAG_tagset_table,
			   &tagset_type, &tagset_types_count, &tagset_tokens_count);
	for (tag = 0; tag < tagset_types_count; tag++)
	    TLM_release_context (TAG_Models [tag_model].TAG_chars_models [tag],
				 TAG_Contexts [tag_context].TAG_chars_contexts [tag]);

	Free (141, TAG_Contexts [tag_context].TAG_chars_contexts,
	      (tagset_types_count+1) * (sizeof (unsigned int)));
      }

    /* We can't do the following as the previous word is used
       by other contexts:
    prev_word = TAG_Contexts [tag_context].TAG_prev_word;
    if (prev_word != NIL)
        TXT_release_text (prev_word);
    */

    /* add context record at the head of the TAG_Contexts_used list */
    TAG_Contexts [tag_context].TAG_next = TAG_Contexts_used;
    TAG_Contexts_used = tag_context;

    TAG_Contexts [tag_context].TAG_deleted = TRUE; /* Used for testing if context no. is valid or not */
}

void
TAG_copy_context1 (unsigned int tag_model, unsigned int tag_context,
		   unsigned int new_context)
/* Copies the contents of the specified context into the new context. */
{
    unsigned int tagset_type, tagset_types_count, tagset_tokens_count;
    unsigned int tag_id;

    TAG_Contexts [new_context].TAG_tags_context =
        TLM_copy_context (TAG_Models [tag_model].TAG_tags_model,
			  TAG_Contexts [tag_context].TAG_tags_context);

    if (TAG_Contexts [tag_context].TAG_chars_contexts == NULL)
        TAG_Contexts [new_context].TAG_chars_context =
	    TLM_copy_context (TAG_Models [tag_model].TAG_chars_model,
			      TAG_Contexts [tag_context].TAG_chars_context);
    else
      {
	TXT_getinfo_table (TAG_Models [tag_model].TAG_tagset_table,
			   &tagset_type, &tagset_types_count, &tagset_tokens_count);
	TAG_Contexts [new_context].TAG_chars_contexts =
	    Calloc (141, tagset_types_count+1, sizeof (unsigned int));

	for (tag_id = 0; tag_id < tagset_types_count; tag_id++)
	  TAG_Contexts [new_context].TAG_chars_contexts [tag_id] =
	    TLM_copy_context (TAG_Models [tag_model].TAG_chars_models [tag_id],
			      TAG_Contexts [tag_context].TAG_chars_contexts [tag_id]);
      }

    TAG_Contexts [new_context].TAG_prev_word =
        TAG_Contexts [tag_context].TAG_prev_word;

    TAG_Contexts [new_context].TAG_deleted =
        TAG_Contexts [tag_context].TAG_deleted;
    TAG_Contexts [new_context].TAG_next =
        TAG_Contexts [tag_context].TAG_next;
}

unsigned int
TAG_copy_context (unsigned int model, unsigned int context)
/* Creates a new TAG context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the TAG context being copied is for a dynamic model. */
{
    unsigned int tag_model;
    unsigned int new_context;

    tag_model = TLM_verify_model (model, TLM_TAG_Model, TAG_valid_model);

    assert (TAG_valid_context (context));

    new_context = create_TAG_context ();

    TAG_copy_context1 (tag_model, context, new_context);
    return (new_context);
}

void
TAG_update_context (unsigned int model, unsigned int context,
		    unsigned int tag_word_symbol)
/* Updates the TAG context record so that the current symbol becomes tag_word_symbol.
   tag_word_symbol is assumed to be a symbol from which the tag and word can be
   extracted by mod-ing it against the tag alphabet size to extract out the word
   (with the remainder being the tag_id). */
{
    unsigned int word, word_id, word_count, words_taglist;
    unsigned int tags_alphabet_size, tag_id, pos, symbol;
    unsigned int tag_model, model_form;
    unsigned int chars_model, chars_context;
    unsigned int prev_word;
    float codelength;
    
    model_form = Models [model].M_model_form;

    tag_model = TLM_verify_model (model, TLM_TAG_Model, TAG_valid_model);

    assert (TAG_valid_context (context));

    codelength = 0;
    if (tag_word_symbol == 0)
      { /* EOF */
	tag_id = 0;
	word = NIL;
      }
    else
      { /* Extract both the tag and the word from the tag_word_symbol */
	TLM_get_model (TAG_Models [tag_model].TAG_tags_model, PPM_Get_Alphabet_Size,
		       &tags_alphabet_size);
	tag_id = tag_word_symbol % tags_alphabet_size;
	word = tag_word_symbol / tags_alphabet_size;
	assert (TXT_valid_text (word));

	if (Debug.range)
	  {
	    unsigned int tag;

	    tag = TXT_getkey_table (TAG_Models [tag_model].TAG_tagset_table, tag_id);
	    fprintf (stderr, " [");
	    TXT_dump_text (Stderr_File, tag, NULL);
	    fprintf (stderr, "] tag id: %d\n", tag_id);
	  }

	/* Update the words table, and the list of tags and their frequencies
	   associated with the word */
	if (model_form != TLM_Static)
	  {
	    TXT_update_table (TAG_Models [tag_model].TAG_words_table, word,
			      &word_id, &word_count);
	    if (!TXT_get_index (TAG_Models [tag_model].TAG_words_index,
				word_id, &words_taglist))
	      {
		words_taglist = TXT_create_list ();
		TXT_put_index (TAG_Models [tag_model].TAG_words_index, word_id,
			       words_taglist);
	      }
	    /* Add the number tag_id into the list of tags associated with the word */
	    TXT_put_list (words_taglist, tag_id, 1);
	  }
      }

    /* Update the tag context */
    TLM_update_context (TAG_Models [tag_model].TAG_tags_model,
			TAG_Contexts [context].TAG_tags_context, tag_id);
    codelength += TLM_Codelength;

    if (tag_word_symbol == 0)
        return;

    PT_Novel_Symbols = TRUE;

    prev_word = TAG_Contexts [context].TAG_prev_word;
    if (prev_word != NIL)
      { /* Encode and update the p(Wn | Tn Wn-1) context */

	TXT_setlength_text (TAG_Words_Context, 0);
	TXT_append_symbol (TAG_Words_Context, tag_id);
	TXT_append_text (TAG_Words_Context, prev_word);
	TXT_append_symbol (TAG_Words_Context, TXT_sentinel_symbol ());
	/* The sentinel symbol is used here to separate out the words in
	   the context since some "words" may in fact have spaces in them */
	TLM_update_context (TAG_Models [tag_model].TAG_words_model,
			    TAG_Words_Context, word);
	codelength += TLM_Codelength;
      }

    /* Create the higher order word contexts, and update the words model with them */
    /* Encode and update the p(Wn | Tn) context */
    TXT_setlength_text (TAG_Words_Context, 0);
    TXT_append_symbol (TAG_Words_Context, tag_id);
    if (PT_Novel_Symbols)
      {
	TLM_update_context (TAG_Models [tag_model].TAG_words_model,
			    TAG_Words_Context, word);
	codelength += TLM_Codelength;
      }

    /* Encode and update the order 0 word context */
    TXT_setlength_text (TAG_Words_Context, 0);
    TXT_append_symbol (TAG_Words_Context, TXT_sentinel_symbol ());
    if (PT_Novel_Symbols)
      {
	TLM_update_context (TAG_Models [tag_model].TAG_words_model,
			    TAG_Words_Context, word);
	codelength += TLM_Codelength;
      }

    /* Encode and update the character contexts */
    if (PT_Novel_Symbols)
      {
	if (Debug.level > 2)
	  {
	    fprintf (stderr, "Novel word: ");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "\n");
	  }

	if (TAG_Models [tag_model].TAG_chars_models == NULL)
	  {
	    chars_model = TAG_Models [tag_model].TAG_chars_model;
	    chars_context = TAG_Contexts [context].TAG_chars_context;
	  }
	else
	  {
	    chars_model = TAG_Models [tag_model].TAG_chars_models [tag_id];
	    chars_context = TAG_Contexts [context].TAG_chars_contexts [tag_id];
	  }

	pos = 0;
	while (TXT_get_symbol (word, pos, &symbol))
	  {
	    TLM_update_context (chars_model, chars_context, symbol);
	    codelength += TLM_Codelength;
	    pos++;
	  }
	/* Terminate the word with sentinel symbol as some "words" may have
	   nulls in them */
	TLM_update_context (chars_model, chars_context, TXT_sentinel_symbol ());
	codelength += TLM_Codelength;
      }

    /* Set prev_word for next time */
    TAG_Contexts [context].TAG_prev_word = word;

    TLM_Codelength = codelength;
}

void
TAG_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int tag_word_symbol)
/* Encodes and updates the TAG context record so that the current symbol becomes the
   encoded tag_word_text.
   tag_word_symbol is assumed to be a symbol from which the tag and word can be
   extracted by mod-ing it against the tag alphabet size to extract out the word
   (with the remainder being the tag_id). */
{
    unsigned int word, tags_alphabet_size;
    unsigned int tag_id, pos, symbol;
    unsigned int tag_model;
    unsigned int prev_word;

    tag_model = TLM_verify_model (model, TLM_TAG_Model, TAG_valid_model);

    assert (TAG_valid_context (context));

    if (tag_word_symbol == 0)
      { /* EOF */
	tag_id = 0;
	word = NIL;
      }
    else
      { /* Extract both the tag and the word from the tag_word_symbol */
	TLM_get_model (TAG_Models [tag_model].TAG_tags_model, PPM_Get_Alphabet_Size,
		       &tags_alphabet_size);
	tag_id = tag_word_symbol % tags_alphabet_size;
	word = tag_word_symbol / tags_alphabet_size;
	assert (TXT_valid_text (word));

	if (Debug.range)
	  {
	    unsigned int tag;

	    tag = TXT_getkey_table (TAG_Models [tag_model].TAG_tagset_table, tag_id);
	    fprintf (stderr, " [");
	    TXT_dump_text (Stderr_File, tag, NULL);
	    fprintf (stderr, "] tag id: %d\n", tag_id);
	  }
      }

    /* Encode and update the tag context */
    TLM_encode_symbol (TAG_Models [tag_model].TAG_tags_model,
		       TAG_Contexts [context].TAG_tags_context, coder, tag_id);

    if (tag_word_symbol == 0)
        return;

    PT_Novel_Symbols = TRUE;

    prev_word = TAG_Contexts [context].TAG_prev_word;
    if (prev_word != NIL)
      { /* Encode and update the p(Wn | Tn Wn-1) context */

	TXT_setlength_text (TAG_Words_Context, 0);
	TXT_append_symbol (TAG_Words_Context, tag_id);
	TXT_append_text (TAG_Words_Context, prev_word);
	TXT_append_symbol (TAG_Words_Context, TXT_sentinel_symbol ());
	/* The sentinel symbol is used here to separate out the words in
	   the context since some "words" may in fact have spaces in them */
	TLM_encode_symbol (TAG_Models [tag_model].TAG_words_model,
			   TAG_Words_Context, coder, word);
      }

    /* Create the higher order word contexts, and update the words model with them */
    /* Encode and update the p(Wn | Tn) context */
    TXT_setlength_text (TAG_Words_Context, 0);
    TXT_append_symbol (TAG_Words_Context, tag_id);
    if (PT_Novel_Symbols)
	TLM_encode_symbol (TAG_Models [tag_model].TAG_words_model,
			   TAG_Words_Context, coder, word);

    /* Encode and update the order 0 word context */
    TXT_setlength_text (TAG_Words_Context, 0);
    TXT_append_symbol (TAG_Words_Context, TXT_sentinel_symbol ());
    if (PT_Novel_Symbols)
	TLM_encode_symbol (TAG_Models [tag_model].TAG_words_model,
			   TAG_Words_Context, coder, word);

    /* Encode and update the character contexts */
    if (PT_Novel_Symbols)
      {
	if (Debug.level > 2)
	  {
	    fprintf (stderr, "Novel word: ");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "\n");
	  }

	pos = 0;
	while (TXT_get_symbol (word, pos, &symbol))
	  {
	    TLM_encode_symbol (TAG_Models [tag_model].TAG_chars_model,
			       TAG_Contexts [context].TAG_chars_context, coder, symbol);
	    pos++;
	  }
	/* Terminate the word with sentinel symbol as some "words" may have
	   nulls in them */
	TLM_encode_symbol (TAG_Models [tag_model].TAG_chars_model,
			   TAG_Contexts [context].TAG_chars_context, coder,
			   TXT_sentinel_symbol ());
      }

    /* Set prev_word for next time */
    TAG_Contexts [context].TAG_prev_word = word;
}

unsigned int
TAG_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder)
/* Decodes and Updates the TAG context record so that the current symbol becomes the
   decoded tag_word_text. Returns a text record; NIL on EOF.
   The decoded text record is a sequence of text symbols on
   two lines of text. On one line is a tag, and the next contains the word. */
{
    unsigned int tag_id, word, symbol, tags_alphabet_size;
    unsigned int tag_model, tag_word_symbol;
    unsigned int prev_word;

    tag_model = TLM_verify_model (model, TLM_TAG_Model, TAG_valid_model);

    assert (TAG_valid_context (context));

    /* decode and update the tag context */
    tag_id = TLM_decode_symbol (TAG_Models [tag_model].TAG_tags_model,
				TAG_Contexts [context].TAG_tags_context, coder);
    if (!tag_id) /* EOF */
        return (NIL);

    if (Debug.range)
        fprintf (stderr, " tag id: %d\n", tag_id);
    
    PT_Novel_Symbols = TRUE;

    word = NIL;
    prev_word = TAG_Contexts [context].TAG_prev_word;
    if (prev_word != NIL)
      { /* Decode and update the p(Wn | Tn Wn-1) context */
	TXT_setlength_text (TAG_Words_Context, 0);
	TXT_append_symbol (TAG_Words_Context, tag_id);
	TXT_append_text (TAG_Words_Context, prev_word);
	TXT_append_symbol (TAG_Words_Context, TXT_sentinel_symbol ());
	/* The sentinel symbol is used here to separate out the words in
	   the context since some "words" may in fact have spaces in them */
	word = TLM_decode_symbol (TAG_Models [tag_model].TAG_words_model,
				  TAG_Words_Context, coder);
      }

    /* Create the higher order word contexts, and update the words model with them */
    /* Decode and update the p(Wn | Tn) context */
    TXT_setlength_text (TAG_Words_Context, 0);
    TXT_append_symbol (TAG_Words_Context, tag_id);
    if (PT_Novel_Symbols)
	word = TLM_decode_symbol (TAG_Models [tag_model].TAG_words_model,
				  TAG_Words_Context, coder);

    /* Decode and update the order 0 word context */
    TXT_setlength_text (TAG_Words_Context, 0);
    TXT_append_symbol (TAG_Words_Context, TXT_sentinel_symbol ());
    if (PT_Novel_Symbols)
	word = TLM_decode_symbol (TAG_Models [tag_model].TAG_words_model,
				  TAG_Words_Context, coder);

    /* Decode and update the character contexts */
    if (PT_Novel_Symbols)
      {
	word = TXT_create_text ();
	for (;;)
	  { /* Each novel word is terminated with a sentinel symbol as some "words"
	       may have nulls in them */
	    symbol = TLM_decode_symbol (TAG_Models [tag_model].TAG_chars_model,
					TAG_Contexts [context].TAG_chars_context, coder);
	    if (symbol == TXT_sentinel_symbol ())
	        break;

	    TXT_append_symbol (word, symbol);
	  }

	if (Debug.level > 2)
	  {
	    fprintf (stderr, "Novel word: ");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "\n");
	  }
      }

    /* Now update the contexts using the decoded word if necessary */
    PT_update_decode_context (TAG_Models [tag_model].TAG_words_model, word);

    /* Set prev_word for next time */
    TAG_Contexts [context].TAG_prev_word = word;

    /* Return the tag and word as a single symbol:<word-text-no>*tags_alphabet_size + tag-id */
    TLM_get_model (TAG_Models [tag_model].TAG_tags_model, PPM_Get_Alphabet_Size,
		   &tags_alphabet_size);
    tag_word_symbol = word * tags_alphabet_size + tag_id;

    return (tag_word_symbol);
}

unsigned int
TAG_getcontext_position (unsigned int model, unsigned int context)
/* Returns an integer which uniquely identifies the current position
   associated with the TAG context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */
{
    unsigned int tag_model;
    unsigned int position;

    tag_model = TLM_verify_model (model, TLM_TAG_Model, TAG_valid_model);

    /* The following assumes a Viterbi-based search; for the stack
       algorithm, some paths will not have the same words and previous
       words; these should be factored into the context position here,
       but the easiest solution is to just use the tags to identify
       the position, which will result in some paths being dropped
       when they really should be kept. */
    position = TLM_getcontext_position
      (TAG_Models [tag_model].TAG_tags_model,
       TAG_Contexts [context].TAG_tags_context);
    /* We could multiply the tags context position by the chars
       context position, but for the Viterbi algorithm, the word
       will be the same anyway, so the tags will distinguish the
       context in any case (even if different char models are
       being used for each tag rather than a single one */

    /*
    fprintf (stderr, "Tags context pos = %d\n",
	     TLM_getcontext_position
	       (TAG_Models [tag_model].TAG_tags_model,
		TAG_Contexts [context].TAG_tags_context));
    fprintf (stderr, "Chars context pos = %d\n",
	     TLM_getcontext_position
	       (TAG_Models [tag_model].TAG_chars_model,
		TAG_Contexts [context].TAG_chars_context));
    */

    return (position);
}
