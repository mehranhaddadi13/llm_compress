/* Text module definitions. */

#include <ctype.h>
#include "model.h"


#ifndef TEXT_H
#define TEXT_H

#define IS_ASCII 128
#define IS_SPACE 32

#define MAX_SYMBOL 2147483645
/* Maximum positive integer (= 2^31-3) 
   (The only reason that it has to be positive is that the implementation of
   PPM models uses the sign bit to flag that the end of a trie's symbol list
   has been reached.) */

#define SENTINEL_SYMBOL (MAX_SYMBOL+1)
/* This number is used to denote the sentinel symbol. */

#define SPECIAL_SYMBOL (MAX_SYMBOL+2)
/* This number is used in special cases e.g. to indicate end of a symbol list
   when the symbol is zero (and therefore cannot be negative) */

/* Global variable used for storing the latest input symbol. */
extern unsigned int TXT_Input_Symbol;

void
TXT_write_symbol (unsigned int file, unsigned int symbol);
/* Writes the ASCII symbol out in human readable form. */

void
TXT_dump_symbol (unsigned int file, unsigned int symbol);
/* Writes the ASCII symbol out in human readable form (excluding '\n' and '\t's). */

void
TXT_dump_symbol1 (unsigned int file, unsigned int symbol);
/* Writes the ASCII symbol out in human readable form (excluding white space). */

void
TXT_dump_symbol2 (unsigned int file, unsigned int symbol);
/* Writes the ASCII symbol out in human readable form (including '\n' and '\t's). */

boolean
TXT_is_ascii (unsigned int symbol);
/* Returns TRUE if symbol is an ASCII character. */

boolean
TXT_is_vowel (unsigned int symbol);
/* Returns TRUE if symbol is a vowel. */

boolean
TXT_is_consonant (unsigned int symbol);
/* Returns TRUE if symbol is a consonant. */

boolean
TXT_is_alphanumeric (unsigned int symbol);
/* Returns TRUE if symbol is an alphanumeric character. */

boolean
TXT_is_alpha (unsigned int symbol);
/* Returns TRUE if symbol is an alphabetic character. */

boolean
TXT_is_control (unsigned int symbol);
/* Returns TRUE if symbol is a control character. */

boolean
TXT_is_digit (unsigned int symbol);
/* Returns TRUE if symbol is a digit. */

boolean
TXT_is_graph (unsigned int symbol);
/* Returns TRUE if symbol is a printable character except space. */

boolean
TXT_is_lower (unsigned int symbol);
/* Returns TRUE if symbol is a uppercace character. */

unsigned int
TXT_to_lower (unsigned int symbol);
/* Returns the lower case of symbol. */

boolean
TXT_is_print (unsigned int symbol);
/* Returns TRUE if symbol is a printable character. */

boolean
TXT_is_punct (unsigned int symbol);
/* Returns TRUE if symbol is a punctuation character. */

boolean
TXT_is_space (unsigned int symbol);
/* Returns TRUE if symbol is a white space character. */
 
boolean
TXT_is_upper (unsigned int symbol);
/* Returns TRUE if symbol is a uppercace character. */

unsigned int
TXT_to_upper (unsigned int symbol);
/* Returns the upper case of symbol. */

boolean
TXT_valid_text (unsigned int text);
/* Returns non-zero if the text is valid, zero otherwize. */

unsigned int
TXT_create_text (void);
/* Creates a text record. */

void
TXT_release_text (unsigned int text);
/* Releases the memory allocated to the text and the text number (which may
   be reused in later TXT_create_text calls). */

int
TXT_compare_text (unsigned int text1, unsigned int text2);
/* Compares text1 with text2. Returns zero if they are the same,
   negative if text1 < text2, and positive if text1 > text2.
   The sentinel symbol returned by TLM_sentinel_symbol is
   regarded as having a value lower than all other symbols. */

int
TXT_strcmp_text (unsigned int text, char *string);
/* Compares the text with the string. Returns zero if they are the same,
   negative if text < string, and positive if text > string. */

boolean
TXT_null_text (unsigned int text);
/* Returns TRUE if the text is empty. */

unsigned int
TXT_sentinel_symbol (void);
/* Returns an unsigned integer that uniquely identifies a special ``sentinel''
   symbol */

boolean
TXT_sentinel_text (unsigned int text);
/* Returns TRUE if the text contains just a sentinel symbol. */

unsigned int
TXT_sentinel1_symbol (void);
/* Returns an unsigned integer that uniquely identifies another special ``sentinel1''
   symbol. */

unsigned int
TXT_createsentinel_text (void);
/* Returns text that contains just a sentinel symbol. */

unsigned int
TXT_alloc_text (unsigned int text);
/* Returns the allocation of the text (it's memory use; for debugging purpose). */

unsigned int
TXT_length_text (unsigned int text);
/* Returns the length of the text. Assumes the text is non-NIL. */

unsigned int
TXT_length_text1 (unsigned int text);
/* Returns the length of the text. Returns 0 if the text is NIL. */

unsigned int
TXT_setlength_text (unsigned int text, unsigned int length);
/* Sets the length of the text to be at most length symbols long. If the
   current length of the text is longer than this, then the text will be
   truncated to the required length, otherwise the length will remain
   unchanged. Setting the length of the text to be 0 will set the text
   to the null string. */

boolean
TXT_getpos_text (unsigned int text, unsigned int symbol, unsigned int *pos);
/* Returns TRUE if the symbol is found in the text. The argument pos is set
   to the position of the first symbol in the text that matches the specified
   symbol if found, otherwise it remains unchanged. */

boolean
TXT_getrpos_text (unsigned int text, unsigned int symbol, unsigned int *pos);
/* Returns TRUE if the symbol is found in the text. The argument pos is set to
   the position of the last symbol in the text that matches the specified
   symbol if found, otherwise it remains unchanged. */

void
TXT_string_text (unsigned int text, char *string, unsigned int max);
/* Returns as ASCII version of the text in string. */

boolean
TXT_getstr_text (unsigned int text, char *string, unsigned int *pos);
/* Returns TRUE if the string is found in the text. The argument pos is set
   to the position of the first symbol in the text starting from position
   0 that matches the specified string if found, otherwise it remains
   unchanged. */

boolean
TXT_getstring_text (unsigned int text, char *string, unsigned int *pos);
/* Returns TRUE if the string is found in the text. The argument pos contains
   the starting point and gets set to the position of the first point
   in the following text that matches the specified string if found,
   otherwise it remains unchanged. */

boolean
TXT_getsymbol_text (unsigned int text, unsigned int pos);
/* Returns the symbol at position in the text. A run-time assertion error
   occurs if pos is greater then the length of the text. */

boolean
TXT_getsymbol_file (unsigned int file, boolean load_numbers);
/* Gets the next symbol (in the global variable TXT_Input_Symbol) from input
   stream FILE. Returns FALSE when there are no more symbols (EOF has been reached).
   If load_numbers is TRUE, then numbers will be scanned instead of characters. */

boolean
TXT_get_symbol (unsigned int text, unsigned int pos, unsigned int *symbol);
/* Returns TRUE if there exists a symbol at position pos in the text. 
   The argument symbol is set to the specified symbol. */

void
TXT_put_symbol (unsigned int text, unsigned int symbol, unsigned int pos);
/* Inserts the symbol at position pos into the text. Inserting a symbol beyond
   the current bounds of the text will cause a run-time error. */

boolean
TXT_find_symbol (unsigned int text, unsigned int symbol, unsigned int start_pos,
		 unsigned int *pos);
/* Returns TRUE if the symbol exists in the text starting from start_pos. 
   The argument \verb|pos| us set to the symbol's position. */

boolean
TXT_find_text (unsigned int text, unsigned int subtext, unsigned int *pos);
/* Returns TRUE if the subtext is found in the text. The argument pos is set
   to the position of the first symbol in the text starting from position
   0 that matches the specified string if found, otherwise it remains
   unchanged. */

void
TXT_append_symbol (unsigned int text, unsigned int symbol);
/* Appends the symbol onto the end of the text. */

void
TXT_append_string (unsigned int text, char *string);
/* Appends the string onto the end of the text. */

boolean
TXT_find_string (unsigned int text, char *string, unsigned int start_pos,
		 unsigned int *pos);
/* Returns TRUE if the string exists in the text starting from start_pos. 
   The argument \verb|pos| us set to the string's position. */

void
TXT_nullify_string (unsigned int text, char *string);
/* Replaces all occurences of string in the text with nulls. (This can be used for
   "removeing" the string if using TXT_dump_symbol2 to dump it out. */

void
TXT_overwrite_string (unsigned int text, char *string, char *new_string);
/* Overwrites all occurences of string in the text with new_string. Appends
   nulls (or truncates) if there is a mis-match in lengths of the string. */

void
TXT_sprintf_text (unsigned int text, char *format, ...);
/* Sets the text to the symbols specified by the format and variable length
   argument list. Meaning of the formatting characters:

         %%    - the % (percentage) character is inserted into the text.
         %s    - the argument list contains a symbol number (unsigned int)
	         which will be inserted into the text.
	 %d    - the argument list contains an unsigned int which will be
	         written into the text.
 */

void
TXT_scanf_text (unsigned int text, char *format, ...);
/* Scans in the text the arguments specified by the variable length
   argument list in the format. Meaning of the formatting characters:

	 %d    - the argument list contains a single unsigned int
	         which will be scanned from the text.
	 %f    - the argument list contains a single float
	         which will be scanned from the text.
 */

void
TXT_extract_text (unsigned int text, unsigned int subtext,
		  unsigned int subtext_pos, unsigned int subtext_len);
/* Extracts the text from out of the text record. The argument subtext is set
   to the extracted text; subtext_len is the length of the text to be
   extracted; and subtext_pos is the position from which the text should be
   extracted from. The extracted subtext is filled with nulls for any part of
   it that extends beyond the bounds of the text record. */

void
TXT_extractstring_text (unsigned int text, char *string,
			unsigned int subtext_pos, unsigned int subtext_len);
/* Returns as ASCII version of the extracted text in string. */

unsigned int
TXT_copy_text (unsigned int text);
/* Creates a new text record and text number, then copies the text record into
   it. */

void
TXT_overwrite_text (unsigned int text, unsigned int text1);
/* Overwrites the text with a copy of text1. */

void
TXT_append_text (unsigned int text, unsigned int text1);
/* Appends the text1 onto the end of text. */

int
TXT_readline_text (unsigned int file, unsigned int text);
/* Loads the text using the next line of input from the file. Returns the
   last character read or EOF. */

int
TXT_readline_text1 (unsigned int file, unsigned int text);
/* Loads the text using the next line of input from the file. Returns the
   last character read or EOF. Does not skip over \r characters. */

int
TXT_readline_text2 (unsigned int file, unsigned int text);
/* Loads the text using the next line of input from the file. Returns the
   last character read or EOF. Does not skip over \r or \n characters. */

boolean
TXT_getline_text (unsigned int text, unsigned int line,
		  unsigned int *pos);
/* Reads the next line from the specified text.
   The argument pos is set to the updated position
   in the text. Returns FALSE when no more text exists. */

boolean
TXT_readword_text (unsigned int file, unsigned int non_word,
		   unsigned int word);
/* Reads the non-word and word from the specified file. A word is
   defined as any continuous alphanumeric sequence, and a non-word
   anything in between. Returns FALSE when EOF occurs. */

boolean
TXT_readword_text1 (unsigned int file, unsigned int non_word,
		    unsigned int word);
/* Same as TXT_readword_text () except replaces null words with the sentinel word. */

boolean
TXT_getword_text (unsigned int text, unsigned int non_word, unsigned int word,
		  unsigned int *text_pos, unsigned int *nonword_text_pos,
		  unsigned int *word_text_pos);
/* Reads the non-word and word from the specified text. A word is
   defined as any continuous alphanumeric sequence, and a non-word
   anything in between. The argument pos is set to the updated position
   in the text. Returns FALSE when no more text exists. */

boolean
TXT_getword_text1 (unsigned int text, unsigned int non_word, unsigned int word,
		   unsigned int *text_pos, unsigned int *nonword_text_pos,
		   unsigned int *word_text_pos);
/* Same as TXT_getword_text () except replaces null words with the sentinel word. */

boolean
TXT_readword1_text (unsigned int file, unsigned int word);
/* Reads the word from the specified file. A word is
   defined as any continuous non-whitespace sequence.
   Returns FALSE when EOF occurs. */

boolean
TXT_getword1_text (unsigned int text, unsigned int word,
		  unsigned int *pos);
/* Reads the word from the specified text. A word is
   defined as any continuous non-whitespace sequence.
   The argument pos is set to the updated position
   in the text. Returns FALSE when no more text exists. */

boolean
TXT_gettag_text (unsigned int text, unsigned int non_word, unsigned int word,
		  unsigned int *tag, unsigned int *pos);
/* Reads the non-word, word and tag (i.e. part of speech) from the specified
   text. A word is defined as any continuous sequence of symbols up until
   the next tag symbol, a non-word any continuous sequence until the next
   appearance of a alphanumeric symbol and a tag symbol is any symbol
   whose value is greater than the maximum ASCII symbol value (i.e. > 256).
   The argument pos is set to the updated position in the text. Returns FALSE
   when no more text exists. */

unsigned int
TXT_load_text (unsigned int file);
/* Creates a new text record and text number, then loads it using text from
   the file. Assumes standard ASCII text. */

unsigned int
TXT_load_numbers (unsigned int file);
/* Creates a new text record and text number, then loads it using
   the sequence of unsigned numbers from the file. */

unsigned int
TXT_load_file (unsigned int file);
/* Creates a new text record and text number, then loads it using text from
   the file. The text is ASCII text except for the "\". This signifies a symbol
   number to follow (a sequence of numeric characters) up until the next \ is
   found. The respective symbol number is substituted into the text. 
   e.g. "standard text with a symbol number \258\ inside it". 
   If \\ is found, then this is replaced with a single \. */

void
TXT_load_filetext (unsigned int file, unsigned int text);
/* Loads the text record using text from the file. The first integer loaded from the file
   signifies the length of the text. The text is ASCII text except for the "\".
   This signifies a symbol number to follow (a sequence of numeric characters) up until the next \
   is found. The respective symbol number is substituted into the text. e.g. "standard text with
   a symbol number \258\ inside it".  If \\ is found, then this is replaced with a single \. */

void
TXT_dump_file (unsigned int file, unsigned int text);
/* Writes out the text to the output file as ASCII if possible,
but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
This output file can then be used for later reloading into a text record
using the routine TXT_load_symbols. */

void
TXT_write_filetext (unsigned int file, unsigned int text);
/* Writes out to the output file the length of the text to be written,
then the text itself. Writes it out as ASCII if possible,
but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
This output file can then be used for later reloading into a text record
using the routine TXT_load_filetext. */

void
TXT_load_symbols (unsigned int file, unsigned int text);
/* Overwrites the text record by loading it using the text symbols
   (4 byte numbers) from the file. */

void
TXT_write_symbols (unsigned int file, unsigned int text);
/* Writes out the text symbols (i.e. 4 byte symbol numbers) to the file.
   This output file can then be used for later reloading into a text record
   using the routine TXT_load_symbols. */

void
TXT_read_text (unsigned int file, unsigned int text, unsigned int max);
/* Reads max symbols from the specified file into an existing text record.
   If the argument max is zero, then all symbols until eof are read. */

void
TXT_dump_text (unsigned int file, unsigned int text,
	       void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out a human readable version of the text record to the file. The
   argument dump_symbol_function is a pointer to a function for printing
   symbols. If this is NULL, then each symbol will be printed as an
   unsigned int surrounded by angle brackets (e.g. <123>), unless it is
   human readable ASCII, in which case it will be printed as a char. */

void
TXT_dump_text1 (unsigned int file, unsigned int text, unsigned int pos,
		void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out a human readable version of the text record to the file
   starting at position pos. */

void
TXT_dump_text2 (unsigned int file, unsigned int text, unsigned int pos,
		unsigned int len,
		void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out a human readable version of the text record to the file
   starting at position pos of length len. */

void
TXT_dump_word (unsigned int file, unsigned int text, unsigned int pos,
	       void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out a human readable version of the next word in the text record to
   the file starting at position pos. */

#endif
