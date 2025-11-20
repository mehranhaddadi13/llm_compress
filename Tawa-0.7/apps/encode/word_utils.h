/* Utility routines for both word_encode/word_decode */

#ifndef WORD_UTILS_H
#define WORD_UTILS_H

#define MODELS 4        /* There are four types of word models/contexts/etc. */

#define ARGS_MAX_ORDER 5         /* Default maximum order of the model */
#define ARGS_EXCLUSIONS TRUE     /* Perform exclusions by default */
#define ARGS_TITLE_SIZE 32       /* Size of the model's title string */
/* Title of model - this could be anything you choose: */
#define ARGS_TITLE "Sample PPMD model"

#define DEFAULT_WORD_MAX_ORDER 1 /* Default max.order for word-based model */
#define DEFAULT_CHAR_MAX_ORDER 4 /* Default max.order for char-based model */
#define DEFAULT_WORD_ALPHABET_SIZE 0 /* Default alphabet sizefor word-based model */
#define DEFAULT_CHAR_ALPHABET_SIZE 256 /* Default alphabet size for char-based model */

#define FILENAME_SIZE 256        /* Maximum size for a filename string */

extern int Args_max_order [MODELS];               /* Maximum order of the models */
extern unsigned int Args_escape_method [MODELS];  /* Escape method for the models */
extern unsigned int Args_alphabet_size [MODELS];  /* The models' alphabet size, 0 means unbounded  */
extern unsigned int Args_performs_full_excls [MODELS]; /* Flag to indicate whether the models perform full exclusions or not */
extern unsigned int Args_performs_update_excls [MODELS]; /* Flag to indicate whether the models perform update exclusions or not */

extern char *Args_input_filename;                 /* The filename of the input text file being compressed */
extern char *Args_output_filename;                /* The filename of the encoded output file, the output from the compression */
extern char *Args_model_filename;                 /* The filename of an existing model */
extern char *Args_title;                          /* The title of the words model */
extern unsigned int Args_model_file [MODELS];     /* The file pointers to the model files */
extern unsigned int Args_table_file [MODELS];     /* The file pointers to the table files */

void init_word_arguments (int argc, char *argv[]);
/* Initializes the arguments for each model from the arguments list. */

void
open_word_files (char *file_prefix);
/* Opens all the word files for reading. */

#endif
