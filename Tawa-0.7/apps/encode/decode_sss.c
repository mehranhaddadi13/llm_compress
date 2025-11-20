/* SSS decoder. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "coder.h"
#include "io.h"
#include "sss_model.h"
#include "model.h"

void
decodeSymbols (FILE *fp, unsigned int model, unsigned int coder)
/* Decodes symbols and writes them out to FP. */
{
    unsigned int symbol, context, textlength, p;

    context = TLM_create_context (model);

    textlength = TLM_decode_symbol (model, context, coder);

    for (p = 0; p < textlength; p++)
    {
	if ((Debug.progress > 0) && ((p % Debug.progress) == 0))
	    fprintf (stderr, "decoding pos %d\n", p);

	symbol = TLM_decode_symbol (model, context, coder);

	fprintf (fp, "%d\n", symbol);

	if (Debug.level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
    }
    TLM_release_context (model, context);

    /*fprintf (stderr, "Decoded %d symbols\n", p);*/
}

int
main (int argc, char *argv[])
{
    unsigned int Model, Coder;

    arith_decode_start (Stdin_File);

    Model = TLM_create_model (TLM_SSS_Model, "Fred", 0, 1, 32);

    Coder = TLM_create_arithmetic_coder ();

    decodeSymbols (stdout, Model, Coder);

    arith_encode_finish (Stdin_File);

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
