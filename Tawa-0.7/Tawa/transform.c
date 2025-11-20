/* Text transform routines. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "model.h"
#include "ppm_model.h"
#include "ppm_context.h"
#include "io.h"
#include "text.h"
#include "paths.h"
#include "confusion.h"
#include "transform.h"

#define TRANSFORMS_SIZE 4             /* Initial size of Transforms array */

struct transformType *Transforms = NULL; /* List of transform records */
unsigned int Transforms_max_size = 0; /* Current max. size of the texts array */
unsigned int Transforms_used = NIL;   /* List of deleted text records */
unsigned int Transforms_unused = 1;   /* Next unused text record */

/* Global functions used for debugging the transform process. */
void (*Transform_dump_symbol_function) (unsigned int, unsigned int) = NULL;
void (*Transform_dump_symbols_function) (unsigned int, unsigned int) = NULL;
void (*Transform_dump_confusion_function) (unsigned int, unsigned int) = NULL;

boolean
TTM_valid_transform (unsigned int transform)
/* Returns non-zero if the transform is valid, zero otherwize. */
{
    if (transform == NIL)
        return (FALSE);
    else if (transform >= Transforms_unused)
        return (FALSE);
    else if (transform != Transforms [transform].Transform_next)
        return (FALSE);
        /* The transform next pointer gets set to the next in the
	   used list the model gets deleted; this way you can test to see if
	   the transform record has been deleted or not */
    else
        return (TRUE);
}

unsigned int
TTM_create_transform (unsigned int transform_algorithm, ...)
/* Creates and returns an unsigned integer which provides a reference to a
   record associated with a transform model used to correct or "mark up" text.
   The argument transform_algorithm specifies the type of transform algorithm to
   use such as TTM_Viterbi, and is followed by a varying number of additional
   arguments as determined by the transform_algorithm. */
{
    va_list args; /* points to each unnamed argument in turn */
    struct transformType *transform;
    unsigned int m, old_size;

    va_start (args, transform_algorithm);

    if (Transforms_used != NIL)
    {	/* use the first list of transforms on the used list */
	m = Transforms_used;
	Transforms_used = Transforms [m].Transform_next;
    }
    else
    {
	m = Transforms_unused;
	if (Transforms_unused+1 >= Transforms_max_size)
	{ /* need to extend Transforms array */
	    old_size = Transforms_max_size * sizeof (struct transformType);
	    if (Transforms_max_size == 0)
		Transforms_max_size = TRANSFORMS_SIZE;
	    else
		Transforms_max_size *= 2; /* Keep on doubling the array on demand */
	    Transforms = (struct transformType *) Realloc (61, Transforms,
		      Transforms_max_size * sizeof (struct transformType), old_size);

	    if (Transforms == NULL)
	    {
		fprintf (stderr, "Fatal error: out of transforms space\n");
		exit (1);
	    }
	}
	Transforms_unused++;
    }

    if (m != NIL)
    {
      transform = Transforms + m;

      transform->Transform = createConfusion ();
      transform->Transform_next = m; /* this signifies that this transform record is valid */ 
      transform->Transform_algorithm = transform_algorithm;
      transform->Transform_paths = NULL;
      transform->Transform_leaves = NULL;
      transform->Transform_text = NIL;

      switch (transform_algorithm)
	{
	case TTM_Stack:
	  transform->Transform_stack_type = va_arg (args, unsigned int);
	  transform->Transform_stack_depth = va_arg (args, unsigned int);
	  transform->Transform_stack_extension = va_arg (args, unsigned int);
	  break;
	case TTM_Viterbi:
	  break;
	default:
	  fprintf (stderr, "Fatal error: invalid transform algorithm %d\n", transform_algorithm);
	  exit (1);
	  break;
	}
    }

    va_end (args);

    startPaths (m);

    return (m);
}

void
TTM_release_transform (unsigned int transform_model)
/* Releases the memory allocated to the transform record and the transform number
   (which may be reused in later TTM_create_transform calls). */
{
    struct transformType *transform;

    assert (TTM_valid_transform (transform_model));
    transform = Transforms + transform_model;

    TXT_release_text (transform->Transform_text);
    releasePaths (transform_model);

    releaseConfusions (transform->Transform);

    /* Append onto head of the used list */
    transform->Transform_next = Transforms_used;
    Transforms_used = transform_model;
}

void
TTM_start_transform (unsigned int transform_model, unsigned int transform_type,
		  unsigned int source_text, unsigned int language_model)
/* Starts a new process for correcting or "marking up" the source text using the specified
   language model. This routine must be called at least once by TTM_perform_transform. Multiple
   calls to this procedure will cause separate transform paths to be initiated for each of the
   specified language models. */
{
    struct transformType *transform;

    assert (TTM_valid_transform (transform_model));
    assert (TLM_valid_model (language_model));
    transform = Transforms + transform_model;

    if (transform->Transform_text == NIL)
        transform->Transform_text = TXT_create_text ();
    else if (TXT_valid_text (transform->Transform_text) &&
	     (TXT_length_text (transform->Transform_text) != 0))
      { /* there are an old marked up text and paths trie hanging around - get rid of them
	   before we start again */
	TXT_setlength_text (transform->Transform_text, 0);
	releasePaths (transform_model);
      }

    if (Debug.level1 > 0)
      {
	if (transform_type == TTM_transform_multi_context)
	  fprintf (stderr, "Multi context transform for model %d (%s)\n",
		   language_model, TLM_get_tag (language_model));
	else
	  fprintf (stderr, "Single context transform for model %d (%s)\n",
		   language_model, TLM_get_tag (language_model));
      }

    startPath (transform_model, transform_type, language_model);
}

void
TTM_add_transform (unsigned int transform_model, float codelength,
		char *source_text_format,
		char *transform_text_format, ...)
/* Adds the formatted correction to the transform model. The argument source_text_format is the format of the original text
   begin corrected; transform_text_format is the format of the text it will be corrected to; and codelength is the
   cost in bits of making that correction when the text is being marked up. A variable length list of arguments
   as specified by the formats follows.

   Meaning of source text formatting characters:

         %%    - this will match the % (percentage) character.
         %m    - this will match when the predicting model has the same model
                 number as the corresponding one specified in the argument list.
         %s    - this will match the corresponding symbol in the argument list.
	 %b    - the argument list contains a pointer to a boolean function that
	         takes an unsigned int symbol number as its only argument (which
                 will be set to the current context symbol number when the function
                 is called by the transform process) and returns a non-zero value if
                 the current context symbol number is a valid match.

		 format: [boolean (*function) (unsigned int symbol)]  
                 example: is_punct (symbol)

	 %f    - the argument list contains a pointer to a boolean function that
	         takes an unsigned int model number, an unsigned int symbol number
		 and an unsigned int previous symbol number as the its first 3 arguments
		 (which will be set to the current context's model, symbol and previous
		 symbol numbers when the function is called by the transform process) and
		 returns a non-zero value if the current context symbol number is a valid
		 match.

		 format:
		   [boolean (*function)
		     (unsigned int model, unsigned int source_symbol,
		      unsigned int source_text, unsigned int source_pos)
		      unsigned int previous_symbol)]

         %w    - wildcard symbol: this will match the current symbol in the context.
         %[..] - range symbols: this will match any symbol specified between the
                 square brackets.

                 example: %[aeiou] matches vowels.


   Meaning of transform text formatting characters:

         %%    - the % (percentage) character is inserted into the transform text.
         %m    - the argument list contains a model number which will be used
	         to predict subsequent symbols in the transform text.
         %s    - the argument list contains a symbol number (unsigned int) which will be
	         inserted into the transform text.
         %b    - this will insert the matching function symbol from the context into the
	         transform text.
	 %f    - the argument list contains a pointer to a function that
	         takes an unsigned int model number, an unsigned int symbol number
		 and an unsigned int previous symbol number as the its first 3 arguments
		 (which will be set to the current context's model, symbol and previous
		 symbol numbers when the function is called by the transform process) and
		 an unsigned int reference to a text record as its fourth argument and
		 returns in the text record a list of symbol numbers which will be each
		 used separately to replace the given symbol number in the marked up text.

		 format: [void (*function)
		   (unsigned int model, unsigned int source_symbol,
		    unsigned int source_text, unsigned int source_pos,
		    unsigned int previous_symbol, unsigned int range_text)]  

         %w    - wildcard symbol: this will insert the matching symbol from the context
                 into the transform text.
         %r    - this will insert the matching range symbol from the context into the
	         transform text.
         %[..] - range symbols: this will generate transform texts for all the symbols
                 specified between the square brackets.

                 example: %[aeiou] generates five distinct transform texts for each
	 %$    - the sentinel symbol is inserted into the transform text.
         %_    - insert the next symbol into the marked up text but do not
	         encode it (or update the context)
         %.    - insert the next symbol into the marked up text, update the
                 context (but do not encode it)

   Examples:

       TTM_add_transform (transform_model, codelength, "1", "a");
           This generates a single transform that replaces the character 1 with the letter a.

       TTM_add_transform (transform_model, codelength, "%w", "%w ");
           This generates a single transform that inserts an extra space after each symbol.

       TTM_add_transform (transform_model, codelength, "%f", "%f ", TTM_is_punct);
           This generates a single transform that inserts an extra space after each punctuation symbol.

       TTM_add_transform (transform_model, codelength, "%[xy]", "%r%[abc]");
           This generates the following transform corrections:
	   e.g.    "x" generates "xa", "xb" and "xc"
                   "y" generates "ya", "yb" and "yc"

 */
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int source_text, transform_text, source_type, transform_type;
    unsigned int symbol, model, range, p;
    boolean percent_found;

    assert (TTM_valid_transform (transform_model));

    /* create the source and transform texts - these consists of a sequence of
       pairs of numbers - the first in the pair is number that indicates the "type"
       (e.g. symbol number, model number, function, wildcard, range) of the next in the pair */
    source_text = TXT_create_text ();
    source_type = TXT_create_text ();
    transform_text = TXT_create_text ();
    transform_type = TXT_create_text ();

    va_start (args, transform_text_format);

    /* process the source text format */
    percent_found = FALSE;
    for (p = 0; p < strlen (source_text_format); p++)
      {
	symbol = (unsigned char) source_text_format [p];
	if (!percent_found)
	  {
	    if (symbol == '%')
	        percent_found = TRUE;
	    else
	      {
		TXT_append_symbol (source_text, symbol);
		TXT_append_symbol (source_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	      }
	    continue;
	  }
	switch (symbol)
	  {
	  case '%':
	    TXT_append_symbol (source_text, '%'); /* it's a percentage character */
	    TXT_append_symbol (source_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	    break;
	  case 's':
	    symbol = va_arg (args, unsigned int);
	    TXT_append_symbol (source_text, symbol);
	    TXT_append_symbol (source_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	    break;
	  case 'm':
	    model = va_arg (args, unsigned int);
	    TXT_append_symbol (source_text, model);
	    TXT_append_symbol (source_type, TRANSFORM_MODEL_TYPE); /* we have a model number */
	    break;
	  case 'b':
	    symbol = va_arg (args, unsigned int); /* store function pointer as an unsigned int to be recast later */
	    TXT_append_symbol (source_text, symbol);
	    TXT_append_symbol (source_type, TRANSFORM_BOOLEAN_TYPE);
	    break;
	  case 'f':
	    symbol = va_arg (args, unsigned int); /* store function pointer as an unsigned int to be recast later */
	    TXT_append_symbol (source_text, symbol);
	    TXT_append_symbol (source_type, TRANSFORM_FUNCTION_TYPE);
	    break;
	  case 'w':
	    TXT_append_symbol (source_text, 0);   /* the 0 is ignored */
	    TXT_append_symbol (source_type, TRANSFORM_WILDCARD_TYPE);
	    break;
	  case '[':
	    range = TXT_create_text ();
	    while ((symbol = source_text_format [++p]) != ']')
		TXT_append_symbol (range, symbol);
	    TXT_append_symbol (source_text, range);
	    TXT_append_symbol (source_type, TRANSFORM_RANGE_TYPE);
	    break;
	  default:
	    TXT_append_symbol (source_text, symbol);
	    TXT_append_symbol (source_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	    break;
	  }
	percent_found = FALSE;
      }

    /* process the transform text format */
    percent_found = FALSE;
    for (p = 0; p < strlen (transform_text_format); p++)
      {
	symbol = (unsigned char) transform_text_format [p];
	if (!percent_found)
	  {
	    if (symbol == '%')
	        percent_found = TRUE;
	    else
	      {
		TXT_append_symbol (transform_text, symbol);
		TXT_append_symbol (transform_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	      }
	    continue;
	  }
	switch (symbol)
	  {
	  case '%':
	    TXT_append_symbol (transform_text, '%'); /* it's the percentage character */
	    TXT_append_symbol (transform_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	    break;
	  case 's':
	    symbol = va_arg (args, unsigned int);
	    TXT_append_symbol (transform_text, symbol);
	    TXT_append_symbol (transform_type, TRANSFORM_SYMBOL_TYPE); /* we have a symbol number */
	    break;
	  case 'm':
	    symbol = va_arg (args, unsigned int);
	    TXT_append_symbol (transform_text, symbol);
	    TXT_append_symbol (transform_type, TRANSFORM_MODEL_TYPE); /* we have a model number */
	    break;
	  case 'b':
	    TXT_append_symbol (transform_text, 0);   /* the 0 is ignored */
	    TXT_append_symbol (transform_type, TRANSFORM_BOOLEAN_TYPE);
	    break;
	  case 'f':
	    symbol = va_arg (args, unsigned int); /* store function pointer as an unsigned int to be recast later */
	    TXT_append_symbol (transform_text, symbol);
	    TXT_append_symbol (transform_type, TRANSFORM_FUNCTION_TYPE);
	    break;
	  case 'r':
	    TXT_append_symbol (transform_text, 0);   /* the 0 is ignored */
	    TXT_append_symbol (transform_type, TRANSFORM_RANGE_TYPE);
	    break;
	  case 'w':
	    TXT_append_symbol (transform_text, 0);   /* the 0 is ignored */
	    TXT_append_symbol (transform_type, TRANSFORM_WILDCARD_TYPE);
	    break;
	  case '[':
	    range = TXT_create_text ();
	    while ((symbol = transform_text_format [++p]) != ']')
		TXT_append_symbol (range, symbol);
	    TXT_append_symbol (transform_text, (unsigned int) range);
	    TXT_append_symbol (transform_type, TRANSFORM_RANGE_TYPE);
	    break;
	  case '$':
	    TXT_append_symbol (transform_text, TXT_sentinel_symbol ());
	    TXT_append_symbol (transform_type, TRANSFORM_SENTINEL_TYPE); /* we have a model number */
	    break;
	  case '_':
	    TXT_append_symbol (transform_text, 0); /* ignored */
	    TXT_append_symbol (transform_type, TRANSFORM_GHOST_TYPE); /* "ghost" encoding of next symbol */
	    break;
	  case '.':
	    TXT_append_symbol (transform_text, 0); /* ignored */
	    TXT_append_symbol (transform_type, TRANSFORM_SUSPEND_TYPE); /* suspend encoding of next symbol */
	    break;
	  default:
	    TXT_append_symbol (transform_text, symbol);
	    TXT_append_symbol (transform_type, TRANSFORM_MODEL_TYPE); /* we have a model number */
	    break;
	  }
	percent_found = FALSE;
      }

    va_end (args);

    addConfusion (Transforms [transform_model].Transform, codelength, source_text, source_type, transform_text, transform_type);

    TXT_release_text (source_text);
    TXT_release_text (source_type);
    TXT_release_text (transform_text);
    TXT_release_text (transform_type);
}

void
TTM_debug_transform
(void (*dump_symbol_function) (unsigned int, unsigned int),
 void (*dump_symbols_function) (unsigned int, unsigned int),
 void (*dump_confusion_function) (unsigned int, unsigned int))
/* Sets the function for dumping out intermediate output generated
   by TTM_perform_transform (intended mainly for debugging). */
{
    if (Debug.level1 == 0)
        Debug.level1 = 1;
    Transform_dump_symbol_function = dump_symbol_function;
    Transform_dump_symbols_function = dump_symbols_function;
    Transform_dump_confusion_function = dump_confusion_function;
}

void
TTM_load_transform (unsigned int file, unsigned int transform_model)
/* Adds the confusions found in file to the transform model. */
{
    unsigned int text;

    assert (TTM_valid_transform (transform_model));

    text = TXT_load_file (file);
    loadConfusions (text, transform_model);
    TXT_release_text (text);
}

void
TTM_dump_transform (unsigned int file, unsigned int transform_model)
/* Prints a human readable version of the transform model (intended mainly for debugging). */
{
    assert (TTM_valid_transform (transform_model));

    fprintf (Files [file], "\nDump of transformations model:\n");
    dumpConfusions (file, Transforms [transform_model].Transform);
    fprintf (Files [file], "\n");
}

unsigned int
TTM_perform_transform (unsigned int transform_model, unsigned int source_text)
/* Creates and returns an unsigned integer which provides a reference to a text record
   that contains the source text corrected according to the transform and language models. */
{
    struct transformType *transform;

    assert (TTM_valid_transform (transform_model));
    assert (TXT_valid_text (source_text));

    transform = Transforms + transform_model;

    if (transform->Transform == NULL)
        return (source_text);
    else
        return
	  (transformPaths (transform_model, source_text,
			Transform_dump_symbol_function,
			Transform_dump_symbols_function,
			Transform_dump_confusion_function));
}
