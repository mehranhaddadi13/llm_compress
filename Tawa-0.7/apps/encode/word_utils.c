/* Utility routines for encode_word/decode_word. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "word.h"
#include "word_utils.h"
#include "coder.h"
#include "model.h"

int Args_max_order [MODELS];               /* Maximum order of the models */
unsigned int Args_escape_method [MODELS];  /* Escape method for the models */
unsigned int Args_alphabet_size [MODELS];  /* The models' alphabet size, 0 means unbounded  */
unsigned int Args_performs_full_excls [MODELS]; /* Flag to indicate whether the models full perform exclusions or not */
unsigned int Args_performs_update_excls [MODELS]; /* Flag to indicate whether the models update perform exclusions or not */

char *Args_input_filename;                 /* The filename of the input text file being compressed */
char *Args_output_filename;                /* The filename of the encoded output file, the output from the compression */
char *Args_model_filename;                 /* The filename of an existing words model */
char *Args_title;                          /* The title of the words model */
unsigned int Args_model_file [MODELS];     /* The file pointers to the model files */
unsigned int Args_table_file [MODELS];     /* The file pointers to the table files */

void
word_usage ()
{
    fprintf (stderr,
	     "Usage: encode_word [options]\n"
	     "\n"
	     "options:\n"
	     "  -i fn\tname of uncompressed input file=fn (required argument)\n"
	     "  -o fn\tname of compressed output file=fn (required argument)\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -p n\treport progress every n chars\n"
	     "  -r\tdebug arithmetic coding ranges\n");
    fprintf (stderr,
	     "  -1 n\tmax order of non-words model=n\n"
	     "  -2 n\tmax order of words model=n\n"
	     "  -3 n\tmax order of new non-words model=n\n"
	     "  -4 n\tmax order of new words model=n\n"
	     "  -5 c\tescape method for non-words model=c\n"
	     "  -6 c\tescape method for words model=c\n"
	     "  -7 c\tescape method for new non-words model=c\n"
	     "  -8 c\tescape method for new words model=c\n");
}
    
void
init_word_arguments (int argc, char *argv[])
/* Initializes the arguments for each model from the arguments list. */
{
    extern char *optarg;
    extern int optind;
    int opt, type, escape;
    boolean input_found, output_found;

    /* Set up the defaults */
    input_found = FALSE;
    output_found = FALSE;
    Args_input_filename = NULL;
    Args_output_filename = NULL;
    Args_model_filename = NULL;
    Args_title = (char *) malloc (ARGS_TITLE_SIZE * sizeof (char));
    Args_max_order [TLM_Words_Nonword_Model] = DEFAULT_WORD_MAX_ORDER;
    Args_max_order [TLM_Words_Word_Model] = DEFAULT_WORD_MAX_ORDER;
    Args_max_order [TLM_Words_Nonchar_Model] = DEFAULT_CHAR_MAX_ORDER;
    Args_max_order [TLM_Words_Char_Model] = DEFAULT_CHAR_MAX_ORDER;

    Args_alphabet_size [TLM_Words_Nonword_Model] = DEFAULT_WORD_ALPHABET_SIZE;
    Args_alphabet_size [TLM_Words_Word_Model] = DEFAULT_WORD_ALPHABET_SIZE;
    Args_alphabet_size [TLM_Words_Nonchar_Model] = DEFAULT_CHAR_ALPHABET_SIZE;
    Args_alphabet_size [TLM_Words_Char_Model] = DEFAULT_CHAR_ALPHABET_SIZE;

    for (type=0; type<MODELS; type++)
      {
	Args_escape_method [type] = TLM_PPM_Method_D;
        Args_performs_full_excls [type] = TRUE;
        Args_performs_update_excls [type] = TRUE;
      }

    while ((opt = getopt (argc, argv, "d:i:mo:p:1:2:3:4:5:6:7:8:r")) != -1)
	switch (opt)
	{
	case 'd':
	    Debug.level = atoi (optarg);
	    break;
	case 'i':
	    Args_input_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_input_filename, optarg);
	    input_found = TRUE;
	    break;
	case 'm':
	    Args_model_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_model_filename, optarg);  
	    break;
	case 'o':
	    Args_output_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_output_filename, optarg);  
	    output_found = TRUE;
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = 1;
	    break;
	case '1':
	    Args_max_order [TLM_Words_Nonword_Model] = atoi (optarg);
	    break;
	case '2':
	    Args_max_order [TLM_Words_Word_Model] = atoi (optarg);
	    break;	
	case '3':
	    Args_max_order [TLM_Words_Nonchar_Model] = atoi (optarg);
	    break;
	case '4':
	    Args_max_order [TLM_Words_Char_Model] = atoi (optarg);
	    break;
	case '5':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method [TLM_Words_Nonword_Model] = escape;
	    break;
	case '6':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method [TLM_Words_Word_Model] = escape;
	    break;	
	case '7':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method [TLM_Words_Nonchar_Model] = escape;
	    break;
	case '8':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method [TLM_Words_Char_Model] = escape;
	    break;
	default:
	    word_usage ();
	    break;
	}

    if ((argc < optind) || !input_found || !output_found)
      {
	word_usage ();
	exit (1);
      }

    strcpy (Args_title, ARGS_TITLE);  
}
