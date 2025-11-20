/* PPM extended model module definitions. */

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "model.h"

#define TTM_Viterbi 0
/* Indicates transform algorithm type is Viterbi; no additional arguments */
#define TTM_Stack 1
/* Indicates transform algorithm type is the Stack decoding algorithm; it is
   followed by two additonal arguments: stack_type, whcih specifies the type
   of stack decoding algorithm to be used; and stack_depth, which specifies
   the maximum depth of the stack when the worst paths have been deleted. */

/* The following specifies different types of stack decoding algorithms */
#define TTM_Stack_type0   0  /* Viterbi pruning */
#define TTM_Stack_type1   1  /* No Viterbi pruning */

/* The following specifies different types of transform methods */
#define TTM_transform_multi_context 0     /* Uses multiple contexts (default) */
#define TTM_transform_single_context 1    /* Single context only */

#define TRANSFORM_SYMBOL_TYPE 0   /* Specifies transform type is a symbol number */
#define TRANSFORM_MODEL_TYPE 1    /* Specifies transform type is a model number */
#define TRANSFORM_BOOLEAN_TYPE 2  /* Specifies transform type is a boolean function*/
#define TRANSFORM_WILDCARD_TYPE 3 /* Specifies transform type is a wildcard */
#define TRANSFORM_RANGE_TYPE 4    /* Specifies transform type is a range */
#define TRANSFORM_FUNCTION_TYPE 5 /* Specifies transform type is a function */
#define TRANSFORM_SENTINEL_TYPE 6 /* Specifies transform type is the sentinel sym. */
#define TRANSFORM_GHOST_TYPE 7    /* Specifies transform type is a ghost symbol */
#define TRANSFORM_SUSPEND_TYPE 8  /* Specifies transform type is a suspended symbol*/

struct modelTransformType
{ /* model transform record */
  unsigned int M_transform_type;    /* The type of transform that is being used
				    for this model */
  unsigned int M_transform_context; /* Current transform context */
  unsigned int M_transform_codelength;
                                 /* Codelength for current transform symbol */
  unsigned int M_transform_sentinel_codelength;
                                 /* Codelength for encoding a sentinel symbol
				    at the current position */
};

struct transformType
{ /* Defines a "transform" model for correcting text. */
    struct confusionTrieType *Transform;   /* The transform model's trie list of confusions */
    struct pathType *Transform_paths;      /* The transform paths trie */
    struct leafType *Transform_leaves;     /* All the leaves in the transform paths
					   trie */
    unsigned int Transform_algorithm;      /* The transform model's algorithm */
    unsigned int Transform_stack_type;     /* The type of stack decoding algorithm
					   to use */
    unsigned int Transform_stack_depth;
    /* The depth of the stack if algorithm = TTM_Stack; 0 means any stack depth
       is permissible */
    unsigned int Transform_stack_extension;
    /* The max. path extension before old paths get discarded;
       if algorithm = TTM_Stack; 0 means any extension length
       is permissible */
    unsigned int Transform_text;           /* Best marked up output text; gets
					   continually re-used */
    unsigned int Transform_next;           /* Used for deletions - the next in
					   the used list */
};

extern struct transformType *Transforms; /* List of transform records */

/* Global functions used for debugging the transform process. */
extern void (*Transform_dump_symbol_function) (unsigned int, unsigned int);
extern void (*Transform_dump_symbols_function) (unsigned int, unsigned int);
extern void (*Transform_dump_confusion_function) (unsigned int, unsigned int);

boolean
TTM_valid_transform (unsigned int transform);
/* Returns non-zero if the transform is valid, zero otherwize. */

unsigned int
TTM_create_transform (unsigned int transform_algorithm, ...);
/* Creates and returns an unsigned integer which provides a reference to a record associated
   with a transform model used to correct or "mark up" text. The argument transform_algorithm
   specifies the type of transform algorithm to use such as TTM_Viterbi, and is followed by a
   varying number of additional arguments as determined by the transform_algorithm. */

void
TTM_release_transform (unsigned int transform_model);
/* Releases the memory allocated to the transform record and the transform number
   (which may be reused in later TTM_create_transform calls). */

void
TTM_start_transform (unsigned int transform_model, unsigned int transform_type,
		  unsigned int source_text, unsigned int language_model);
/* Starts a new process for correcting or "marking up" the source text using the specified
   language model. This routine must be called at least once by TTM_perform_transform. Multiple
   calls to this procedure will cause separate transform paths to be initiated for each of the
   specified language models. */

void
TTM_add_transform (unsigned int transform_model, float codelength,
		char *source_text_format,
		char *transform_text_format, ...);
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

void
TTM_load_transform (unsigned int file, unsigned int transform_model);
/* Adds the confusions in file to the transform model. */

void
TTM_dump_transform (unsigned int file, unsigned int transform_model);
/* Prints a human readable version of the transform model (intended mainly for debugging). */

void
TTM_debug_transform
(void (*dump_symbol_function) (unsigned int, unsigned int),
 void (*dump_symbols_function) (unsigned int, unsigned int),
 void (*dump_confusion_function) (unsigned int, unsigned int));
/* Sets the functions for dumping out intermediate output generated
   by TTM_perform_transform (intended mainly for debugging). */

unsigned int
TTM_perform_transform (unsigned int transform_model, unsigned int source_text);
/* Creates and returns an unsigned integer which provides a reference to a text record
   that contains the source text corrected according to the transform and language models. */

#endif
