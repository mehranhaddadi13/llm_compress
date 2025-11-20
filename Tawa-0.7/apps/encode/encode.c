/* Dynamic PPMD encoder (based on characters) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "coder.h"
#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_OPTIONS 32
#define ARGS_MAX_ORDER 5       /* Default maximum order of the model */
#define ARGS_ALPHABET_SIZE 256 /* Default size of the model's alphabet */
#define ARGS_TITLE_SIZE 32     /* Size of the model's title string */
#define ARGS_TITLE "Sample PPMD model" /* Title of model - this could be anything you choose */

int Args_max_order;               /* Maximum order of the models */
unsigned int Args_escape_method;  /* Escape method for the model */
unsigned int Args_alphabet_size;  /* The models' alphabet size */
unsigned int Args_performs_full_excls; /* Flag to indicate whether the models perform full exclusions or not */
unsigned int Args_performs_update_excls; /* Flag to indicate whether the models perform update exclusions or not */

char *Args_input_filename;        /* The filename of the input text to be
				     compressed */
char *Args_output_filename;       /* The filename of the compressed output
				     text */
char *Args_model_filename;        /* The filename of an existing model */
char *Args_title;                 /* The title of the primary model */

boolean Encode_Numbers = FALSE;   /* TRUE if input file is list of numbers */
boolean Dump_Model = FALSE;
boolean Input_file_found = FALSE; /* TRUE if the input file has been specified using -i */
boolean Output_file_found = FALSE;/* TRUE if the output file has been specified using -o */

void
usage ()
{
    fprintf (stderr,
	     "Usage: encode1 [options] -i input-text -o output-text\n"
	     "\n"
	     "options:\n"
	     "  -F\tdoes not perform full exclusions (default = TRUE)\n"
	     "  -U\tdoes not perform update exclusions (default = TRUE)\n"
	     "  -N\tinput text is a sequence of unsigned numbers\n");
    fprintf (stderr,
	     "  -a n\tsize of alphabet=n\n"
	     "  -d\tdump model after every update\n"
	     "  -e c\tescape method for the model=c\n"
	     "  -i fn\tname of uncompressed input file=fn\n"
	     "  -o fn\tname of compressed output file=fn\n"
	     "  -O n\tmax order of model=n\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -p n\treport progress every n chars\n"
	     "  -r\tdebug arithmetic coding ranges\n");
}

void
init_arguments (int argc, char *argv[])
/* Initializes the arguments from the arguments list. */
{
    extern char *optarg;
    extern int optind;
    int opt, escape;

    Args_alphabet_size = ARGS_ALPHABET_SIZE;
    Args_max_order = ARGS_MAX_ORDER;
    Args_escape_method = TLM_PPM_Method_D;
    Args_performs_full_excls = TRUE;
    Args_performs_update_excls = TRUE;
    Args_title = (char *) malloc (ARGS_TITLE_SIZE * sizeof (char));

    while ((opt = getopt (argc, argv, "FUNa:de:m:i:o:O:p:r")) != -1)
	switch (opt)
	{
	case 'F':
	    Args_performs_full_excls = FALSE;
	    break;
	case 'U':
	    Args_performs_update_excls = FALSE;
	    break;
	case 'N':
	    Encode_Numbers = TRUE;
	    break;
        case 'a':
	    Args_alphabet_size = atoi (optarg);
	    break;
	case 'd':
	    Dump_Model = TRUE;
	    break;
	case 'e' :
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method = escape;
	    break;
	case 'i':
	    Input_file_found = TRUE;
	    Args_input_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_input_filename, optarg);  
	    break;
	case 'o':
	    Output_file_found = TRUE;
	    Args_output_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_output_filename, optarg);  
	    break;
	case 'O':
	    Args_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'm':
	    Args_model_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_model_filename, optarg);  
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (argc < optind)
      {
	usage ();
	exit (1);
      }

    strcpy (Args_title, ARGS_TITLE);  
}

int
getSymbol (unsigned int file, unsigned int *symbol)
/* Returns the next symbol from input stream FILE. */
{
    unsigned int sym;
    int result;

    sym = 0;
    if (Encode_Numbers)
      {
        result = fscanf (Files [file], "%u", &sym);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	  }
      }
    else
      {
        sym = getc (Files [file]);
	result = sym;
      }
    *symbol = sym;

    if (result == EOF)
        *symbol = TXT_sentinel_symbol ();
    return (result);
}

void
encodeText (unsigned int model, unsigned int coder,
	    unsigned int input_file, unsigned int output_file)
/* Encodes the text using the ppm model. */
{
    int cc, pos;
    unsigned int context, symbol;

    context = TLM_create_context (model);

    pos = 0;
    for (;;)
    {
	bytes_input++;
        pos++;
	if ((Debug.progress > 0) && ((pos % Debug.progress) == 0))
	    fprintf (stderr, "position %d bytes input %d bytes output %d %.3f bpc\n", pos, bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);


        cc = getSymbol (input_file, &symbol);
	if (Debug.range != 0)
	  {
	    if (cc == EOF)
	      fprintf (stderr, "Encoding sentinel symbol\n");
	    else
	      fprintf (stderr, "Encoding symbol %d (%c)\n", symbol, symbol);
	  }

	TLM_encode_symbol (model, context, coder, symbol);

	if (Dump_Model)
	  {
	    fprintf (stderr, "Dump of model after position %d updated:\n", pos);
	    TLM_dump_model (Stderr_File, model, NULL);
	  }

	if (symbol == TXT_sentinel_symbol ())
	    break;
    }
    bytes_input--; /* Do not count EOF character so that byte_input = filesize of input file */
    if (Debug.progress > 0)
        fprintf (stderr, "position %d ", pos);
    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int input_file, output_file;
    unsigned int Model, Coder;

    init_arguments (argc, argv);

    if (!Input_file_found)
        input_file = Stdin_File;
    else
	input_file = TXT_open_file
	  (Args_input_filename, "r", "Reading from text file",
	   "Encode: can't open input text file" );

    if (!Output_file_found)
        output_file = Stdout_File;
    else
        output_file = TXT_open_file
	  (Args_output_filename, "w", "Writing to text file",
	   "Encode: can't open output text file" );

    arith_encode_start (output_file);

    if (!Args_model_filename)
        Model = TLM_create_model (TLM_PPM_Model,
		Args_title, Args_alphabet_size, Args_max_order,
		Args_escape_method, Args_performs_full_excls,
		Args_performs_update_excls);
    else /* load existing model */
	Model = TLM_read_model (Args_model_filename, NULL, NULL);

    Coder = TLM_create_arithmetic_encoder (input_file, output_file);

    encodeText (Model, Coder, input_file, output_file);

    arith_encode_finish (output_file);

    fprintf (stderr, "bytes input %d bytes output %d %.3f bpc\n", bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);

    /*fprintf (stderr, "Size of model        = %d\n", TLM_sizeof_model (Model));
      fprintf (stderr, "Min. length of model = %d\n", TLM_minlength_model (Model));*/

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
