/* "PPMo training program (update model but don't encode it) */
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "assert.h"

#include "io.h"
#include "text.h"
#include "coder.h"
#include "model.h"
#include "ppmo_model.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

#define DEFAULT_ALPHABET_SIZE 256
#define DEFAULT_OMAX_ORDER 3
#define DEFAULT_SMAX_ORDER 4

unsigned int Input_file;
char Input_filename [MAX_FILENAME_SIZE];
unsigned int Output_file;
char Output_filename [MAX_FILENAME_SIZE];

boolean Use_Numbers = FALSE;
unsigned int Alphabet_Size;
unsigned int PPMo_Mask;
int OMax_Order;
int SMax_Order;
int OS_Model_Threshold;
boolean Performs_Excls;

unsigned int Position = 0;

void junk ()
/* For debugging purposes. */
{
  fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train_ppmo [options]\n"
	     "\n"
	     "options:\n"
	     "  -a n\talphabet size=c (default = 256)\n"
	     "  -d n\tdebug level=n\n"
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -M str\tmask orders=str (do not code them)\n"
	     "  -N\tinput text file is a sequence of unsigned numbers\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -O n\torder of order model=n (default = 3)\n");
    fprintf (stderr,
	     "  -p n\tdebug progress=n\n"
	     "  -S n\torder of symbol model=n (default = 4)\n"
	     "  -T n\tset threshold for OS order stream modelling to n\n"
	     "  -X\tperform exclusions (default=FALSE)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    int opt;

    boolean Input_found = FALSE, Output_found = FALSE;

    PPMo_Mask = TXT_create_text ();

    Alphabet_Size = DEFAULT_ALPHABET_SIZE;
    OMax_Order = DEFAULT_OMAX_ORDER;
    SMax_Order = DEFAULT_SMAX_ORDER;
    OS_Model_Threshold = PPMo_DEFAULT_OS_MODEL_THRESHOLD;
    Performs_Excls = PPMo_DEFAULT_PERFORMS_EXCLS;

    /* get the argument options */
    while ((opt = getopt (argc, argv, "a:d:i:M:No:O:p:S:T:X")) != -1)
	switch (opt)
	{
	case 'a':
	    Alphabet_Size = atol (optarg);
	    break;
	case 'd':
	    Debug.level = atol (optarg);
	    break;
	case 'i':
	    Input_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'M':
	    fprintf( stderr, "Masking orders %s\n", optarg);
	    TXT_append_string (PPMo_Mask, optarg);
	    break;
	case 'N':
	    Use_Numbers = TRUE;
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'O':
	    OMax_Order = atol (optarg);
	    assert (OMax_Order >= 0);
	    break;
	case 'p':
	    Debug.progress = atol (optarg);
	    break;
	case 'S':
	    SMax_Order = atol (optarg);
	    assert (SMax_Order >= -1);
	    break;
	case 'T':
	    OS_Model_Threshold = atoi (optarg);
	    break;
	case 'X':
	    Performs_Excls = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

int
getSymbol (unsigned int file, unsigned int *symbol)
/* Returns the next symbol from the input file. */
{
    unsigned int sym;
    int result;

    sym = 0;
    if (Use_Numbers)
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
    return (result);
}

void PPMo_train_model (unsigned int file, unsigned int model, unsigned int context)
/* Trains the model using the text from the file. */
{
    unsigned int symbol;

    /* dump_memory (stdout); */

    Position = 0;
    for (;;)
      {
        if (getSymbol (file, &symbol) == EOF)
	    break;
	if (Debug.level > 4)
	    fprintf (stderr, ">>> Input pos %d symbol %d\n", Position, symbol);

	if (Debug.progress && (Position % Debug.progress) == 0)
	  {
	    fprintf (stderr, "%d symbols processed\n", Position);
	  }

	TLM_update_context (model, context, symbol);

	if (Debug.level > 4)
	  {
	    TLM_dump_model (Stderr_File, model, NULL);
	  }

	Position++;
      }

    if (Debug.level > 4)
      {
        TLM_dump_model (Stderr_File, model, NULL);
      }
}

int
main(int argc, char *argv [])
{
    unsigned int model, context;

    init_arguments (argc, argv);

    Input_file = TXT_open_file
      (Input_filename, "r", "Reading input file",
       "Train_ppmo: can't open input file" );
    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Train_ppmo: can't open output file" );

    model = TLM_create_model (TLM_PPMo_Model, "Encode PPMo Model", Alphabet_Size, OMax_Order,
			      SMax_Order, PPMo_Mask, OS_Model_Threshold,
			      Performs_Excls);
    context = TLM_create_context (model); 

    PPMo_train_model (Input_file, model, context);

    TLM_write_model (Output_file, model, TLM_Static);

    if (Debug.level > 3)
      {
        TLM_dump_model (Stderr_File, model, NULL);
      }
    exit (0);
}
