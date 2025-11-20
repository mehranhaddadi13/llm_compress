/* PPM decoder for numbers. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "text.h"
#include "table.h"
#include "coder.h"
#include "io.h"
#include "sss_model.h"
#include "model.h"

void
decodeNumbers (FILE *fp, unsigned int model, unsigned int model1,
	       unsigned int coder, unsigned int table)
/* Decodes numbers and writes them out to FP. */
{
    unsigned int number, number_id, number_text, number_key, number_count;
    unsigned int context, context1, table_type, types, tokens, p;

    number_text = TXT_create_text ();

    context = TLM_create_context (model);
    context1 = TLM_create_context (model1);

    p = 0;
    for (;;)
    {
	if ((Debug.progress > 0) && ((p % Debug.progress) == 0))
	    fprintf (stderr, "decoding pos %d\n", p);

	TXT_getinfo_table (table, &table_type, &types, &tokens);
	number_id = TLM_decode_symbol (model, context, coder);
	if (number_id == TXT_sentinel_symbol ())
	    break;
	else if (number_id < types)
	  { /* number existed before */
	    number_key = TXT_getkey_table (table, number_id);
	    TXT_scanf_text (number_key, "%d", &number);
	  }
	else
	  { /* found a new number */
	    number = TLM_decode_symbol (model1, context1, coder);
	    TXT_setlength_text (number_text, 0);
	    TXT_sprintf_text (number_text, "%d", number);
	    TXT_update_table (table, number_text, &number_id, &number_count);
	  }

	p++;
	fprintf (fp, "%d\n", number);

	if (Debug.level > 1)
	    TLM_dump_model (Stderr_File, model, NULL);
    }
    TLM_release_context (model, context);

    /*fprintf (stderr, "Decoded %d numbers\n", p);*/
}

int
main (int argc, char *argv[])
{
    unsigned int Model, Model1, Coder, Table;

    arith_decode_start (Stdin_File);

    Model  = TLM_create_model (TLM_PPM_Model, "Fred", 0, 0, TLM_PPM_Method_D, TRUE);
    Model1 = TLM_create_model (TLM_SSS_Model, "Fred", 0, 2, 32);

    Coder = TLM_create_arithmetic_coder ();

    Table = TXT_create_table (TLM_Dynamic, 0);

    decodeNumbers (stdout, Model, Model1, Coder, Table);

    arith_encode_finish (Stdin_File);

    TLM_release_model (Model);
    TLM_release_coder (Coder);

    return (0);
}
