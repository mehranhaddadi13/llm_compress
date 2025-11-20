/* Writes out the codelength for the input string specified by the -s argument
   for the model specified by the -m argument. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#if defined (SYSTEM_LINUX)
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "codelength_utils.h"


#define MAX_ORDER 5       /* Maximum order of the model */

#define MAX_ALPHABET 256  /* Default max. alphabet size for all PPM models */

#define MAX_FILENAME 256  /* Maximum length of a filename string */
#define MAX_STRING 10000  /* Maximum length of a string */

int Debug_model = 0;      /* For dumping out the model */
int Debug_chars = 0;      /* For dumping out the codelength character by character */
int Display_entropy = 0;  /* For displaying cross-entropies and not codelengths */
unsigned int Model = 0;   /* The model being loaded */

char Input_String [MAX_STRING] = ""; /* The string being encoded */
boolean Encode_Numbers = FALSE;      /* TRUE if the input string is a list of numbers */

int
getSymbol (char *text, unsigned int *pos, unsigned int *symbol)
/* Gets the next symbol from input stream TEXT and updates the position POS.
   Returns EOF if there are no more symbols or an error has occurred. */
{
    unsigned int sym;
    unsigned int p;
    int result;

    sym = 0;
    p = *pos;
    if (!Encode_Numbers)
      {
        sym = text [p];
	p++;
	if (sym == '\0')
	    result = EOF;
	else
	    result = 1;
      }
    else
      {
	/* Skip over leading spaces */
	while ((text [p] != '\0') && (text [p] == ' '))
	    p++;

	/* Scan the next number from the string */
	if (text [p] == '\0')
	    result = EOF;
	else
	    result = sscanf (text + p, "%u", &sym);
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
	/* Find where next space is */
	while ((text [p] != '\0') && (text [p] != ' '))
	    p++;
      }

    *symbol = sym;
    *pos = p;

    if (result == EOF)
        *symbol = TXT_sentinel_symbol ();
    return (result);
}

float
codelengthText (unsigned int model, char *text)
/* Returns the codelength (in bits) for encoding the text using the PPM model. */
{
    unsigned int context, symbol, p;
    float codelen, codelength;

    context = TLM_create_context (model);
    TLM_set_context_operation (TLM_Get_Codelength);

    /* Insert the sentinel symbol at start of text to ensure first character
       is encoded using a sentinel symbol context rather than an order 0
       context */
    if (Debug.range)
        fprintf (stderr, "Coding ranges for the sentinel symbol (not included in overall total:\n");

    TLM_update_context (model, context, TXT_sentinel_symbol ());
    if (Debug.range)
        fprintf (stderr, "\n");

    codelength = 0.0;
    /* Now encode each symbol */
    p = 0;
    while (getSymbol (text, &p, &symbol) != EOF) /* encode each symbol */
    {
	TLM_update_context (model, context, symbol);
	codelen = TLM_Codelength;
	if (Debug_chars)
	  {
	    if (Encode_Numbers)
	      fprintf (stderr, "Codelength for number %d = %7.3f\n", symbol, codelen);
	    else
	      fprintf (stderr, "Codelength for character %d (%c) = %7.3f\n", symbol, symbol, codelen);
	  }
	codelength += codelen;
    }
    /* Now encode the sentinel symbol again to signify the end of the text */
    TLM_update_context (model, context, TXT_sentinel_symbol ());
    codelen = TLM_Codelength;
    if (Debug_chars)
        fprintf (stderr, "Codelength for sentinel symbol = %.3f\n", codelen);
    codelength += codelen;

    TLM_release_context (model, context);

    if (Display_entropy)
        return (codelength / p);
    else
        return (codelength);
}
