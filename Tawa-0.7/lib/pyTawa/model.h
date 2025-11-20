/* TLM routines for language models. */

#ifndef MODEL_H
#define MODEL_H

#include "ptable.h"

#define FALSE 0                  /* For boolean expressions */
#define TRUE 1                   /* For boolean expressions */

typedef unsigned int boolean;

#define TLM_Version_No 1         /* Version number for identifying TLM version of model */

#define TLM_Static 0             /* Indicates model is static */
#define TLM_Dynamic 1            /* Indicates model is dynamic i.e. adaptive */

/* Defines the type of modelling algorithms */
#define TLM_PPM_Model 0          /* Indicates model type is PPM
				    ("Prediction by Partial Matching") */
#define TLM_PPMq_Model 1         /* Indicates model type is PPM, with "quick" update */ 
#define TLM_PPMo_Model 2         /* Indicates model type is PPMO (encodes order stream separately) */ 
#define TLM_PPMstar_Model 3      /* Indicates model type is PPM* 
				    (PPM star) */
#define TLM_SAMM_Model 4         /* Indicates model type is SAMM
				    ("Semi-adaptive Markov Model") */ 
#define TLM_PT_Model 5           /* Indicates model type is cumulative probability tables */
#define TLM_CPT_Model 6          /* Indicates model type is cumulative probability tables */
#define TLM_PCFG_Model 7         /* Indicates model type is probabilistic cfgs
				    (context free grammars) */
#define TLM_HMM_Model 8          /* Indicates model type is the same as used in the
				    Cambridge/CMU toolkit (e.g. Katz backoff with Good-Turing
				    Estimate) */
#define TLM_SSS_Model 9          /* Uses start-step-stop codes */
#define TLM_TAG_Model 10         /* Tag based model (i.e. parts of speech) */
#define TLM_USER_Model 11        /* User defined model */

/* Defines the type of modelling algorithms */
#define TLM_PPM_Method_A 0       /* Indicates PPM model uses escape method A */
#define TLM_PPM_Method_B 1       /* Indicates PPM model uses escape method B */
#define TLM_PPM_Method_C 2       /* Indicates PPM model uses escape method C */
#define TLM_PPM_Method_D 3       /* Indicates PPM model uses escape method D */

/* Operations for TLM_next_symbol, TLM_get_symbol, TLM_find_symbol and
   TLM_update_context routines.*/
#define TLM_Get_Nothing 0        /* Default operation; no additional
				    information is returned */
#define TLM_Get_Codelength 1     /* Returns the symbol's codelength i.e.
				    cost in bits of encoding the symbol */
#define TLM_Get_Coderanges 2     /* Returns the symbol's coderanges i.e.
				    arithmetic coding ranges */
#define TLM_Get_Maxorder 3       /* Returns codelength assuming max order
				    context only i.e. no escapes occur */

/* Operations for TLM_load_model set by TLM_set_load_operation. */
#define TLM_Load_No_Operation 0  /* Default operation; no additional
				    information is used */
#define TLM_Load_Change_Model_Type 1
                                 /* The following parameter is used to specify the new type
				    that the model is transformed into as it is loaded. */
#define TLM_Load_Change_Title 2  /* Specifies that the following parameter is used to change the
				    title of the model after it gets loaded. */

/* Operations for TLM_write_model set by TLM_set_write_operation. */
#define TLM_Write_No_Operation 0  /* Default operation; no additional
				     information is used */
#define TLM_Write_Change_Model_Type 1
                                 /* The following parameter is used to specify the new type
				    that the model is transformed into as it is written out. */

struct modelType
{ /* model record */
  char *M_title;                 /* The long description (title) of the model */
  char *M_tag;                   /* The short description (tag) of the model */
  unsigned int M_model;          /* Reference to model record that is specific
				    to the model_type */
  unsigned int M_model_type;     /* Model type e.g. PPM, PPM*, PCFG etc. */
  unsigned int M_model_form;     /* Indicates whether model is static or
				    dynamic */
  unsigned int M_contexts;       /* The number of active contexts associated
				    with this model */
  unsigned int M_version_no;     /* The version no. identifying the type of
				    model being used */
  unsigned int M_next;           /* Next record in the doubly linked Models
				    list */
  unsigned int M_prev;           /* Previous record in the doubly linked
				    Models list */
};

/* Global variables used for storing the models */
extern struct modelType *Models;           /* List of models */
extern unsigned int Models_max_size;       /* Current max. size of the models array */
extern unsigned int Models_count;          /* Current number of models (always <= max. size) */
extern unsigned int Models_used;           /* List of deleted model records */
extern unsigned int Models_unused;         /* Next unused model record */
extern unsigned int Models_head;           /* Head of the models list */
extern unsigned int Models_tail;           /* Tail of the models list */
extern unsigned int Models_ptr;            /* Current position in the models list */

struct coderType
{ /* Record of an arithmetic coder */
    unsigned int A_max_frequency;          /* The maximum frequency of the coder */
    unsigned int A_encoder_input_file;     /* Input file to be used during encoding;
					      i.e. can be Stdin_File */
    unsigned int A_encoder_output_file;    /* Encoded file to be used as output from encoder */
    unsigned int A_decoder_input_file;     /* Input file to be used as input to decoder */
    unsigned int A_decoder_output_file;    /* Decoded file to be used as output from decoder */

    void (*A_arithmetic_encode) (unsigned int, unsigned int, unsigned int, unsigned int);
                                           /* Pointer to arithmetic encoding routine */
    void (*A_arithmetic_decode) (unsigned int, unsigned int, unsigned int, unsigned int);
                                           /* Pointer to arithmetic decoding routine */
    unsigned int (*A_arithmetic_decode_target) (unsigned int, unsigned int);
                                           /* Pointer to arithmetic decode target routine */
    unsigned int A_next;                   /* Next in the linked list of coder records (used for deletions) */
};

/* Global variables used for storing the arithmetic coders */
extern struct coderType *Coders;           /* List of coder records */
extern unsigned int Coders_max_size;       /* Current max. size of the coders array */
extern unsigned int Coders_used;           /* List of deleted coder records */
extern unsigned int Coders_unused;         /* Next unused coder record */

/* The following defines the input and output coder files that
   are (possibly) used by the TLM_next_symbol, TLM_get_symbol, TLM_find_symbol
   and TLM_update_context routines. */
extern unsigned int TLM_Coder_Input_File;
extern unsigned int TLM_Coder_Output_File;
/* The following defines the default arithmetic coder to be used by the
   TLM_encode_symbol and TLM_decode_symbol routines. */
extern unsigned int TLM_Coder;

/* Global variables used for storing the latest codelength and coderanges
   for the routines TLM_next_symbol, TLM_get_symbol, TLM_find_symbol or
   TLM_update_context. */
extern unsigned int TLM_Context_Operation;
extern unsigned int TLM_Symbol;
extern unsigned int TLM_Coderanges;
extern float TLM_Codelength;

/* Global variables used by TLM_get_PPM_model. */
extern char *TLM_PPM_title; /* The title of the PPM model */
extern unsigned int TLM_PPM_model_form;       /* TLM_Dynamic or TLM_Static depending on
						 whether the model is static or dynamic. */
extern unsigned int TLM_PPM_alphabet_size;    /* The PPM model's alphabet size */
extern int TLM_PPM_max_order;                 /* The PPM model's max order, >= -1 */
extern char TLM_PPM_escape_method;            /* The escape method used by the PPM model */
extern boolean TLM_PPM_performs_full_excls;   /* The PPM model performs full exclusions */
extern boolean TLM_PPM_performs_update_excls; /* The PPM model performs update exclusions */

/* Define different types of coding:
      UPDATE_TYPE just updates the counts and moves everything along
        without doing any coding at all;
      ENCODE_TYPE performs arithmetic encoding;
      DECODE_TYPE performs arithmetic decoding;
      CODELENGTH_TYPE calculates the cost of encoding the symbol in
        bits without actually doing it;
      UPDATE1_TYPE does an update without updating the input (for
        inserting supplementary symbols into dynamic models);
      UPDATE2_TYPE does an update without updating the input or
        incrementing the symbol's count (another way of
        inserting supplementary symbols into dynamic models);
      UPDATE_MAXORDER_TYPE calculates the cost of encoding the
        symbol in the max order context only (it excludes any escape
        counts) and updates the counts at the same time;
      Plus combinations of more than one of the operations. */
typedef enum
{
  UPDATE_TYPE, ENCODE_TYPE, DECODE_TYPE,
  FIND_CODELENGTH_TYPE, UPDATE_CODELENGTH_TYPE,
  FIND_CODERANGES_TYPE, UPDATE_CODERANGES_TYPE,
  UPDATE1_TYPE, UPDATE1_CODELENGTH_TYPE, UPDATE2_TYPE,
  FIND_MAXORDER_TYPE, UPDATE_MAXORDER_TYPE
} codingType;

/* Define four different types of operations */
typedef enum
{
  NEXT_SYMBOL_TYPE, FIND_SYMBOL_TYPE, FIND_TARGET_TYPE
} operType;

#define NIL 0                   /* Indicates ptr is nil */
#define NO_CODER 0              /* Means do not use an arithmetic coder */
#define DEFAULT_CODER 1         /* Default arithmetic coder */

extern unsigned int debugModel;

float log_two (float x);

struct debugType
{ /* For debugging purposes */
    unsigned int level;         /* Used for indicating what levels of
				   debugging information needs to be spat out */
    unsigned int level1;        /* Also used for indicating what levels of
				   debugging information needs to be spat out */
    unsigned int progress;      /* Used for debugging progress at intermediate
				   stages */
    unsigned int range;         /* Used for debugging the model's arith.coding ranges */
    unsigned int coder;         /* Used for debugging the arithmetic coder ranges */
    unsigned int coder_target;  /* Used for debugging the arithmetic coder targets */
    unsigned int codelengths;   /* Used for debugging model codelengths */
};

extern struct debugType Debug;   /* For setting various debugging options */

boolean
TLM_valid_model (unsigned int model);
/* Returns non-zero if the model is valid, zero otherwize. */

unsigned int
TLM_verify_model (unsigned int model, unsigned int model_type,
		  boolean (*valid_model_function) (unsigned int));
/* Verifies the model_type and number for the model and returns the internal model number. */

unsigned int
TLM_valid_coder (unsigned int coder);
/* Returns a non-zero coder number if the coder is a valid coder,
   zero otherwize. */

unsigned int
TLM_create_coder
( unsigned int max_frequency,
  unsigned int encoder_input_file, unsigned int encoder_output_file,
  unsigned int decoder_input_file, unsigned int decoder_output_file,
  void (*arithmetic_encode) (unsigned int, unsigned int, unsigned int, unsigned int),
  void (*arithmetic_decode) (unsigned int, unsigned int, unsigned int, unsigned int),
  unsigned int (*arithmetic_decode_target) (unsigned int, unsigned int));
/* Creates and returns an unsigned integer which provides a reference to a coder
   record associated with an arithmetic coder. The argument max_frequency
   specifies the maximum frequency allowed for the coder. The arguments
   arithmetic_encode, arithmetic_decode and arithmetic_decode_target are
   pointers to the necessary routines required for encoding and decoding. Both
   arithmetic_encode and arithmetic_decode take three unsigned integers
   as arguments that specify the current arithmetic coding range (low, high
   and total); arithmetic_decode_target takes just a single unsigned integer
   as an argument, which is set to the total of the current coding range. */

unsigned int
TLM_create_arithmetic_coder (void);
/* Creates and returns the default arithmetic coder. */

unsigned int
TLM_create_arithmetic_encoder (unsigned int input_file, unsigned int output_file);
/* Creates and returns the arithmetic encoder. */

unsigned int
TLM_create_arithmetic_decoder (unsigned int input_file, unsigned int output_file);
/* Creates and returns the arithmetic decoder. */

void
TLM_release_coder (unsigned int coder);
/* Releases the memory allocated to the coder and the coder number (which may
   be reused in later TLM_create_coder calls). */

boolean
TLM_valid_context (unsigned int model, unsigned int context);
/* Returns non-zero if the model is valid, zero otherwize. */

unsigned int
TLM_create_context (unsigned int model);
/* Creates and returns an unsigned integer which provides a reference to a
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */

unsigned int
TLM_copy_context (unsigned int model, unsigned int context);
/* Creates a new context record, copies the contents of the specified context
   into it, and returns an integer reference to it. A run-time error occurs
   if the context being copied is for a dynamic model. */

unsigned int
TLM_clone_context (unsigned int model, unsigned int context);
/* Creates a new context record, copies the contents of the specified context
   into it, and returns an integer reference to it. A run-time error does not
   occur if the context being copied is for a dynamic model. */

void
TLM_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context);
/* Overlays the context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */

void
TLM_set_context_operation (unsigned int context_operation);
/* Sets the type of operation to be performed by the routines
   TLM_next_symbol, TLM_get_symbol, TLM_find_symbol and TLM_update_context.
   The argument operation_type is one of the following:
     TLM_Get_Codelength
	 Returned in the global variable TLM_Codelength is a float value 
	 which is set to the codelength for encoding the specified symbol
	 (i.e. the cost in bits of encoding it given the current context).
     TLM_Get_Coderanges
         Returned in the global variable TLM_Coderanges is an unsigned int
	 pointer to the list of arithmetic coding ranges required for
	 encoding the specified symbol given the current context.
     TLM_Get_Maxorder
	 Returned in the global variable TLM_Codelength is a float value 
	 which is set to the codelength for encoding the specified symbol
	 (i.e. the cost in bits of encoding it given the current context)
	 assuming only the maxorder symbols are being coded (i.e. that no
	 escapes occur). */

void
TLM_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol);
/* Finds the predicted symbol in the context. */

void
TLM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol);
/* Updates the context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. (For faster updates, set the option
   TLM_Get_No_Operation, so that the routine does not return
   any additional information such as the cost of encoding
   the symbol in bits (in TLM_Codelength).

   The ``sentinel symbol'' (whose value is obtained using the
   TXT_sentinel_symbol () routine) is used where there is a break
   required in the updating of the context, such as when the end of
   string has been reached or when more than one model is being used
   to encode different parts of a string. The effect of encoding the
   sentinel symbol is that the prior context is forced to the null
   string i.e. the subsequent context will contain just the sentinel
   symbol itself. This is useful during training if there are statistics
   that differ markedly at the start of some text than in the middle of
   it (for example, individual names, and titles within a long list).

   This routine is often used with the routines TLM_next_symbol,
   TLM_get_symbol, TLM_find_symbol. For example,
       TLM_update_context (context, TXT_sentinel_symbol (), ...)
   will update the context record so that the current symbol becomes the
   sentinel symbol. */

void
TLM_suspend_update (unsigned int model);
/* Suspends the update for a dynamic model temporarily (i.e. the
   model becomes a temporary static model and TLM_update_context
   will not update any of the internal statistics of the model.
   The update can be resumed using TLM_resume_update ().

   This is useful if it needs to be determined in advance which
   of two or more dynamic models a sequence of text should be
   added to (based on how much each requires to encode it, say). */

void
TLM_resume_update (unsigned int model);
/* Resumes the update for a model. See TLM_suspend_update (). */

void
TLM_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the context and the context number (which may
   be reused in later TLM_create_context or TLM_copy_context calls). */

unsigned int
TLM_create_coderanges (void);
/* Return a new pointer to a list of coderanges */

void
TLM_append_coderange (unsigned int coderanges, unsigned int lbnd,
		      unsigned int hbnd, unsigned int totl);
/* Append a new coderange record onto the tail of the coderange list */

void
TLM_reset_coderanges (unsigned int coderanges);
/* Resets the position in the list of coderanges associated with the current symbol.
   The next call to TLM_next_coderanges will return the first coderanges on the list. */

void
TLM_release_coderanges (unsigned int coderanges);
/* Release the coderange list to the used list */

float
TLM_codelength_coderanges (unsigned int coderanges);
/* Returns the code length of the current symbol's coderange in bits. It does this without
   altering the current symbol or the current coderange. */

void
TLM_dump_coderanges (unsigned int file, unsigned int coderanges);
/* Prints the coderange list for the current symbol in a human readable form.
   It does this without altering the current position in the coderange. list as determined
   by the functions TLM_reset_coderange or TLM_next_coderanges. */

unsigned int
TLM_copy_coderanges (unsigned int coderanges);
/* Creates a copy of the list of coderanges and returns a pointer to it. */

void
TLM_reset_symbol (unsigned int model, unsigned int context);
/* Resets the context record to point at the first predicted symbol of the
   current position. */

boolean
TLM_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol);
/* Returns the next predicted symbol in the context and the cost in bits of
   encoding it. The context record is not updated.

   The global variable TLM_Codelength is set to the symbol's codelength.

   If a sequence of calls to TLM_next_symbol are made, every symbol in the
   alphabet will be visited exactly once although the order in which they are
   visited is undefined being implementation and data dependent. The function
   returns FALSE when there are no more symbols to process. TLM_reset_symbol
   will reset the current position to point back at the first predicted symbol
   of the current context.

   The codelength value is the same as that returned by TLM_update_context
   which may use a faster search method to find the symbol's codelength
   more directly (rather than sequentially as TLM_next_symbol does). A call
   to TLM_update_context or other routines will have no affect on subsequent
   calls to TLM_next_symbol. */

boolean
TLM_get_symbol (unsigned int model, unsigned int context);
/* Returns the next predicted symbol in the context and the cost in bits of
   encoding it. The context record is not updated.

   The global variables TLM_Symbol and TLM_Codelength is set to the symbol's
   value and codelength respectively.

   If a sequence of calls to TLM_get_symbol are made, every symbol in the
   alphabet will be visited exactly once although the order in which they are
   visited is undefined being implementation and data dependent. The function
   returns FALSE when there are no more symbols to process. TLM_reset_symbol
   will reset the current position to point back at the first predicted symbol
   of the current context.

   The codelength value is the same as that returned by TLM_update_context
   which may use a faster search method to find the symbol's codelength
   more directly (rather than sequentially as TLM_get_symbol does). A call
   to TLM_update_context or other routines will have no affect on subsequent
   calls to TLM_get_symbol. */

void
TLM_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int symbol, unsigned int coder);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   context becomes the encoded symbol. */

unsigned int
TLM_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   context record so that the last symbol in the context becomes the
   decoded symbol. */

unsigned int
TLM_create_model (unsigned int model_type, char *title, ...);
/* Creates a new empty dynamic model. Returns the new model number allocated
   to it if the model was created successfully, NIL if not.

   The model_type argument specified the type of model to be created e.g.
   TLM_PPM_Model or TLM_PCFG_Model. It is followed by a variable number of
   parameters used to hold model information which differs between
   implementations of the different model types. For example, the
   TLM_PPM_Model implementation uses it to specify
   the maximum order of the PPM model, and whether the model should perform
   update exclusions.

   The title argument is intended to be a short human readable text description
   of the origins and content of the model.
*/

unsigned int
TLM_create_PPM_model (char *title, unsigned int alphabet_size, int max_order,
		      unsigned int escape_method, unsigned int performs_full_excls,
		      unsigned int performs_update_excls);
/* Creates a new empty dynamic PPM model. Returns the new model number allocated
   to it if the model was created successfully, NIL if not.

   The title argument is intended to be a short human readable text description
   of the origins and content of the model.
   alphabet_size is the size of the alphabet.
   max_order is the maximum order of the PPM model.
   escape_method is the escape method being used by the model. e.g. TLM_PPM_Method_C for PPMC and TLM_PPM_Method_D for PPMD.
   performs_full_excls is TRUE if the model is to perform full exclusions.
   performs_update_excls is TRUE if the model is to perform exclusions.
*/

boolean
TLM_get_model_type (unsigned int model, unsigned int *model_type,
		    unsigned int *model_form, char **title);
/* Returns information describing the model. Returns NIL if the model does not
   exist (and leaves the other parameters unmodified in this case), non-zero
   otherwise.  The arguments title and model_type are the
   values used to create the model in TLM_create_model().
   The argument model_form is set to TLM_Dynamic or TLM_Static depending on
   whether the model is static or dynamic. */

boolean
TLM_get_PPM_model_type (unsigned int model);
/* Returns information describing the PPM model such as the title for the model  
   and the model_form which is TLM_Dynamic or TLM_Static depending on whether
   the model is static or dynamic. The arguments title and model_type are the
   values used to create the model in TLM_create_PPM_model(). */

boolean
TLM_get_model (unsigned int model, ...);
/* Returns information describing the model. Returns NIL if the model does not
   exist (and leaves the other parameters unmodified in this case), non-zero
   otherwise. The argument model is followed by a variable number of parameters
   used to hold model information which differs between implementations of the
   different model types. */

void
TLM_set_model (unsigned int model, ...);
/* Sets information that describes the model. The argument model
   is followed by a variable number of parameters used to hold model
   information which differs between implementations of the different
   model types. For example, the implementation of a TLM_PPM_Model uses it to
   specify the new (extended) size of the alphabet and the maximum
   symbol number for which statistics will be updated (symbols that
   exceed this number will become "static" symbols). */

void
TLM_dump_model (unsigned int file, unsigned int model, void (*dump_symbol_function) (unsigned int, unsigned int));
/* Prints a human readable version of the model (intended mainly for debugging).
   The argument dump_symbol_function is a pointer to a function for printing symbols.
   If this is NULL, then each symbol will be printed as an unsigned int surrounded by
   angle brackets (e.g. <123>), unless it is human readable ASCII, in which case it will
   be printed as a char. */

void
TLM_dump_PPM_model (unsigned int file, unsigned int model);
/* Prints a human readable version of the PPM model (intended mainly for debugging). */

void
TLM_check_model (unsigned int file, unsigned int model,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Checks the model is consistent (for debugging purposes). */

void
TLM_set_load_operation (unsigned int load_operation, ...);
/* Sets the type of operation to be performed by the routine
   TLM_load_model. The argument is followed by a variable number of parameters
   used to hold various information depending on the load_operation parameter:
     TLM_Load_Change_Title
	 This specifies that the following parameter is used to change the
         title of the model after it gets loaded.
     TLM_Load_Change_Model_Type
         The following parameter is used to specify the new type that the model
         is transformed into as it is loaded. */

unsigned int
TLM_load_model (unsigned int file);
/* Loads a model which has been previously saved to the file into memory and
   allocates it a new model number which is returned. */

void
TLM_load_models (char *filename);
/* Load the models and their associated tags from the specified file. */

unsigned int
TLM_read_model (char *filename, char *debug_line, char *error_line);
/* Reads in the model directly by loading the model from the file
   with the specified filename into a new model and returns it. */

void
TLM_set_write_operation (unsigned int write_operation, ...);
/* Sets the type of operation to be performed by the routine
   TLM_write_model. The argument is followed by a variable number of parameters
   used to hold various information depending on the write_operation parameter:
     TLM_Write_Change_Model_Type
         The following parameter is used to specify the new type that the model
         is transformed into as it is written out. */

void
TLM_write_model (unsigned int file, unsigned int model, unsigned int model_form);
/* Writes out the model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */

void
TLM_release_model (unsigned int model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later TLM_create_model or TLM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active contexts pointing at it. */

unsigned int
TLM_copy_model (unsigned int model);
/* Copies the model. */

void
TLM_nullify_model (unsigned int model);
/* Replaces the model with the null model and releases the memory allocated
   to it. */

unsigned int
TLM_minlength_model (unsigned int model);
/* Returns the minimum number of bits needed to write the model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages. */

unsigned int
TLM_numberof_models (void);
/* Returns the number of currently valid models. */

void
TLM_reset_modelno (void);
/* Resets the current model number so that the next call to TLM_next_modelno will
   return the first valid model number (or NIL if there are none). */

unsigned int
TLM_next_modelno (void);
/* Returns the model number of the next valid model. Returns NIL if
   there isn't any. */

unsigned int
TLM_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the model's context. (One implementation is to return
   a memory location corresponding to the current position. This routine is
   useful if you need to check whether different contexts have encoded
   the same prior symbols as when checking whether the context pathways
   converge in the Viterbi or trellis-based algorithms.) */

char *
TLM_get_title (unsigned int model);
/* Returns the title associated with the model. */

void
TLM_set_tag (unsigned int model, char *tag);
/* Sets the tag associated with the model. */

char *
TLM_get_tag (unsigned int model);
/* Return the tag associated with the model. */

unsigned int
TLM_getmodel_tag (char *tag);
/* Returns the model associated with the model's tag. If the tag
   occurs more than once, it will return the lowest model number. */

void
TLM_extend_alphabet_size (unsigned int model, unsigned int alphabet_size);
/* Extends the alphabet size associated with the model. */

unsigned int
TLM_sizeof_model (unsigned int model);
/* Returns the current number of bits needed to store the
   model in memory. */

void
TLM_dump_models (unsigned int file, void (*dump_symbol_function) (unsigned int, unsigned int));
/* Writes a human readable version of all the currently valid models to the
   file. The argument dump_symbol_function is a pointer to a function for printing symbols.
   If this is NULL, then each symbol will be printed as an unsigned int surrounded by
   angle brackets (e.g. <123>), unless it is human readable ASCII, in which case it will
   be printed as a char. */

void
TLM_dump_models1 (unsigned int file);
/* Writes a human readable version of all the currently valid models to the
   file. */

void
TLM_release_models (void);
/* Releases the memory used by all the models. */

void
TLM_stats_model (unsigned int file, unsigned int model);
/* Writes out statistics about the model in human readable form. */

#endif
