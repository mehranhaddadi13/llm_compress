/* SSS encoder. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "coder.h"
#include "text.h"
#include "io.h"
#include "sss_model.h"
#include "model.h"

unsigned int symbols_input = 0;

int
getSymbol (FILE *fp, unsigned int *symbol)
/* Returns the next symbol from input stream fp. */
{
    unsigned int sym;
    int result;

    sym = 0;

    result = fscanf (fp, "%u", &sym);
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

    *symbol = sym;
    return (result);
}

void
encodeSymbols (FILE *fp, unsigned int model, unsigned int coder)
/* Encodes symbols using the characters in FP. */
{
    unsigned int symbols, symbol, context, textlength, pos;

    symbols = TXT_create_text ();

    context = TLM_create_context (model);

    symbols_input = 0;
    for (;;)
    {
	if ((Debug.progress > 0) && ((symbols_input % Debug.progress) == 0))
	    fprintf (stderr, "encoding pos %d\n", symbols_input);

        /* repeat until EOF or max input */
        if (getSymbol (fp, &symbol) == EOF)
	    break;
	TXT_append_symbol (symbols, symbol);
	symbols_input++;

	if (Debug.level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
    }

    textlength = TXT_length_text (symbols);
    TLM_encode_symbol (model, context, coder, textlength);
    
    for (pos = 0; pos < textlength; pos++)
      {
	TXT_get_symbol (symbols, pos, &symbol);
	TLM_encode_symbol (model, context, coder, symbol);
      }

    TLM_release_context (model, context);
    TXT_release_text (symbols);

    fprintf (stderr, "Encoded %d symbols\n", symbols_input);
}

int
main (int argc, char *argv[])
{
    unsigned int Model, Coder;

    arith_encode_start (Stdout_File);

    Model = TLM_create_model (TLM_SSS_Model, "Fred", 0, 1, 32);

    Coder = TLM_create_arithmetic_coder ();

    encodeSymbols (stdin, Model, Coder);

    arith_encode_finish (Stdout_File);

    fprintf (stderr, "Symbols input %d, bytes output %d (%.3f bits per symbol)\n",
	     symbols_input, bytes_output,
	     (8.0 * bytes_output) / symbols_input);

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
