import ctypes

c_uint = ctypes.c_uint
c_char_p = ctypes.c_char_p
c_void_p = ctypes.c_void_p

libTawa = ctypes.CDLL('pyTawa.so')

Stdin_File = 1   # File associated with stdin
Stdout_File = 2  # File associated with stdout
Stderr_File = 3  # File associated with stderr

# The following constants define the operations for TXT_set_file ():
File_Exit_On_Errors = 1  # Exits with a run-time error when a file error has been encountered
File_Ignore_Errors = 2   # Inores any file errors

def TXT_init_files ():
    """ Initialises the Files array and Stdin_File, Stdout_File and Stderr_Files. """
    libTawa.TXT_init_files.restype = c_void_p
    libTawa.TXT_init_files ()

def TXT_valid_file (fileid):
    """ Returns True if the file is valid, False otherwize. """
    libTawa.TXT_valid_file.argtypes=[c_uint]
    return (bool(libTawa.TXT_valid_file (fileid)))

def TXT_set_file (operation):
    """ Sets various flags for the file functions

    File_Exit_On_Errors 1  (Default) Exits with a run-time error when a
				     file error has been encountered
    File_Ignore_Errors 2             Inores any file errors
    """
    libTawa.TXT_set_file.argtypes=[c_uint]
    libTawa.TXT_set_file (operation)

def TXT_open_file (filename, mode, debug_line, error_line):
    """ Opens the file with the specified mode. Returns an unsigned integer
    as a reference to the file pointer.

    Each of the arguments must be byte-encoded strings e.g. b"filename". """
    libTawa.TXT_open_file.restype=c_uint
    libTawa.TXT_open_file.argtypes=[c_char_p,c_char_p,c_char_p,c_char_p]
    return (libTawa.TXT_open_file (filename, mode, debug_line, error_line))

def TXT_close_file (fileid):
    """ Opens the file with the specified mode. Returns an unsigned integer
    as a reference to the file pointer. """
    libTawa.TXT_close_file.restype = c_void_p
    libTawa.TXT_close_file.argtypes=[c_uint]
    libTawa.TXT_close_file (fileid)

def TXT_write_file (fileid, string):
    """ Writes out the string to the file. """
    libTawa.TXT_write_file.restype= c_void_p
    libTawa.TXT_write_file.argtypes=[c_uint,c_char_p]
    libTawa.TXT_write_file (fileid, string)

def TXT_write_symbol (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form. """
    libTawa.TXT_write_symbol.restype= c_void_p
    libTawa.TXT_write_symbol.argtypes=[c_uint,c_uint]
    libTawa.TXT_write_symbol (fileid, symbol)

def TXT_dump_symbol (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form (excluding '\n' and '\t's). """
    libTawa.TXT_dump_symbol.restype= c_void_p
    libTawa.TXT_dump_symbol.argtypes=[c_uint,c_uint]
    libTawa.TXT_dump_symbol (fileid, symbol)

def TXT_dump_symbol1 (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form (excluding white space). """
    libTawa.TXT_dump_symbol1.restype= c_void_p
    libTawa.TXT_dump_symbol1.argtypes=[c_uint,c_uint]
    libTawa.TXT_dump_symbol1 (fileid, symbol)

def TXT_dump_symbol2 (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form (including '\n' and '\t's). """
    libTawa.TXT_dump_symbol2.restype= c_void_p
    libTawa.TXT_dump_symbol2.argtypes=[c_uint,c_uint]
    libTawa.TXT_dump_symbol2 (fileid, symbol)

def TXT_is_ascii (symbol):
    """ Returns True if symbol is an ASCII character. """
    libTawa.TXT_is_ascii.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_ascii (symbol)))

def TXT_is_vowel (symbol):
    """ Returns True if symbol is a vowel. """
    libTawa.TXT_is_vowel.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_vowel (symbol)))

def TXT_is_consonant (symbol):
    libTawa.TXT_is_consonant.argtypes=[c_uint]
    """ Returns True if symbol is a consonant. """
    return (bool(libTawa.TXT_is_consonant (symbol)))

def TXT_is_alphanumeric (symbol):
    """ Returns True if symbol is an alphanumeric character. """
    libTawa.TXT_is_alphanumeric.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_alphanumeric (symbol)))

def TXT_is_alpha (symbol):
    """ Returns True if symbol is an alphabetic character. """
    libTawa.TXT_is_alpha.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_alpha (symbol)))

def TXT_is_control (symbol):
    """ Returns True if symbol is a control character. """
    libTawa.TXT_is_control.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_control (symbol)))

def TXT_is_digit (symbol):
    """ Returns True if symbol is a digit. """
    libTawa.TXT_is_digit.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_digit (symbol)))

def TXT_is_graph (symbol):
    """ Returns True if symbol is a printable character except space. """
    libTawa.TXT_is_graph.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_graph(symbol)))

def TXT_is_lower (symbol):
    """ Returns True if symbol is a uppercase character. """
    libTawa.TXT_is_lower.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_lower (symbol)))

def TXT_to_lower (symbol):
    """ Returns the lower case of symbol. """
    libTawa.TXT_to_lower.restype=c_uint
    libTawa.TXT_to_lower.argtypes=[c_uint]
    return (libTawa.TXT_to_lower (symbol))

def TXT_is_print (symbol):
    """ Returns True if symbol is a printable character. """
    libTawa.TXT_is_print.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_print (symbol)))

def TXT_is_punct (symbol):
    """ Returns True if symbol is a punctuation character. """
    libTawa.TXT_is_punct.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_punct (symbol)))

def TXT_is_space (symbol):
    """ Returns True if symbol is a white space character. """
    libTawa.TXT_is_space.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_space (symbol)))
 
def TXT_is_upper (symbol):
    """ Returns True if symbol is a uppercace character. """
    libTawa.TXT_is_upper.argtypes=[c_uint]
    return (bool(libTawa.TXT_is_upper (symbol)))

def TXT_to_upper (symbol):
    """ Returns the upper case of symbol. """
    libTawa.TXT_to_upper.restype=c_uint
    libTawa.TXT_to_upper.argtypes=[c_uint]
    return (libTawa.TXT_to_upper (symbol))

def TXT_valid_text (text):
    """ Returns non-zero if the text is valid, zero otherwize. """
    libTawa.TXT_valid_text.argtypes=[c_uint]
    return (bool(libTawa.TXT_valid_text (text)))

def TXT_create_text ():
    """ Creates a text record. """
    libTawa.TXT_create_text.restype=c_uint
    return (libTawa.TXT_create_text ())

def TXT_release_text (text):
    """ Releases the memory allocated to the text and the text number (which may
    be reused in later TXT_create_text calls). """
    libTawa.TXT_release_text.restype=c_void_p
    libTawa.TXT_release_text.argtypes=[c_uint]
    return (libTawa.TXT_release_text (text))

def TXT_compare_text (text1, text2):
    """ Compares text1 with text2. Returns zero if they are the same,
    negative if text1 < text2, and positive if text1 > text2.
    The sentinel symbol returned by TLM_sentinel_symbol is
    regarded as having a value lower than all other symbols. """
    libTawa.TXT_compare_text.restype=c_uint
    libTawa.TXT_compare_text.argtypes=[c_uint,c_uint]
    return (libTawa.TXT_compare_text (text1, text2))

def TXT_strcmp_text (text, string):
    """ Compares the text with the string. Returns zero if they are the same,
    negative if text < string, and positive if text > string. """
    libTawa.TXT_strcmp_text.restype=c_uint
    libTawa.TXT_strcmp_text.argtypes=[c_uint,c_char_p]
    return (libTawa.TXT_strcmp_text (string))

def TXT_null_text (text):
    """ Returns True if the text is empty. """
    libTawa.TXT_null_text.argtypes=[c_uint]
    return (bool(libTawa.TXT_null_text (text)))

def TXT_sentinel_symbol ():
    """ Returns an unsigned integer that uniquely identifies a special ``sentinel''
    symbol """
    libTawa.TXT_sentinel_symbol.restype=c_uint
    return (libTawa.TXT_sentinel_symbol ())

def TXT_sentinel_text (text):
    """ Returns True if the text contains just a sentinel symbol. """
    libTawa.TXT_sentinel_symbol.argtypes=[c_uint]
    return (bool(libTawa.TXT_sentinel_text (text)))

def TXT_createsentinel_text ():
    """ Returns text that contains just a sentinel symbol. """
    libTawa.TXT_createsentinel_text.restype=c_void_p
    return (libTawa.TXT_createsentinel_text ())

def TXT_alloc_text (text):
    """ Returns the allocation of the text (it's memory use; for debugging purpose). """
    libTawa.TXT_alloc_text.restype=c_uint
    libTawa.TXT_alloc_text.argtypes=[c_uint]
    return (libTawa.TXT_alloc_text (text))

def TXT_length_text (text):
    """ Returns the length of the text. Assumes the text is non-NIL. """
    libTawa.TXT_length_text.restype=c_uint
    libTawa.TXT_length_text.argtypes=[c_uint]
    return (libTawa.TXT_length_text (text))

def TXT_length_text1 (text):
    """ Returns the length of the text. Returns 0 if the text is NIL. """
    libTawa.TXT_length_text1.restype=c_uint
    libTawa.TXT_length_text1.argtypes=[c_uint]
    return (libTawa.TXT_length_text1 (text))

def TXT_setlength_text (text, length):
    """ Sets the length of the text to be at most length symbols long. If the
    current length of the text is longer than this, then the text will be
    truncated to the required length, otherwise the length will remain
    unchanged. Setting the length of the text to be 0 will set the text
    to the null string. """
    libTawa.TXT_setlength_text.restype=c_void_p
    libTawa.TXT_setlength_text.argtypes=[c_uint,c_uint]
    libTawa.TXT_setlength_text (text)

def TXT_string_text (text, string, maxchars):
    """ Returns an ASCII version of the text in string. """
    libTawa.TXT_string_text.restype=c_char_p
    libTawa.TXT_string_text.argtypes=[c_uint,c_char_p,c_uint]
    return (libTawa.TXT_string_text (text, string, maxchars))

def TXT_getsymbol_text (text, pos):
    """ Returns the symbol at position in the text. A run-time assertion error       
    occurs if pos is greater then the length of the text. """
    libTawa.TXT_getsymbol_text.restype=c_uint
    libTawa.TXT_getsymbol_text.argtypes=[c_uint,c_uint]
    return (libTawa.TXT_getsymbol_text (text, pos))

def TXT_input_symbol():
    return ctypes.c_uint.in_dll(libTawa, "TXT_Input_Symbol").value

def TXT_getsymbol_file (fileid, load_numbers):
    """ Gets the next symbol (returned by TXT_input_symbol ()) from
    input stream FILE. Returns False when there are no more symbols (EOF has
    been reached). If load_numbers is True, then numbers will be scanned
    instead of characters. """
    libTawa.TXT_getsymbol_file.argtypes=[c_uint,c_uint]
    return (bool (libTawa.TXT_getsymbol_file (fileid, load_numbers)))

def TXT_put_symbol (text, symbol, pos):
    """ Inserts the symbol at position pos into the text. Inserting a symbol beyond
    the current bounds of the text will cause a run-time error. """
    libTawa.TXT_put_symbol.restype=c_void_p
    libTawa.TXT_put_symbol.argtypes=[c_uint,c_uint,c_uint]
    return (libTawa.TXT_put_symbol (text, pos, symbol))

def TXT_find_symbol (text, symbol, start_pos, pos):
    """ Returns True if the symbol exists in the text starting from start_pos. 
    The argument pos is set to the symbol's position. """
    libTawa.TXT_find_symbol.argtypes=[c_uint,c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_find_symbol (text, symbol, start_pos, pos)))

def TXT_append_symbol (text, symbol):
    """ Appends the symbol onto the end of the text. """
    libTawa.TXT_append_symbol.restype=c_void_p
    libTawa.TXT_append_symbol.argtypes=[c_uint,c_uint]
    libTawa.TXT_append_symbol (text, symbol)

def TXT_append_string (text, string):
    """ Appends the string onto the end of the text. """
    libTawa.TXT_append_string.restype=c_void_p
    libTawa.TXT_append_string.argtypes=[c_uint,c_char_p]
    libTawa.TXT_append_string (text, string)

def TXT_find_string (text, string, start_pos, pos):
    """ Returns True if the string exists in the text starting from start_pos. 
    The argument pos is set to the string's position. """
    libTawa.TXT_find_string.argtypes=[c_uint,c_char_p,c_uint,c_uint]
    return (bool(libTawa.TXT_find_string (text, string, start_pos, pos)))

def TXT_nullify_string (text, string):
    """ Replaces all occurences of string in the text with nulls. (This can be used for
    "removing" the string if using TXT_dump_symbol2 to dump it out. """
    libTawa.TXT_nullify_string.restype=c_void_p
    libTawa.TXT_bullify_string.argtypes=[c_uint,c_char_p]
    libTawa.TXT_nullify_string (text, string)

def TXT_overwrite_string (text, string, new_string):
    """ Overwrites all occurences of string in the text with new_string. Appends
    nulls (or truncates) if there is a mis-match in lengths of the string. """
    libTawa.TXT_overwrite_string.restype=c_void_p
    libTawa.TXT_overwrite_string.argtypes=[c_uint,c_char_p,c_char_p]
    libTawa.TXT_overwrite_string (text, string, new_string)

def TXT_extract_text (text, subtext, subtext_pos, subtext_len):
    """ Extracts the text from out of the text record. The argument subtext is set
    to the extracted text; subtext_len is the length of the text to be
    extracted; and subtext_pos is the position from which the text should be
    extracted from. The extracted subtext is filled with nulls for any part of
    it that extends beyond the bounds of the text record. """
    libTawa.TXT_extract_text.restype=c_void_p
    libTawa.TXT_extract_text.argtypes=[c_uint,c_uint,c_uint,c_uint]
    libTawa.TXT_extract_text (text, subtext, subtext_pos, subtext_len)

def TXT_extractstring_text (text, string, subtext_pos, subtext_len):
    """ Returns as ASCII version of the extracted text in string. """
    libTawa.TXT_extractstring_text.restype=c_char_p
    libTawa.TXT_extractstring_text.argtypes=[c_uint,c_char_p,c_uint,c_uint]
    return (libTawa.TXT_extractstring_text (text, string, subtext_pos, subtext_len))

def TXT_copy_text (text):
    """ Creates a new text record and text number, then copies the text record into
    it. """
    libTawa.TXT_copy_text.restype=c_uint
    libTawa.TXT_copy_text.argtypes=[c_uint]
    libTawa.TXT_copy_text (text)

def TXT_overwrite_text (text, text1):
    """ Overwrites the text with a copy of text1. """
    libTawa.TXT_overwrite_text.restype=c_void_p
    libTawa.TXT_overwrite_text.argtypes=[c_uint,c_uint]
    libTawa.TXT_overwrite_text (text, text1)

def TXT_append_text (text, text1):
    """ Appends the text1 onto the end of text. """
    libTawa.TXT_append_text.restype=c_void_p
    libTawa.TXT_append_text.argtypes=[c_uint,c_uint]
    libTawa.TXT_append_text (text, text1)

def TXT_readline_text (fileid, text):
    """ Loads the text using the next line of input from the file. Returns the
    last character read or EOF. """
    libTawa.TXT_readline_text.restype=c_uint
    libTawa.TXT_readline_text.argtypes=[c_uint,c_uint]
    return (libTawa.TXT_readline_text (fileid, text))

def TXT_readline_text1 (fileid, text):
    """ Loads the text using the next line of input from the file. Returns the
    last character read or EOF. Does not skip over \r characters. """
    libTawa.TXT_readline1_text.restype=c_uint
    libTawa.TXT_readline1_text.argtypes=[c_uint,c_uint]
    return (libTawa.TXT_readline_text1 (fileid, text))

def TXT_readline_text2 (fileid, text):
    """ Loads the text using the next line of input from the file. Returns the
    last character read or EOF. Does not skip over \r or \n characters. """
    libTawa.TXT_readline2_text.restype=c_uint
    libTawa.TXT_readline2_text.argtypes=[c_uint,c_uint]
    return (libTawa.TXT_readline_text2 (fileid, text))

def TXT_getline_text (text, line, pos):
    """ Reads the next line from the specified text.
    The argument pos is set to the updated position
    in the text. Returns FALSE when no more text exists. """
    libTawa.TXT_getline_text.argtypes=[c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_getline_text (text, line, pos)))

def TXT_readword_text (fileid, non_word, word):
    """ Reads the non-word and word from the specified file. A word is
    defined as any continuous alphanumeric sequence, and a non-word
    anything in between. Returns FALSE when EOF occurs. """
    libTawa.TXT_readword_text.argtypes=[c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_readword_text (fileid, non_word, word)))

def TXT_readword_text1 (fileid, non_word, word):
    """ Same as TXT_readword_text () except replaces null words with the sentinel word. """
    libTawa.TXT_readword_text1.argtypes=[c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_readword_text1 (fileid, non_word, word)))

def TXT_getword_text (text, non_word, word, text_pos, nonword_text_pos, word_text_pos):
    """ Reads the non-word and word from the specified text. A word is
    defined as any continuous alphanumeric sequence, and a non-word
    anything in between. The argument pos is set to the updated position
    in the text. Returns FALSE when no more text exists. """
    libTawa.TXT_getword_text.argtypes=[c_uint,c_uint,c_uint,c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_getword_text (text, non_word, word, text_pos, nonword_text_pos, word_text_pos)))

def TXT_getword_text1 (text, non_word, word, text_pos, nonword_text_pos, word_text_pos):
    """ Same as TXT_getword_text () except replaces null words with the sentinel word. """
    libTawa.TXT_getword_text1.argtypes=[c_uint,c_uint,c_uint,c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_getword_text1 (text, non_word, word, text_pos, nonword_text_pos, word_text_pos)))

def TXT_readword1_text (fileid, word):
    """ Reads the word from the specified file. A word is
    defined as any continuous non-whitespace sequence.
    Returns FALSE when EOF occurs. """
    libTawa.TXT_readword1_text.argtypes=[c_uint,c_uint]
    return (bool(libTawa.TXT_readword1_text (fileid, word)))

def TXT_getword1_text (text, word, pos):
    """ Reads the word from the specified text. A word is
    defined as any continuous non-whitespace sequence.
    The argument pos is set to the updated position
    in the text. Returns FALSE when no more text exists. """
    libTawa.TXT_getword1_text.argtypes=[c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_getword1_text (text, word, pos)))

def TXT_gettag_text (text, non_word, word, tag, pos):
    """ Reads the non-word, word and tag (i.e. part of speech) from the specified
    text. A word is defined as any continuous sequence of symbols up until
    the next tag symbol, a non-word any continuous sequence until the next
    appearance of a alphanumeric symbol and a tag symbol is any symbol
    whose value is greater than the maximum ASCII symbol value (i.e. > 256).
    The argument pos is set to the updated position in the text. Returns FALSE
    when no more text exists. """
    libTawa.TXT_gettag_text.argtypes=[c_uint,c_uint,c_uint,c_uint,c_uint]
    return (bool(libTawa.TXT_gettag_text (text, non_word, word, tag, pos)))

def TXT_load_text (fileid):
    """ Creates a new text record and text number, then loads it using text from
    the file. Assumes standard ASCII text. """
    libTawa.TXT_load_text.restype=c_void_p
    libTawa.TXT_load_text.argtypes=[c_uint]
    return (libTawa.TXT_load_text (fileid))

def TXT_load_numbers (fileid):
    """ Creates a new text record and text number, then loads it using
    the sequence of unsigned numbers from the file. """
    libTawa.TXT_load_numbers.restype=c_void_p
    libTawa.TXT_load_numbers.argtypes=[c_uint]
    return (libTawa.TXT_load_numbers (fileid))

def TXT_load_file (fileid):
    """ Creates a new text record and text number, then loads it using text from
    the file. The text is ASCII text except for the "\". This signifies a symbol
    number to follow (a sequence of numeric characters) up until the next \ is
    found. The respective symbol number is substituted into the text. 
    e.g. "standard text with a symbol number \258\ inside it". 
    If \\ is found, then this is replaced with a single \. """
    libTawa.TXT_load_file.restype=c_void_p
    libTawa.TXT_load_file.argtypes=[c_uint]
    return (libTawa.TXT_load_file (fileid))

def TXT_load_filetext (fileid, text):
    """ Loads the text record using text from the file. The first integer loaded from the file
    signifies the length of the text. The text is ASCII text except for the "\".
    This signifies a symbol number to follow (a sequence of numeric characters) up until the next \
    is found. The respective symbol number is substituted into the text. e.g. "standard text with
    a symbol number \258\ inside it".  If \\ is found, then this is replaced with a single \. """
    libTawa.TXT_load_filetext.restype=c_void_p
    libTawa.TXT_load_filetext.argtypes=[c_uint,c_uint]
    libTawa.TXT_load_filetext (fileid, text)

def TXT_dump_file (fileid, text):
    """ Writes out the text to the output file as ASCII if possible,
    but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
    This output file can then be used for later reloading into a text record
    using the routine TXT_load_symbols. """
    libTawa.TXT_dump_file.restype=c_void_p
    libTawa.TXT_dump_file.argtypes=[c_uint,c_uint]
    libTawa.TXT_dump_file (fileid, text)

def TXT_write_filetext (fileid, text):
    """ Writes out to the output file the length of the text to be written,
    then the text itself. Writes it out as ASCII if possible,
    but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
    This output file can then be used for later reloading into a text record
    using the routine TXT_load_filetext. """
    libTawa.TXT_write_filetext.restype=c_void_p
    libTawa.TXT_write_filetext.argtypes=[c_uint,c_uint]
    libTawa.TXT_write_filetext (fileid, text)

def TXT_load_symbols (fileid, text):
    """ Overwrites the text record by loading it using the text symbols
    (4 byte numbers) from the file. """
    libTawa.TXT_load_symbols.restype=c_void_p
    libTawa.TXT_load_symbols.argtypes=[c_uint,c_uint]
    libTawa.TXT_load_symbols (fileid, text)

def TXT_write_symbols (fileid, text):
    """ Writes out the text symbols (i.e. 4 byte symbol numbers) to the file.
    This output file can then be used for later reloading into a text record
    using the routine TXT_load_symbols. """
    libTawa.TXT_write_symbols.restype=c_void_p
    libTawa.TXT_write_symbols.argtypes=[c_uint,c_uint]
    libTawa.TXT_write_symbols (fileid, text)

def TXT_write_string (string):
    """ Creates a new text record, writes string into it, and returns it. """
    libTawa.TXT_write_string.restype=c_uint
    libTawa.TXT_write_string.argtypes=[c_char_p]
    return(libTawa.TXT_write_string (string))
    
def TXT_read_text (fileid, text, maxchars):
    """ Reads max symbols from the specified file into an existing text record.
    If the argument max is zero, then all symbols until eof are read. """
    libTawa.TXT_read_text.restype=c_void_p
    libTawa.TXT_read_text.argtypes=[c_uint,c_uint,c_uint]
    libTawa.TXT_read_text (fileid, text, maxchars)
