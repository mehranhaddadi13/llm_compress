import ctypes

libTawa = ctypes.CDLL('../../pyTawa.so')

def TLM_release_context (model, context):
    libTawa.TLM_release_context (model, context)

def TLM_release_models():
    libTawa.TLM_release_models ()

def TLM_get_codelength():
    return ctypes.c_float.in_dll(libTawa, "TLM_Codelength").value

def TLM_set_codelength(value):
    ctypes.c_float.in_dll(libTawa, "TLM_Codelength").value = value

def TXT_write_symbol (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form. """
    libTawa.TXT_write_symbol (fileid, symbol)

def TXT_dump_symbol (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form (excluding '\n' and '\t's). """
    libTawa.TXT_dump_symbol (fileid, symbol)

def TXT_dump_symbol1 (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form (excluding white space). """
    libTawa.TXT_dump_symbol1 (fileid, symbol)

def TXT_dump_symbol2 (fileid, symbol):
    """ Writes the ASCII symbol out in human readable form (including '\n' and '\t's). """
    libTawa.TXT_dump_symbol2 (fileid, symbol)

def TXT_is_ascii (symbol):
    """ Returns True if symbol is an ASCII character. """
    return (bool(libTawa.TXT_is_ascii (symbol)))

def TXT_is_vowel (symbol):
    """ Returns True if symbol is a vowel. """
    return (bool(libTawa.TXT_is_vowel (symbol)))

def TXT_is_consonant (symbol):
    """ Returns True if symbol is a consonant. """
    return (bool(libTawa.TXT_is_consonant (symbol)))

def TXT_is_alphanumeric (symbol):
    """ Returns True if symbol is an alphanumeric character. """
    return (bool(libTawa.TXT_is_alphanumeric (symbol)))

def TXT_is_alpha (symbol):
    """ Returns True if symbol is an alphabetic character. """
    return (bool(libTawa.TXT_is_alpha (symbol)))

def TXT_is_control (symbol):
    """ Returns True if symbol is a control character. """
    return (bool(libTawa.TXT_is_control (symbol)))

def TXT_is_digit (symbol):
    """ Returns True if symbol is a digit. """
    return (bool(libTawa.TXT_is_digit (symbol)))

def TXT_is_graph (symbol):
    """ Returns True if symbol is a printable character except space. """
    return (bool(libTawa.TXT_is_graph(symbol)))

def TXT_is_lower (symbol):
    """ Returns True if symbol is a uppercase character. """
    return (bool(libTawa.TXT_is_lower (symbol)))

def TXT_to_lower (symbol):
    """ Returns the lower case of symbol. """
    return (libTawa.TXT_to_lower (symbol))

def TXT_is_print (symbol):
    """ Returns True if symbol is a printable character. """
    return (bool(libTawa.TXT_is_print (symbol)))

def TXT_is_punct (symbol):
    """ Returns True if symbol is a punctuation character. """
    return (bool(libTawa.TXT_is_punct (symbol)))

def TXT_is_space (symbol):
    """ Returns True if symbol is a white space character. """
    return (bool(libTawa.TXT_is_space (symbol)))
 
def TXT_is_upper (symbol):
    """ Returns True if symbol is a uppercace character. """
    return (bool(libTawa.TXT_is_upper (symbol)))

def TXT_to_upper (symbol):
    """ Returns the upper case of symbol. """
    return (libTawa.TXT_to_upper (symbol))

def TXT_valid_text (text):
    """ Returns non-zero if the text is valid, zero otherwize. """
    return (bool(libTawa.TXT_valid_text (text)))

def TXT_create_text (void):
    """ Creates a text record. """
    return (libTawa.TXT_create_text ())

def TXT_release_text (text):
    """ Releases the memory allocated to the text and the text number (which may
    be reused in later TXT_create_text calls). """
    return (libTawa.TXT_release_text (text))

def TXT_compare_text (text1, text2):
    """ Compares text1 with text2. Returns zero if they are the same,
    negative if text1 < text2, and positive if text1 > text2.
    The sentinel symbol returned by TLM_sentinel_symbol is
    regarded as having a value lower than all other symbols. """
    return (libTawa.TXT_compare_text (text1, text2))

def TXT_strcmp_text (text, string):
    """ Compares the text with the string. Returns zero if they are the same,
    negative if text < string, and positive if text > string. """
    return (libTawa.TXT_strcmp_text (string))

def TXT_null_text (text):
    """ Returns True if the text is empty. """
    return (bool(libTawa.TXT_null_text (text)))

def TXT_sentinel_symbol (void):
    """ Returns an unsigned integer that uniquely identifies a special ``sentinel''
    symbol """
    return (libTawa.TXT_sentinel_symbol ())

def TXT_sentinel_text (text):
    """ Returns True if the text contains just a sentinel symbol. """
    return (bool(libTawa.TXT_sentinel_text (text)))

def TXT_createsentinel_text (void):
    """ Returns text that contains just a sentinel symbol. """
    return (libTawa.TXT_createsentinel_text ())

def TXT_alloc_text (text):
    """ Returns the allocation of the text (it's memory use; for debugging purpose). """
    return (libTawa.TXT_alloc_text (text))

def TXT_length_text (text):
    """ Returns the length of the text. Assumes the text is non-NIL. """
    return (libTawa.TXT_length_text (text))

def TXT_length_text1 (text):
    """ Returns the length of the text. Returns 0 if the text is NIL. """
    return (libTawa.TXT_length_text1 (text))

def TXT_setlength_text (text, length):
    """ Sets the length of the text to be at most length symbols long. If the
    current length of the text is longer than this, then the text will be
    truncated to the required length, otherwise the length will remain
    unchanged. Setting the length of the text to be 0 will set the text
    to the null string. """
    libTawa.TXT_setlength_text (text)

def TXT_getpos_text (text, symbol, *pos):
    """ Returns True if the symbol is found in the text. The argument pos is set
    to the position of the first symbol in the text that matches the specified
    symbol if found, otherwise it remains unchanged. """
    return (libTawa.TXT_getpos_text (text, symbol, pos))

def TXT_getrpos_text (text, symbol, *pos):
    """ Returns True if the symbol is found in the text. The argument pos is set to
    the position of the last symbol in the text that matches the specified
    symbol if found, otherwise it remains unchanged. """
    return (libTawa.TXT_getrpos_text (text, symbol, pos))

def TXT_string_text (text, string, maxchars):
    """ Returns as ASCII version of the text in string. """
    return (libTawa.TXT_string_text (text, string, maxchars))

def TXT_getstr_text (text, string, *pos):
    """ Returns True if the string is found in the text. The argument pos is set
    to the position of the first symbol in the text starting from position
    0 that matches the specified string if found, otherwise it remains
    unchanged. """
    return (libTawa.TXT_getstr_text (text, symbol, pos))

def TXT_getstring_text (text, string, *pos):
    """ Returns True if the string is found in the text. The argument pos contains
    the starting point and gets set to the position of the first point
    in the following text that matches the specified string if found,
    otherwise it remains unchanged. """
    return (libTawa.TXT_getstring_text (text, symbol, pos))

def TXT_get_symbol (text, pos, *symbol):
    """ Returns True if there exists a symbol at position pos in the text. 
    The argument symbol is set to the specified symbol. """
    return (libTawa.TXT_getsymbol_text (text, pos, symbol))

def TXT_put_symbol (text, symbol, pos):
    """ Inserts the symbol at position pos into the text. Inserting a symbol beyond
    the current bounds of the text will cause a run-time error. """
    return (libTawa.TXT_put_symbol (text, pos, symbol))

def TXT_find_symbol (text, symbol, start_pos, pos):
    """ Returns True if the symbol exists in the text starting from start_pos. 
    The argument pos is set to the symbol's position. """
    return (libTawa.TXT_find_symbol (text, symbol, start_pos, pos))

def TXT_find_text (text, subtext, *pos):
    """ Returns True if the subtext is found in the text. The argument pos is set
    to the position of the first symbol in the text starting from position
    0 that matches the specified string if found, otherwise it remains
    unchanged. """
    return (libTawa.TXT_find_text (text, subtext, pos))

def TXT_append_symbol (text, symbol):
    """ Appends the symbol onto the end of the text. """
    libTawa.TXT_append_symbol (text, symbol)

def TXT_append_string (text, string):
    """ Appends the string onto the end of the text. """
    libTawa.TXT_append_symbol (text, string)

def TXT_find_string (text, string, start_pos, pos):
    """ Returns True if the string exists in the text starting from start_pos. 
    The argument pos is set to the string's position. """
    return (libTawa.TXT_find_string (text, string, start_pos, pos))

def TXT_nullify_string (text, string):
    """ Replaces all occurences of string in the text with nulls. (This can be used for
    "removing" the string if using TXT_dump_symbol2 to dump it out. """
    libTawa.TXT_nullify_string (text, string)

def TXT_overwrite_string (text, string, new_string):
    """ Overwrites all occurences of string in the text with new_string. Appends
    nulls (or truncates) if there is a mis-match in lengths of the string. """
    libTawa.TXT_overwrite_string (text, string, new_string)

def TXT_extract_text (text, subtext, subtext_pos, subtext_len):
    """ Extracts the text from out of the text record. The argument subtext is set
    to the extracted text; subtext_len is the length of the text to be
    extracted; and subtext_pos is the position from which the text should be
    extracted from. The extracted subtext is filled with nulls for any part of
    it that extends beyond the bounds of the text record. """
    libTawa.TXT_extract_text (text, subtext, subtext_pos, subtext_len)

def TXT_extractstring_text (text, string, subtext_pos, subtext_len):
    """ Returns as ASCII version of the extracted text in string. """
    return (libTawa.TXT_extract_text (text, string, subtext_pos, subtext_len))

def TXT_copy_text (text):
    """ Creates a new text record and text number, then copies the text record into
    it. """
    libTawa.TXT_copy_text (text)

def TXT_overwrite_text (text, text1):
    """ Overwrites the text with a copy of text1. """
    libTawa.TXT_overwrite_text (text, text1)

def TXT_append_text (text, text1):
    """ Appends the text1 onto the end of text. """
    libTawa.TXT_append_text (text, text1)

def TXT_readline_text (fileid, text):
    """ Loads the text using the next line of input from the file. Returns the
    last character read or EOF. """
    return (libTawa.TXT_readline_text (fileid, text))

def TXT_readline_text1 (fileid, text):
    """ Loads the text using the next line of input from the file. Returns the
    last character read or EOF. Does not skip over \r characters. """
    return (libTawa.TXT_readline_text1 (fileid, text))

def TXT_readline_text2 (fileid, text):
    """ Loads the text using the next line of input from the file. Returns the
    last character read or EOF. Does not skip over \r or \n characters. """
    return (libTawa.TXT_readline_text2 (fileid, text))

def TXT_getline_text (text, line, pos):
    """ Reads the next line from the specified text.
    The argument pos is set to the updated position
    in the text. Returns FALSE when no more text exists. """
    return (bool(libTawa.TXT_getline_text (text, line, pos)))

def TXT_readword_text (fileid, non_word, word):
    """ Reads the non-word and word from the specified file. A word is
    defined as any continuous alphanumeric sequence, and a non-word
    anything in between. Returns FALSE when EOF occurs. """
    return (bool(libTawa.TXT_readword_text (fileid, non_word, word)))

def TXT_readword_text1 (fileid, non_word, word):
    """ Same as TXT_readword_text () except replaces null words with the sentinel word. """
    return (bool(libTawa.TXT_readword_text1 (fileid, non_word, word)))

def TXT_getword_text (text, non_word, word, text_pos, nonword_text_pos, word_text_pos):
    """ Reads the non-word and word from the specified text. A word is
    defined as any continuous alphanumeric sequence, and a non-word
    anything in between. The argument pos is set to the updated position
    in the text. Returns FALSE when no more text exists. """
    return (bool(libTawa.TXT_getword_text (text, non_word, word, text_pos, nonword_text_pos, word_text_pos)))

def TXT_getword_text1 (text, non_word, word, text_pos, nonword_text_pos, word_text_pos):
    """ Same as TXT_getword_text () except replaces null words with the sentinel word. """
    return (bool(libTawa.TXT_getword_text1 (text, non_word, word, text_pos, nonword_text_pos, word_text_pos)))

def TXT_readword1_text (fileid, word):
    """ Reads the word from the specified file. A word is
    defined as any continuous non-whitespace sequence.
    Returns FALSE when EOF occurs. """
    return (bool(libTawa.TXT_readword1_text (fileid, word)))

def TXT_getword1_text (text, word, pos):
    """ Reads the word from the specified text. A word is
    defined as any continuous non-whitespace sequence.
    The argument pos is set to the updated position
    in the text. Returns FALSE when no more text exists. """
    return (bool(libTawa.TXT_getword1_text (text, word, pos)))

def TXT_gettag_text (text, non_word, word, tag, pos):
    """ Reads the non-word, word and tag (i.e. part of speech) from the specified
    text. A word is defined as any continuous sequence of symbols up until
    the next tag symbol, a non-word any continuous sequence until the next
    appearance of a alphanumeric symbol and a tag symbol is any symbol
    whose value is greater than the maximum ASCII symbol value (i.e. > 256).
    The argument pos is set to the updated position in the text. Returns FALSE
    when no more text exists. """
    return (bool(libTawa.TXT_gettag_text (text, non_word, word, tag, pos)))

def TXT_load_text (fileid):
    """ Creates a new text record and text number, then loads it using text from
    the file. Assumes standard ASCII text. """
    return (libTawa.TXT_load_text (fileid))

def TXT_load_numbers (fileid):
    """ Creates a new text record and text number, then loads it using
    the sequence of unsigned numbers from the file. """
    return (libTawa.TXT_load_numbers (fileid))

def TXT_load_file (fileid):
    """ Creates a new text record and text number, then loads it using text from
    the file. The text is ASCII text except for the "\". This signifies a symbol
    number to follow (a sequence of numeric characters) up until the next \ is
    found. The respective symbol number is substituted into the text. 
    e.g. "standard text with a symbol number \258\ inside it". 
    If \\ is found, then this is replaced with a single \. """
    return (libTawa.TXT_load_file (fileid))

def TXT_load_filetext (fileid, text):
    """ Loads the text record using text from the file. The first integer loaded from the file
    signifies the length of the text. The text is ASCII text except for the "\".
    This signifies a symbol number to follow (a sequence of numeric characters) up until the next \
    is found. The respective symbol number is substituted into the text. e.g. "standard text with
    a symbol number \258\ inside it".  If \\ is found, then this is replaced with a single \. """
    libTawa.TXT_load_filetext (fileid, text)

def TXT_dump_file (fileid, text):
    """ Writes out the text to the output file as ASCII if possible,
    but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
    This output file can then be used for later reloading into a text record
    using the routine TXT_load_symbols. """
    libTawa.TXT_dump_file (fileid, text)

def TXT_write_filetext (fileid, text):
    """ Writes out to the output file the length of the text to be written,
    then the text itself. Writes it out as ASCII if possible,
    but uses the "\<symbol-number>\" format for non-ASCII symbol numbers.
    This output file can then be used for later reloading into a text record
    using the routine TXT_load_filetext. """
    libTawa.TXT_write_filetext (fileid, text)

def TXT_load_symbols (fileid, text):
    """ Overwrites the text record by loading it using the text symbols
    (4 byte numbers) from the file. """
    libTawa.TXT_load_symbols (fileid, text)

def TXT_write_symbols (fileid, text):
    """ Writes out the text symbols (i.e. 4 byte symbol numbers) to the file.
    This output file can then be used for later reloading into a text record
    using the routine TXT_load_symbols. """
    libTawa.TXT_write_symbols (fileid, text)

def TXT_read_text (fileid, text, maxchars):
    """ Reads max symbols from the specified file into an existing text record.
    If the argument max is zero, then all symbols until eof are read. """
    libTawa.TXT_read_text (fileid, text, maxchars)

"""
    txt_to_lower = libTawa.TXT_to_lower
    txt_to_lower.restype = ctypes.c_uint
    txt_to_lower.argtypes = [ctypes.c_uint]
    return txt_to_lower (symbol)
"""
