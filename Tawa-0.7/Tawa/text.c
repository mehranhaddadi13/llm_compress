/* Text modelling routines. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>
#include "io.h"
#include "text.h"

#define MAX_TEXTLENGTH_ASSERTION 10000000
/* For testing text length is not too large - probably caused by an error */

#define TEXTS_SIZE 20             /* Initial size of Texts array */
#define TEXT_SIZE 8               /* Initial size of a text record */

#define MAX_NUMBER_STR_SIZE 24    /* Max. length of a number string */
#define MAX_FLOAT_STR_SIZE 24     /* Max. length of a float string */

struct textType
{ /* Input text to correct, or corrected output text */
    unsigned int Text_alloc;      /* Current maximum allocation of the text. */
    unsigned int Text_length;     /* Length of the text. */
    unsigned int *Text;           /* The text. */
};
#define Text_next Text_alloc      /* Used to find the "next" on the used list
				     for deleted texts */

struct textType *Texts = NULL;    /* List of text records */
unsigned int Texts_max_size = 0;  /* Current max. size of the texts array */
unsigned int Texts_used = NIL;    /* List of deleted text records */
unsigned int Texts_unused = 1;    /* Next unused text record */

/* Global variable used for storing the latest input symbol. */
unsigned int TXT_Input_Symbol;

void
debug_text ()
{
    fprintf (stderr, "Got here\n");
}

void
TXT_write_symbol (unsigned int file, unsigned int symbol)
/* Writes the ASCII symbol out in human readable form. */
{
    assert (TXT_valid_file (file));

    if (symbol != TXT_sentinel_symbol ())
	fprintf (Files [file], "%c", symbol);
}

void
TXT_dump_symbol (unsigned int file, unsigned int symbol)
/* Writes the ASCII symbol out in human readable form (excluding '\n' and '\t's). */
{
    assert (TXT_valid_file (file));

    if ((symbol >= IS_SPACE) && (symbol < IS_ASCII))
	fprintf (Files [file], "%c", symbol);
    else if (symbol == TXT_sentinel_symbol ())
	fprintf (Files [file], "<sentinel>");
    else
	fprintf (Files [file], "<%d>", symbol);
}

void
TXT_dump_symbol1 (unsigned int file, unsigned int symbol)
/* Writes the ASCII symbol out in human readable form (excluding white space). */
{
    assert (TXT_valid_file (file));

    if ((symbol > IS_SPACE) && (symbol < IS_ASCII))
	fprintf (Files [file], "%c", symbol);
    else if (symbol == TXT_sentinel_symbol ())
	fprintf (Files [file], "<sentinel>");
    else
	fprintf (Files [file], "<%d>", symbol);
}

void
TXT_dump_symbol2 (unsigned int file, unsigned int symbol)
/* Writes the ASCII symbol out in human readable form (including '\n' and '\t's). */
{
    assert (TXT_valid_file (file));

    if (TXT_is_print (symbol) || (symbol == '\n') || (symbol == '\t'))
	fprintf (Files [file], "%c", symbol);
    else if (symbol == TXT_sentinel_symbol ())
	fprintf (Files [file], "<sentinel>");
}

boolean
TXT_is_ascii (unsigned int symbol)
/* Returns TRUE if symbol is an ASCII character. */
{
    if (symbol < IS_ASCII)
        return (TRUE);
    else
        return (FALSE);
}

boolean
TXT_is_vowel (unsigned int symbol)
/* Returns TRUE if symbol is a vowel. */
{
    if (!TXT_is_ascii (symbol))
        return (FALSE);

    switch (symbol)
      {
      case 'A': case 'a':
      case 'E': case 'e':
      case 'I': case 'i':
      case 'O': case 'o':
      case 'U': case 'u':
	  return (TRUE);
	  break;
      default:
	  return (FALSE);
	  break;
      };
}

boolean
TXT_is_consonant (unsigned int symbol)
/* Returns TRUE if symbol is a consonant. */
{
    return (TXT_is_ascii (symbol) && isalpha (symbol) &&
	    !TXT_is_vowel (symbol));
}

boolean
TXT_is_alphanumeric (unsigned int symbol)
/* Returns TRUE if symbol is an alphanumeric character. */
{
    return (TXT_is_ascii (symbol) && isalnum (symbol));
}

boolean
TXT_is_alpha (unsigned int symbol)
/* Returns TRUE if symbol is an alphabetic character. */
{
    return (TXT_is_ascii (symbol) && isalpha (symbol));
}

boolean
TXT_is_control (unsigned int symbol)
/* Returns TRUE if symbol is a control character. */
{
    return (TXT_is_ascii (symbol) && iscntrl (symbol));
}

boolean
TXT_is_digit (unsigned int symbol)
/* Returns TRUE if symbol is a digit. */
{
    return (TXT_is_ascii (symbol) && isdigit (symbol));
}

boolean
TXT_is_graph (unsigned int symbol)
/* Returns TRUE if symbol is a printable character except space. */
{
    return (TXT_is_ascii (symbol) && isgraph (symbol));
}

boolean
TXT_is_lower (unsigned int symbol)
/* Returns TRUE if symbol is a uppercace character. */
{
    return (TXT_is_ascii (symbol) && islower (symbol));
}

unsigned int
TXT_to_lower (unsigned int symbol)
/* Returns the lower case of symbol. */
{
    return (tolower (symbol));
}

boolean
TXT_is_print (unsigned int symbol)
/* Returns TRUE if symbol is a printable character. */
{
    return (TXT_is_ascii (symbol) && isprint (symbol));
}

boolean
TXT_is_punct (unsigned int symbol)
/* Returns TRUE if symbol is a punctuation character. */
{
    return (TXT_is_ascii (symbol) && ispunct (symbol));
}

boolean
TXT_is_space (unsigned int symbol)
/* Returns TRUE if symbol is a white space character. */
{
    return (TXT_is_ascii (symbol) && isspace (symbol));
}
 
boolean
TXT_is_upper (unsigned int symbol)
/* Returns TRUE if symbol is a uppercace character. */
{
    return (TXT_is_ascii (symbol) && isupper (symbol));
}

unsigned int
TXT_to_upper (unsigned int symbol)
/* Returns the upper case of symbol. */
{
    return (toupper (symbol));
}

boolean
TXT_valid_text (unsigned int text)
/* Returns non-zero if the text is valid, zero otherwize. */
{
    if (text == NIL)
        return (FALSE);
    else if (text >= Texts_unused)
        return (FALSE);
    else if (Texts [text].Text_length > Texts [text].Text_alloc)
        return (FALSE); /* The text position gets set to > the text allocation when the model gets deleted;
			   this way you can test to see if the text has been deleted or not */
    else
        return (TRUE);
}

unsigned int
TXT_create_text (void)
/* Creates a text record. */
{
    struct textType *text;
    unsigned int t, old_size;

    if (Texts_used != NIL)
    {	/* use the first list of texts on the used list */
	t = Texts_used;
	Texts_used = Texts [t].Text_next;
    }
    else
    {
	t = Texts_unused;
	if (Texts_unused+1 >= Texts_max_size)
	{ /* need to extend Texts array */
	    old_size = (Texts_max_size+2) * sizeof (struct textType);
	    if (Texts_max_size == 0)
		Texts_max_size = TEXTS_SIZE;
	    else
		Texts_max_size *= 2; /* Keep on doubling the array on demand */
	    Texts = (struct textType *) Realloc (11, Texts, (Texts_max_size+2) * sizeof (struct textType),
						 old_size);

	    if (Texts == NULL)
	    {
		fprintf (stderr, "Fatal error: out of texts space\n");
		exit (1);
	    }
	}
	Texts_unused++;
    }

    if (t != NIL)
    {
      text = Texts + t;

      text->Text = NULL;
      text->Text_alloc = 0;
      text->Text_length = 0;
    }

    /*fprintf (stderr, "Creating text %d\n", t);*/
    return (t);
}

void
TXT_release_text (unsigned int text)
/* Releases the memory allocated to the text and the text number (which may
   be reused in later TXT_create_text calls). */
{
    struct textType *textp;

    if (text == NIL)
        return;

    assert (TXT_valid_text (text));
    /*fprintf (stderr, "Releasing text %d\n", text);*/

    textp = Texts + text;

    if (textp->Text != NULL)
      {
	assert (textp->Text_alloc != 0);
        Free (12, textp->Text, (textp->Text_alloc+2) * sizeof (unsigned int));
      }

    textp->Text_alloc = 0; /* Used for testing later on if text no. is valid or not */
    textp->Text_length = 1; /* Used for testing later on if text no. is valid or not */

    /* Append onto head of the used list */
    textp->Text_next = Texts_used;
    Texts_used = text;
}

int
TXT_compare_text (unsigned int text1, unsigned int text2)
/* Compares text1 with text2. Returns zero if they are the same,
   negative if text1 < text2, and positive if text1 > text2.
   The sentinel symbol returned by TXT_sentinel_symbol is
   regarded as having a value lower than all other symbols. */
{
    assert (TXT_valid_text (text1));
    assert (TXT_valid_text (text2));

    if (text1 == text2)
        return (0); /* equal */
    else
      {
	struct textType *t1, *t2;
	unsigned int p, tsym1, tsym2, tlen, tlen1, tlen2;
	int cmp;

	t1 = Texts + text1;
	t2 = Texts + text2;
	tlen1 = t1->Text_length;
	tlen2 = t2->Text_length;

	if (!tlen1 && !tlen2)
	    return (0);
	else if (tlen1 && !tlen2)
	    return (1);
	else if (!tlen1 && tlen2)
	    return (-1);

	assert ((t1->Text != NULL) && (t2->Text != NULL));
	if (tlen1 > tlen2)
	    tlen = tlen2;
	else
	    tlen = tlen1;
	cmp = 0;
	for (p=0; p < tlen; p++)
	  {
	    tsym1 = t1->Text [p];
	    tsym2 = t2->Text [p];

	    if (tsym1 == tsym2)
	      continue;
	    if (tsym1 == TXT_sentinel_symbol ())
	      {
		cmp = -1;
		break;
	      }
	    else if (tsym2 == TXT_sentinel_symbol ())
	      {
		cmp = 1;
		break;
	      }
	    else if (tsym1 < tsym2)
	      {
		cmp = -1;
		break;
	      }
	    else
	      {
		cmp = 1;
		break;
	      }
	  }
	if ((tlen1 == tlen2) || (cmp != 0))
	    return (cmp);
	else if (tlen1 > tlen2)
	    return (1);
	else
	    return (-1);
      }
}

unsigned int
TXT_alloc_text (unsigned int text)
/* Returns the allocation of the text (it's memory use; for debugging purpose). */
{
    assert (TXT_valid_text (text));

    return (Texts [text].Text_alloc);
}

unsigned int
TXT_length_text (unsigned int text)
/* Returns the length of the text. Assumes the text is non-NIL. */
{
    assert (TXT_valid_text (text));

    return (Texts [text].Text_length);
}

unsigned int
TXT_length_text1 (unsigned int text)
/* Returns the length of the text. Returns 0 if the text is NIL. */
{
    if (text == NIL)
        return (0);
    else
        return (TXT_length_text (text));
}

unsigned int
TXT_setlength_text (unsigned int text, unsigned int length)
/* Sets the length of the text to be at most length symbols long. If the
   current length of the text is longer than this, then the text will be
   truncated to the required length, otherwise the length will remain
   unchanged. Setting the length of the text to be 0 will set the text
   to the null string. */
{
    assert (TXT_valid_text (text));

    if (length < Texts [text].Text_length)
        Texts [text].Text_length = length;

    return (Texts [text].Text_length);
}

boolean
TXT_getpos_text (unsigned int text, unsigned int symbol, unsigned int *pos)
/* Returns TRUE if the symbol is found in the text. The argument pos is set to the
   position of the first symbol in the text that matches the specified symbol if
   found, otherwise it remains unchanged. */
{
    unsigned int p;

    assert (TXT_valid_text (text));

    for (p = 0; p < Texts [text].Text_length; p++)
      if (symbol == Texts [text].Text [p])
	{
	  *pos = p;
	  return (TRUE);
	}
    return (FALSE);
}

boolean
TXT_getrpos_text (unsigned int text, unsigned int symbol, unsigned int *pos)
/* Returns TRUE if the symbol is found in the text. The argument pos is set to the
   position of the last symbol in the text that matches the specified symbol if
   found, otherwise it remains unchanged. */
{
    int p;

    assert (TXT_valid_text (text));

    for (p = Texts [text].Text_length - 1; p >= 0; p--)
      if (symbol == Texts [text].Text [p])
	{
	  *pos = p;
	  return (TRUE);
	}
    return (FALSE);
}

void
TXT_string_text (unsigned int text, char *string, unsigned int max)
/* Returns as ASCII version of the text in string. */
{
    struct textType *textp;
    unsigned int p;

    assert (TXT_valid_text (text));
    textp = Texts + text;

    assert (textp->Text != NULL);
    for (p=0; (p < textp->Text_length) && (p < max); p++)
      string [p] = textp->Text [p] % IS_ASCII;
    string [p] = '\0';
}

boolean
getstr_text (unsigned int text, char *string, unsigned int *pos)
{
    unsigned int p, q;

    p = 0;
    q = 0;
    for (p = 0; p < Texts [text].Text_length; p++)
    {
      if (!string [q])
	{
	  *pos = p - q;
	  return (TRUE);
	}
      else if (Texts [text].Text [p] == (unsigned int) string [q])
	q++;
      else
	q = 0;
    }
    if (!string [q])
      {
	*pos = p - q;
        return (TRUE);
      }
    else
      {
        *pos = p + 1;
        return (FALSE);
      }
}

boolean
TXT_getstr_text (unsigned int text, char *string, unsigned int *pos)
/* Returns TRUE if the string is found in the text. The argument pos is set
   to the position of the first symbol in the text starting from position
   0 that matches the specified string if found, otherwise it remains
   unchanged. */
{
    assert (TXT_valid_text (text));

    *pos = 0;
    return (getstr_text (text, string, pos));
}

boolean
TXT_getstring_text (unsigned int text, char *string, unsigned int *pos)
/* Returns TRUE if the string is found in the text. The argument pos contains
   the starting point and gets set to the position of the first point
   in the following text that matches the specified string if found,
   otherwise it remains unchanged. */
{
    assert (TXT_valid_text (text));

    return (getstr_text (text, string, pos));
}

int
TXT_strcmp_text (unsigned int text, char *string)
/* Compares the text with the string. Returns zero if they are the same,
   negative if text < string, and positive if text > string. */
{
    unsigned int p;

    assert (TXT_valid_text (text));

    p = 0;
    for (p = 0; string [p] && (p < Texts [text].Text_length); p++)
    {
      if (Texts [text].Text [p] < (unsigned int) string [p])
	  return (-1);
      else if (Texts [text].Text [p] > (unsigned int) string [p])
	  return (1);
    }
    if (!string [p])
      if (p < Texts [text].Text_length)
	return (1);
      else
	return (0);
    else
	return (-1);
}

boolean
TXT_null_text (unsigned int text)
/* Returns TRUE if the text is empty. */
{
    if (TXT_length_text (text) == 0)
      return (TRUE);
    else
      return (FALSE);
}

unsigned int
TXT_sentinel_symbol (void)
/* Returns an unsigned integer that uniquely identifies a special ``sentinel''
   symbol. */
{
   return ((unsigned int) SENTINEL_SYMBOL);
}

unsigned int
TXT_sentinel1_symbol (void)
/* Returns an unsigned integer that uniquely identifies another special ``sentinel1''
   symbol. */
{
   return ((unsigned int) SENTINEL_SYMBOL+1);
}

boolean
TXT_sentinel_text (unsigned int text)
/* Returns TRUE if the text contains just a sentinel symbol. */
{
    if ((TXT_length_text (text) == 1) &&
	(Texts [text].Text [0] == TXT_sentinel_symbol ()))
      return (TRUE);
    else
      return (FALSE);
}

unsigned int
TXT_createsentinel_text (void)
/* Returns text that contains just a sentinel symbol. */
{
    unsigned int text;

    text = TXT_create_text ();
    TXT_append_symbol (text, TXT_sentinel_symbol ());
    return (text);
}

boolean
TXT_getsymbol_text (unsigned int text, unsigned int pos)
/* Returns the symbol at position in the text. A run-time assertion error
   occurs if pos is greater then the length of the text. */
{
    assert (TXT_valid_text (text));
    assert (pos < Texts [text].Text_length);
    return (Texts [text].Text [pos]);
}

boolean
TXT_getsymbol_file (unsigned int file, boolean load_numbers)
/* Gets the next symbol (in the global variable TXT_Input_Symbol) from input
   stream FILE. Returns FALSE when there are no more symbols (EOF has been reached).
   If load_numbers is TRUE, then numbers will be scanned instead of characters. */
{
    unsigned int sym;
    int result;

    sym = 0;
    if (load_numbers)
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

    TXT_Input_Symbol = sym;
    return (result != EOF);
}

boolean
TXT_get_symbol (unsigned int text, unsigned int pos, unsigned int *symbol)
/* Returns TRUE if there exists a symbol at position exists in the text. 
   The argument \verb|symbol| is set to the specified symbol. */
{
    assert (TXT_valid_text (text));
    if (pos >= Texts [text].Text_length)
        return (FALSE);
    else
      {
	*symbol = Texts [text].Text [pos];
	return (TRUE);
      }
}

void
TXT_put_symbol (unsigned int text, unsigned int symbol, unsigned int pos)
/* Inserts the symbol at position pos into the text. Inserting a symbol beyond
   the current bounds of the text will cause a run-time error. */
{
    assert (TXT_valid_text (text));
    assert (pos < Texts [text].Text_length);

    Texts [text].Text [pos] = symbol;
}

boolean
TXT_find_symbol (unsigned int text, unsigned int symbol, unsigned int start_pos,
		 unsigned int *pos)
/* Returns TRUE if the symbol exists in the text starting from start_pos. 
   The argument \verb|pos| us set to the symbol's position. */
{
    unsigned int p, sym;

    assert (TXT_valid_text (text));

    p = start_pos;
    while (TXT_get_symbol (text, p++, &sym) && (sym != symbol));
    *pos = p-1;
    return (sym == symbol);
}

boolean
find_text (unsigned int text, unsigned int subtext, unsigned int *pos)
{
    unsigned int p, q, sym;

    p = 0;
    q = 0;
    for (p = 0; p < Texts [text].Text_length; p++)
    {
      if (!TXT_get_symbol (subtext, q, &sym))
	{
	  *pos = p - q;
	  return (TRUE);
	}
      else if (Texts [text].Text [p] == sym)
	q++;
      else
	q = 0;
    }
    if (!TXT_get_symbol (subtext, q, &sym))
      {
	*pos = p - q;
        return (TRUE);
      }
    else
      {
        *pos = p + 1;
        return (FALSE);
      }
}

boolean
TXT_find_text (unsigned int text, unsigned int subtext, unsigned int *pos)
/* Returns TRUE if the subtext is found in the text. The argument pos is set
   to the position of the first symbol in the text starting from position
   0 that matches the specified string if found, otherwise it remains
   unchanged. */
{
    assert (TXT_valid_text (text));
    assert (TXT_valid_text (subtext));

    *pos = 0;
    return (find_text (text, subtext, pos));
}

void
TXT_append_symbol (unsigned int text, unsigned int symbol)
/* Appends the symbol onto the end of the text. */
{
    struct textType *textp;
    unsigned int p, old_size;

    assert (TXT_valid_text (text));
    textp = Texts + text;

    p = textp->Text_length;
    if (p >= textp->Text_alloc)
    { /* extend array */
	if (textp->Text_alloc == 0)
	    old_size = 0;
	else
	    old_size = (textp->Text_alloc+2) * sizeof(unsigned int);

	if (textp->Text_alloc == 0)
	    textp->Text_alloc = TEXTS_SIZE;
	else
	    textp->Text_alloc = (10 * (textp->Text_alloc + 50))/9; /* Keep on doubling the array on demand */
	textp->Text = (unsigned int *) Realloc (12, textp->Text,
				       (textp->Text_alloc+2) * sizeof(unsigned int), old_size);
	if (textp->Text == NULL)
	{
	    fprintf (stderr, "Fatal error: out of text space at position %d\n", p);
	    exit (1);
	}
    }
    textp->Text [p] = symbol;
    textp->Text_length++;
}

void
TXT_append_string (unsigned int text, char *string)
/* Appends the string onto the end of text. */
{
    unsigned int symbol, pos;

    assert (TXT_valid_text (text));

    pos = 0;
    while ((symbol = string [pos++]))
        TXT_append_symbol (text, symbol);
}

boolean
TXT_find_string (unsigned int text, char *string, unsigned int start_pos,
		 unsigned int *pos)
/* Returns TRUE if the string exists in the text starting from start_pos. 
   The argument \verb|pos| us set to the string's position. */
{
    unsigned int p, q, sym;
    boolean found_sym;

    assert (TXT_valid_text (text));

    p = start_pos;
    /* find first symbol */
    for (;;)
      {
	/* Skip to start of string */
	while ((found_sym = TXT_get_symbol (text, p++, &sym)) && (sym != string [0]));
	q = 0;
	while (found_sym && string [q] && (sym == string [q]))
	  {
	    found_sym = TXT_get_symbol (text, p++, &sym);
	    q++;
	  }
	if (!found_sym || !string [q])
	  break;
      }

    if (string [q])
        *pos = p;
    else
        *pos = p - strlen(string) - 1;
    return (!string [q]);
}

void
TXT_nullify_string (unsigned int text, char *string)
/* Replaces all occurences of string in the text with nulls. (This can be used for
   "removing" the string if using TXT_dump_symbol2 to dump it out. */
{
    unsigned int p, pos, start_pos;

    start_pos = 0;
    while (TXT_find_string (text, string, start_pos, &pos))
      {
	for (p = pos; p < pos + strlen (string); p++)
	  TXT_put_symbol (text, 0, p); /* replace with a null */
	start_pos = pos + 1;
      }
}

void
TXT_overwrite_string (unsigned int text, char *string, char *new_string)
/* Overwrites all occurences of string in the text with new_string. Appends
   nulls (or truncates) if there is a mis-match in lengths of the string. */
{
    unsigned int p, q, pos, start_pos;

    start_pos = 0;
    while (TXT_find_string (text, string, start_pos, &pos))
      {
	for (q = 0, p = pos; q < strlen (string); q++, p++)
	  if (q >= strlen (new_string))
	      TXT_put_symbol (text, 0, p); /* replace with a null */
	  else
	      TXT_put_symbol (text, new_string [q], p); /* replace with new string */
	start_pos = pos + 1;
      }
}

void
TXT_sprintf_text (unsigned int text, char *format, ...)
/* Sets the text to the symbols specified by the format and variable length
   argument list. Meaning of the formatting characters:

         %%    - the % (percentage) character is inserted into the text.
         %s    - the argument list contains a symbol number (unsigned int)
	         which will be inserted into the text.
	 %d    - the argument list contains an unsigned int which will be
	         written into the text.
 */
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int symbol, number, p;
    boolean percent_found;
    char number_str [MAX_NUMBER_STR_SIZE];

    assert (TXT_valid_text (text));

    va_start (args, format);

    /* process the text's format */
    percent_found = FALSE;
    for (p = 0; p < strlen (format); p++)
      {
	symbol = format [p];
	if (!percent_found)
	  {
	    if (symbol == '%')
	        percent_found = TRUE;
	    else
		TXT_append_symbol (text, symbol);
	    continue;
	  }
	symbol = format [p];
	switch (symbol)
	  {
	  case '%':
	    TXT_append_symbol (text, '%');
	    break;
	  case 's':
	    symbol = va_arg (args, unsigned int);
	    TXT_append_symbol (text, symbol);
	    break;
	  case 'd':
	    number = va_arg (args, unsigned int);
	    sprintf (number_str, "%d", number);
	    TXT_append_string (text, number_str);
	    break;
	  default:
	    /* error */
	    assert ((symbol == '%') || (symbol == 's'));
	    break;
	  }
	percent_found = FALSE;
      }

    va_end (args);
}

void
TXT_scanf_text (unsigned int text, char *format, ...)
/* Scans in the text the arguments specified by the variable length
   argument list in the format. Meaning of the formatting characters:

	 %d    - the argument list contains an unsigned int
	         which will be scanned from the text.
	 %f    - the argument list contains a float
	         which will be scanned from the text.
 */
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int *number_value, symbol, p;
    float *float_value;
    boolean percent_found;
    char number_str [MAX_NUMBER_STR_SIZE];
    char float_str [MAX_FLOAT_STR_SIZE];

    assert (TXT_valid_text (text));

    va_start (args, format);

    /* process the text's format */
    percent_found = FALSE;
    for (p = 0; p < strlen (format); p++)
      {
	symbol = format [p];
	if (!percent_found)
	  {
	    if (symbol == '%')
	        percent_found = TRUE;
	    else
		TXT_append_symbol (text, symbol);
	    continue;
	  }
	symbol = format [p];
	switch (symbol)
	  {
	  case '%':
	    /* Ignore for now */
	    break;
	  case 's':
	    /* Ignore for now */
	    break;
	  case 'd':
	    number_value = va_arg (args, unsigned int *);
	    TXT_string_text (text, number_str, MAX_NUMBER_STR_SIZE);
	    sscanf (number_str, "%u", number_value);
	    break;
	  case 'f':
	    float_value = va_arg (args, float *);
	    TXT_string_text (text, float_str, MAX_FLOAT_STR_SIZE);
	    sscanf (float_str, "%f", float_value);
	    break;
	  default:
	    /* error */
	    assert ((symbol == '%') || (symbol == 's'));
	    break;
	  }
	percent_found = FALSE;
      }

    va_end (args);
}

void
TXT_extract_text (unsigned int text, unsigned int subtext,
		  unsigned int subtext_pos, unsigned int subtext_len)
/* Extracts the text from out of the text record. The argument subtext is set
   to the extracted text; subtext_len is the length of the text to be
   extracted; and subtext_pos is the position from which the text should be
   extracted from. The extracted subtext is filled with nulls for any part of
   it that extends beyond the bounds of the text record. */
{
    struct textType *textp, *subtextp;
    unsigned int p;

    if (subtext_len == 0)
        return;

    assert (TXT_valid_text (text));
    textp = Texts + text;
    assert (textp->Text != NULL);

    assert (TXT_valid_text (subtext));
    subtextp = Texts + subtext;
    subtextp->Text_length = 0;

    for (p=0; (p < subtext_len) && (subtext_pos + p < textp->Text_length); p++)
	TXT_append_symbol (subtext, textp->Text [subtext_pos+p]);
}

void
TXT_extractstring_text (unsigned int text, char *string,
			unsigned int subtext_pos, unsigned int subtext_len)
/* Returns as ASCII version of the extracted text in string. */
{
    struct textType *textp;
    unsigned int p, q;

    if (subtext_len == 0)
        return;

    assert (TXT_valid_text (text));
    textp = Texts + text;
    assert (textp->Text != NULL);

    q = 0;
    for (p=0; (p < subtext_len) && (subtext_pos + p < textp->Text_length); p++)
	string [q] = textp->Text [subtext_pos+p];
}

unsigned int
TXT_copy_text (unsigned int text)
/* Creates a new text record and text number, then copies the text record
   into it. */
{
    struct textType *textp;
    unsigned int new_text, p;

    if (text == NIL)
        return (NIL);

    assert (TXT_valid_text (text));

    new_text = TXT_create_text (); /* remember to reset all textp's past here because of realloc */

    textp = Texts + text;
    if (textp->Text_length == 0)
        return (new_text);
    assert (textp->Text != NULL);

    for (p=0; p < textp->Text_length; p++)
        TXT_append_symbol (new_text, textp->Text [p]);

    return (new_text);
}

void
TXT_overwrite_text (unsigned int text, unsigned int text1)
/* Overwrites the text with a copy of text1. */
{
    struct textType *textp;
    unsigned int p;

    assert (TXT_valid_text (text));
    assert (TXT_valid_text (text1));

    TXT_setlength_text (text, 0);

    textp = Texts + text1;
    if (textp->Text_length == 0)
        return;
    else
      {
	assert (textp->Text != NULL);

	for (p=0; p < textp->Text_length; p++)
	  TXT_append_symbol (text, textp->Text [p]);
      }
}

void
TXT_append_text (unsigned int text, unsigned int text1)
/* Appends the text1 onto the end of text. */
{
    struct textType *textp;
    unsigned int p;

    assert (TXT_valid_text (text));
    assert (TXT_valid_text (text1));

    textp = Texts + text1;
    if (textp->Text_length == 0)
        return;
    else
      {
	assert (textp->Text != NULL);

	for (p=0; p < textp->Text_length; p++)
	  TXT_append_symbol (text, textp->Text [p]);
      }
}

int
TXT_readline_text (unsigned int file, unsigned int text)
/* Loads the text using the next line of input from the file. Returns the
   last character read or EOF. */
{
    int symbol;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    TXT_setlength_text (text, 0);

    while (((symbol = getc(Files [file])) != EOF) &&
	   ((symbol == '\n') || (symbol == '\r'))); /* skip over eoln characters */

    if (symbol != EOF)
      {
        TXT_append_symbol (text, symbol);
	while (((symbol = getc(Files [file])) != EOF) &&
	        (symbol != '\n') && (symbol != '\r'))
	    TXT_append_symbol (text, symbol);
      }
    return (symbol);
}

int
TXT_readline_text1 (unsigned int file, unsigned int text)
/* Loads the text using the next line of input from the file. Returns the
   last character read or EOF. Does not skip over \r characters. */
{
    int symbol;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    TXT_setlength_text (text, 0);

    while (((symbol = getc(Files [file])) != EOF) &&
	   (symbol == '\n')); /* skip over eoln characters */

    if (symbol != EOF)
      {
        TXT_append_symbol (text, symbol);
	while (((symbol = getc(Files [file])) != EOF) &&
	       (symbol != '\n'))
	    TXT_append_symbol (text, symbol);
      }
    return (symbol);
}

int
TXT_readline_text2 (unsigned int file, unsigned int text)
/* Loads the text using the next line of input from the file. Returns the
   last character read or EOF. Does not skip over \r or \n characters. */
{
    int symbol;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    TXT_setlength_text (text, 0);

    while (((symbol = getc(Files [file])) != EOF) &&
	   (symbol != '\n'))
        TXT_append_symbol (text, symbol);

    return (symbol);
}

boolean
TXT_getline_text (unsigned int text, unsigned int line,
		  unsigned int *pos)
/* Reads the next line from the specified text.
   The argument pos is set to the updated position
   in the text. Returns FALSE when no more text exists. */
{
    unsigned int textlen, symbol, p;

    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    assert (TXT_valid_text (line));
    TXT_setlength_text (line, 0);

    p = *pos;
    if (p >= textlen)
      return (FALSE);

    /* scan in next line */
    while (TXT_get_symbol (text, p, &symbol) &&
	   (symbol != TXT_sentinel_symbol ()) &&
	   (symbol != '\n'))
      {
        TXT_append_symbol (line, symbol);
	p++;
      }

    *pos = p;
    return (TRUE);
}

boolean
TXT_readword_text (unsigned int file, unsigned int non_word,
		   unsigned int word)
/* Reads the non-word and word from the specified file. A word is
   defined as any continuous alphanumeric sequence, and a non-word
   anything in between. Returns FALSE when EOF occurs. */
{
    int symbol;

    assert (TXT_valid_file (file));

    assert (TXT_valid_text (non_word));
    TXT_setlength_text (non_word, 0);

    assert (TXT_valid_text (word));
    TXT_setlength_text (word, 0);

    /* read in non-word */
    while (((symbol = getc (Files [file])) != EOF) && !isalnum (symbol))
        TXT_append_symbol (non_word, symbol);

    /* read in word */
    if (symbol != EOF)
      {
        TXT_append_symbol (word, symbol);
	while (((symbol = getc (Files [file])) != EOF) && isalnum (symbol))
	  TXT_append_symbol (word, symbol);

	ungetc (symbol, Files [file]); /* Push back the last symbol read onto the stream
					  since it was the first of the next non-word */
      }

    return (symbol != EOF);
}

boolean
TXT_readword_text1 (unsigned int file, unsigned int non_word,
		    unsigned int word)
/* Same as TXT_readword_text () except replaces null words with the sentinel word. */
{
    boolean more;

    more = TXT_readword_text (file, non_word, word);

    if (TXT_length_text (non_word) == 0)
        TXT_append_symbol (non_word, TXT_sentinel_symbol ());
    if (TXT_length_text (word) == 0)
        TXT_append_symbol (word, TXT_sentinel_symbol ());

    return (more);
}

boolean
TXT_getword_text (unsigned int text, unsigned int non_word, unsigned int word,
		  unsigned int *text_pos, unsigned int *nonword_text_pos,
		  unsigned int *word_text_pos)
/* Reads the non-word and word from the specified text. A word is
   defined as any continuous alphanumeric sequence, and a non-word
   anything in between. The argument pos is set to the updated position
   in the text. Returns FALSE when no more text exists. */
{
    unsigned int textlen, symbol, p;

    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    assert (TXT_valid_text (non_word));
    TXT_setlength_text (non_word, 0);

    assert (TXT_valid_text (word));
    TXT_setlength_text (word, 0);

    p = *text_pos;
    *nonword_text_pos = p;
    *word_text_pos = p;
    if (p >= textlen)
      return (FALSE);

    /* scan in non-word */
    while (TXT_get_symbol (text, p, &symbol) &&
	   (symbol != TXT_sentinel_symbol ()) &&
	   !isalnum (symbol))
      {
        TXT_append_symbol (non_word, symbol);
	p++;
      }
    if (symbol == TXT_sentinel_symbol ())
      return (FALSE);

    /* scan in word */
    *word_text_pos = p;
    if ((p < textlen) && (symbol != TXT_sentinel_symbol ()))
      {
        TXT_append_symbol (word, symbol);
	p++;
	while (TXT_get_symbol (text, p, &symbol) &&
	       (symbol != TXT_sentinel_symbol ()) &&
	       isalnum (symbol))
	  {
	    TXT_append_symbol (word, symbol);
	    p++;
	  }
      }

    *text_pos = p;
    return (TRUE);
}

boolean
TXT_getword_text1 (unsigned int text, unsigned int non_word, unsigned int word,
		   unsigned int *text_pos, unsigned int *nonword_text_pos,
		   unsigned int *word_text_pos)
/* Same as TXT_getword_text () except replaces null words with the sentinel word. */
{
    boolean more;

    more = TXT_getword_text (text, non_word, word, text_pos, nonword_text_pos, word_text_pos);

    if (TXT_length_text (non_word) == 0)
        TXT_append_symbol (non_word, TXT_sentinel_symbol ());
    if (TXT_length_text (word) == 0)
        TXT_append_symbol (word, TXT_sentinel_symbol ());

    return (more);
}

boolean
TXT_readword1_text (unsigned int file, unsigned int word)
/* Reads the word from the specified file. A word is
   defined as any continuous non-whitespace sequence.
   Returns FALSE when EOF occurs. */
{
    int symbol;

    assert (TXT_valid_file (file));

    assert (TXT_valid_text (word));
    TXT_setlength_text (word, 0);

    /* read in whitespace, if any */
    while (((symbol = getc (Files [file])) != EOF) && isspace (symbol));

    /* read in word */
    if (symbol != EOF)
      {
        TXT_append_symbol (word, symbol);
	while (((symbol = getc (Files [file])) != EOF) && !isspace (symbol))
	  TXT_append_symbol (word, symbol);
      }

    return (symbol != EOF);
}

boolean
TXT_getword1_text (unsigned int text, unsigned int word,
		   unsigned int *pos)
/* Reads the word from the specified text. A word is
   defined as any continuous non-whitespace sequence.
   The argument pos is set to the updated position
   in the text. Returns FALSE when no more text exists. */
{
    unsigned int textlen, symbol, p;

    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    assert (TXT_valid_text (word));
    TXT_setlength_text (word, 0);

    p = *pos;
    if (p >= textlen)
      return (FALSE);

    /* scan in whitespace */
    while (TXT_get_symbol (text, p, &symbol) &&
	   (symbol != TXT_sentinel_symbol ()) &&
	   isspace (symbol))
      {
	p++;
      }

    if (p >= textlen)
      return (FALSE);

    /* scan in word */
    if ((p < textlen) && (symbol != TXT_sentinel_symbol ()))
      {
        TXT_append_symbol (word, symbol);
	p++;
	while (TXT_get_symbol (text, p, &symbol) &&
	       (symbol != TXT_sentinel_symbol ()) &&
	       !isspace (symbol))
	  {
	    TXT_append_symbol (word, symbol);
	    p++;
	  }
      }

    *pos = p;
    return (TRUE);
}

boolean
TXT_gettag_text (unsigned int text, unsigned int non_word, unsigned int word,
		  unsigned int *tag, unsigned int *pos)
/* Reads the non-word, word and tag (i.e. part of speech) from the specified
   text. A word is defined as any continuous sequence of symbols up until
   the next tag symbol, a non-word any continuous sequence until the next
   appearance of a alphanumeric symbol and a tag symbol is any symbol
   whose value is greater than the maximum ASCII symbol value (i.e. > 256).
   The argument pos is set to the updated position in the text. Returns FALSE
   when no more text exists. */
{
    unsigned int textlen, symbol, p;

    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    assert (TXT_valid_text (non_word));
    TXT_setlength_text (non_word, 0);

    assert (TXT_valid_text (word));
    TXT_setlength_text (word, 0);

    p = *pos;

    /* scan in non-word */
    while (TXT_get_symbol (text, p, &symbol) &&
	   TXT_is_ascii (symbol) && !isalnum (symbol))
      {
        TXT_append_symbol (non_word, symbol);
	p++;
      }

    /* scan in word */
    if ((p < textlen) && (symbol != TXT_sentinel_symbol ()))
      {
        TXT_append_symbol (word, symbol);
	p++;
	while (TXT_get_symbol (text, p, &symbol) &&
	       (symbol != TXT_sentinel_symbol ()) &&
	       TXT_is_ascii (symbol))
	  {
	    TXT_append_symbol (word, symbol);
	    p++;
	  }
      }

    /* get the tag */
    if ((p < textlen) && (symbol != TXT_sentinel_symbol ()))
      {
	*tag = symbol;
	p++;
      }

    *pos = p;
    return ((p < textlen) && (symbol != TXT_sentinel_symbol ()));
}

unsigned int
TXT_load_text (unsigned int file)
/* Creates a new text record and text number, then loads it using text from
   the file. Assumes standard ASCII text. */
{
    unsigned int text;
    int symbol;

    assert (TXT_valid_file (file));

    text = TXT_create_text (); /* remember to reset all textp's past here because of realloc */

    while ((symbol = fgetc (Files [file])) != EOF)
	TXT_append_symbol (text, symbol);
    TXT_append_symbol (text, TXT_sentinel_symbol ());

    return (text);
}

unsigned int
TXT_load_numbers (unsigned int file)
/* Creates a new text record and text number, then loads it using
   the sequence of unsigned numbers from the file. */
{
    unsigned int text, symbol;
    int result;

    assert (TXT_valid_file (file));

    text = TXT_create_text (); /* remember to reset all textp's past here because of realloc */

    for (;;)
      { /* Repeat until EOF */
        result = fscanf (Files [file], "%u", &symbol);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    TXT_append_symbol (text, TXT_sentinel_symbol ());
	    return (text);
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    exit (1);
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	    break;
	  }
	TXT_append_symbol (text, symbol);
      }

    return (text);
}

unsigned int
TXT_load_file (unsigned int file)
/* Creates a new text record and text number, then loads it using text from
   the file. The text is ASCII text except for the "\". This signifies a symbol
   number to follow (a sequence of numeric characters) up until the next \
   is found. The respective symbol number is substituted into the text.
   e.g. "standard text with a symbol number \258\ inside it". 
   If \\ is found, then this is replaced with a single \. */
{
    unsigned int text;
    char nstr [32];
    int nlen, symbol;

    assert (TXT_valid_file (file));

    text = TXT_create_text (); /* remember to reset all textp's past here because of realloc */

    while ((symbol = fgetc (Files [file])) != EOF)
      {
	if (symbol != '\\')
	    TXT_append_symbol (text, symbol);
	else
	  {
	    nlen = 0;
	    while (((symbol = fgetc (Files [file])) != EOF) && (symbol != '\\'))
	      {
		if (nlen < 32)
		  nstr [nlen++] = symbol;
	      }
	    if (!nlen)
	        TXT_append_symbol (text, '\\');
	    else
	      {
		nstr [nlen] = '\0';
		symbol = atoi (nstr);
		TXT_append_symbol (text, symbol);
	      }
	  }
      }
    TXT_append_symbol (text, TXT_sentinel_symbol ());

    return (text);
}

void
TXT_load_filetext (unsigned int file, unsigned int text)
/* Loads the text record using text from the file. The first integer loaded from the file
   signifies the length of the text. The text is ASCII text except for the "\".
   This signifies a symbol number to follow (a sequence of numeric characters) up until the next \
   is found. The respective symbol number is substituted into the text. e.g. "standard text with
   a symbol number \258\ inside it".  If \\ is found, then this is replaced with a single \. */
{
    unsigned int textlen;
    char nstr [32];
    int nlen, symbol;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    TXT_setlength_text (text, 0);

    /* Read in the length of the text */
    textlen = fread_int (file, INT_SIZE);
    while ((TXT_length_text (text) < textlen) && (symbol = fgetc (Files [file])) != EOF)
      {
	if (symbol != '\\')
	    TXT_append_symbol (text, symbol);
	else
	  {
	    nlen = 0;
	    while (((symbol = fgetc (Files [file])) != EOF) && (symbol != '\\'))
	      {
		if (nlen < 32)
		  nstr [nlen++] = symbol;
	      }
	    if (!nlen)
	        TXT_append_symbol (text, '\\');
	    else
	      {
		nstr [nlen] = '\0';
		symbol = atoi (nstr);
		TXT_append_symbol (text, symbol);
	      }
	  }
      }
}

void
TXT_dump_file (unsigned int file, unsigned int text)
/* Writes out the text to the output file as ASCII if possible,
but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
This output file can then be used for later reloading into a text record
using the routine TXT_load_symbols. */
{
    struct textType *textp;
    unsigned int symbol, p;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    textp = Texts + text;

    if (textp->Text_length == 0)
        return;
    assert (textp->Text != NULL);

    for (p=0; p < textp->Text_length; p++)
    {
	symbol = textp->Text [p];
	if (symbol >= IS_ASCII)
	    fprintf (Files [file], "\\%d\\", symbol);
	else if (symbol == '\\')
	    fprintf (Files [file], "\\\\");
	else
	    fputc (symbol, Files [file]);
    }
}

void
TXT_write_filetext (unsigned int file, unsigned int text)
/* Writes out to the output file the length of the text to be written,
then the text itself. Writes it out as ASCII if possible,
but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
This output file can then be used for later reloading into a text record
using the routine TXT_load_filetext. */
{
    unsigned int textlen;

    textlen = TXT_length_text (text);
    fwrite_int (file, textlen, INT_SIZE);
    TXT_dump_file (file, text);
}

void
TXT_load_symbols (unsigned int file, unsigned int text)
/* Overwrites the text record by loading it using the text symbols
   (4 byte numbers) from the file. */
{
    unsigned int textlen, symbol, p;

    assert (TXT_valid_file (file));

    TXT_setlength_text (text, 0); /* overwrite the previous key */

    textlen = fread_int (file, INT_SIZE);
    assert (textlen < MAX_TEXTLENGTH_ASSERTION);

    for (p = 0; p < textlen; p++)
      {
	symbol = fread_int (file, INT_SIZE);
	TXT_append_symbol (text, symbol);
      }
}

void
TXT_write_symbols (unsigned int file, unsigned int text)
/* Writes out the text symbols (i.e. 4 byte symbol numbers) to the file.
   This output file can then be used for later reloading into a text record
   using the routine TXT_load_symbols. */
{
    unsigned int textlen, symbol, p;

    assert (TXT_valid_file (file));

    assert (TXT_valid_text (text));

    textlen = TXT_length_text (text);
    fwrite_int (file, textlen, INT_SIZE);

    for (p = 0; p < textlen; p++)
      {
	TXT_get_symbol (text, p, &symbol);
	fwrite_int (file, symbol, INT_SIZE);
      }
}

void
TXT_read_text (unsigned int file, unsigned int text, unsigned int max)
/* Reads max symbols from the specified file into an existing text record.
   If the argument max is zero, then all symbols until eof are read. */
{
    unsigned int k;
    int symbol;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));

    k = 0;
    while ((!max || (k < max)) && ((symbol = fgetc (Files [file])) != EOF))
      {
	TXT_append_symbol (text, symbol);
	k++;
      }
}

void
TXT_dump_text (unsigned int file, unsigned int text,
	       void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out a human readable version of the text record to the file.
   The argument dump_symbol_function is a pointer to a function for printing
   symbols. If this is NULL, then each symbol will be printed as an unsigned
   int surrounded by angle brackets (e.g. <123>), unless it is human readable
   ASCII, in which case it will be printed as a char. */
{
    struct textType *textp;
    unsigned int symbol, p;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    textp = Texts + text;

    if (textp->Text_length == 0)
        return;
    assert (textp->Text != NULL);

    for (p=0; p < textp->Text_length; p++)
    {
	symbol = textp->Text [p];
	if (dump_symbol_function)
	    dump_symbol_function (file, symbol);
	else if (symbol == TXT_sentinel_symbol ())
	  fprintf (Files [file], "<sentinel>");
	else if (symbol == '\n')
	    fprintf (Files [file], "\n");
	else if (symbol == '\t')
	    fprintf (Files [file], "\t");
	else if ((symbol <= 31) || (symbol >= 127))
	    fprintf (Files [file], "<%d>", symbol);
	else
	    fprintf (Files [file], "%c", symbol);
    }
}

void
TXT_dump_text1 (unsigned int file, unsigned int text, unsigned int pos,
		void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out a human readable version of the text record to the file
   starting at position pos. */
{
    struct textType *textp;
    unsigned int symbol, p;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    textp = Texts + text;

    if (textp->Text_length == 0)
        return;
    assert (textp->Text != NULL);

    for (p=pos; p < textp->Text_length; p++)
    {
	symbol = textp->Text [p];

	if (dump_symbol_function)
	    dump_symbol_function (file, symbol);	
	else if (symbol == '\n')
	    fprintf (Files [file], "\n");
	else if (symbol == '\t')
	    fprintf (Files [file], "\t");
	else if ((symbol <= 31) || (symbol >= 127))
	    fprintf (Files [file], "<%d>", symbol);
	else
	    fprintf (Files [file], "%c", symbol);

	/*fprintf (Files [file], " (%d) ", textp->Text_length);*/

    }
}

void
TXT_dump_text2 (unsigned int file, unsigned int text, unsigned int pos,
		unsigned int len,
		void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out a human readable version of the text record to the file
   starting at position pos of length len. */
{
    struct textType *textp;
    unsigned int symbol, p;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    textp = Texts + text;

    if (textp->Text_length == 0)
        return;
    assert (textp->Text != NULL);

    for (p=pos; p < pos + len; p++)
    {
	symbol = textp->Text [p];

	if (dump_symbol_function)
	    dump_symbol_function (file, symbol);	
	else if (symbol == '\n')
	    fprintf (Files [file], "\n");
	else if (symbol == '\t')
	    fprintf (Files [file], "\t");
	else if ((symbol <= 31) || (symbol >= 127))
	    fprintf (Files [file], "<%d>", symbol);
	else
	    fprintf (Files [file], "%c", symbol);

	/*fprintf (Files [file], " (%d) ", textp->Text_length);*/

    }
}

void
TXT_dump_word (unsigned int file, unsigned int text, unsigned int pos,
	       void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out a human readable version of the next word in the text record to
   the file starting at position pos. */
{
    struct textType *textp;
    unsigned int symbol, p;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (text));
    textp = Texts + text;

    if (textp->Text_length == 0)
        return;
    assert (textp->Text != NULL);

    for (p=pos; p < textp->Text_length; p++)
    {
	symbol = textp->Text [p];
	if (symbol == ' ')
	    break;

	if (dump_symbol_function)
	    dump_symbol_function (file, symbol);	
	else if (symbol == '\n')
	    fprintf (Files [file], "\n");
	else if (symbol == '\t')
	    fprintf (Files [file], "\t");
	else if ((symbol <= 31) || (symbol >= 127))
	    fprintf (Files [file], "<%d>", symbol);
	else
	    fprintf (Files [file], "%c", symbol);

	/*fprintf (Files [file], " (%d) ", textp->Text_length);*/

    }
}
