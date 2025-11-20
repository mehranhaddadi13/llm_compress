/* Test dynamic PPMD encoder (based on characters) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "coder.h"
#include "io.h"
#include "text.h"
#include "model.h"

void
encodeText (unsigned int model, unsigned int coder)
/* Encodes some symbols using the ppm model. */
{
    unsigned int context;

    context = TLM_create_context (model);

    TLM_dump_model (Stderr_File, model, NULL);
    TLM_encode_symbol (model, context, coder, TXT_sentinel_symbol ());

    TLM_dump_model (Stderr_File, model, NULL);
    TLM_encode_symbol (model, context, coder, 32);

    TLM_dump_model (Stderr_File, model, NULL);
    TLM_encode_symbol (model, context, coder, TXT_sentinel_symbol ());

    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int Model, Coder;

    arith_encode_start (Stdout_File);

    Model = TLM_create_model (TLM_PPM_Model, "Test", 256, 4, TRUE);

    Coder = TLM_create_arithmetic_coder ();

    encodeText (Model, Coder);

    arith_encode_finish (Stdout_File);

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
