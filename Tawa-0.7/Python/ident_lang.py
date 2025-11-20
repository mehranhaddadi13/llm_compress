""" Works out the language of a string of text. """

from pyTawa.TLM import *
from pyTawa.TXT import *
from pyTawa.TAR import *

import sys, getopt

def usage ():

    print(
	"Usage: ident_lang [options] <input-text",
	"",
	"options:"
	"  -m fn\tlist of models filename=fn (required)",
	"  -c\tprint out codelengths for each character=n",
	"  -e\tcalculate cross-entropy and not codelength",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
	'-c': ('Debug Chars', True),
	'-e': ('Calculate Cross-Entropy', True),
        '-m': ('Models Filename', 'Str'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    ok = TAR_init_arguments (arguments,
        "cehm:r", ["help"], opts_dict,
        "Ident_lang: option not recognized")
    if (not ok):
        usage()
        sys.exit(2)

    if (not TAR_check_required_arguments (arguments, opts_dict,
        ['Models Filename'])):
        usage ()
        sys.exit(2)

def codelengthText (model, text, debug_chars = False):
    """ Returns the codelength (in bits) for encoding the text using the PPM
    model. """

    context = TLM_create_context (model)
    TLM_set_context_operation (TLM_Get_Codelength)

    """
    Insert the sentinel symbol at start of text to ensure first character
    is encoded using a sentinel symbol context rather than an order 0
    context.
    """

    codelength = 0.0
    # Now encode each symbol
    for p in range(TXT_length_text1 (text)):
        symbol = TXT_getsymbol_text (text, p)
        TLM_update_context (model, context, symbol)
        codelen = TLM_get_codelength ()
        if (debug_chars):
            print ("Codelength for character", "%c" % symbol,
                   "= %7.3f" % codelen)
        codelength += codelen

    TLM_release_context (model, context)

    return (codelength)

def codelengthModels (text, display_entropy = False,
                      debug_chars = False):
    """ Prints out the codelength for encoding the text using the loaded
    PPM models. """

    min_codelen = 0.0

    min_tag = ""
    textlen = TXT_length_text (text)

    TLM_reset_modelno ()
    while (model := TLM_next_modelno ()):
        tag = TLM_get_tag (model)
        codelength = codelengthText (model, text, debug_chars)
        if (display_entropy):
            codelength /= textlen
        if ((min_codelen == 0.0) or (codelength < min_codelen)):
            min_codelen = codelength
            min_tag = tag
        print ("%-24s" % tag, "%9.3f" % codelength)

    if (display_entropy):
        print ("Minimum cross-entropy for ", end = "")
        print (min_tag, "=", "%9.3f" % codelength)
    else:
        print ("Minimum codelength for ", end = "")
        print (min_tag, "=", "%9.3f" % min_codelen)


def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Models Filename' in Arguments):
        models_filename = Arguments ['Models Filename']
        TLM_load_models (models_filename)

    if (TLM_numberof_models () < 1):
        usage()

    while True:
        print ("Enter text: ", end = '')
        try:
            Input_line = input()
        except EOFError:
            break
        print ("Length of text =", len(Input_line), "characters\n")
        print ("Line =", Input_line)
        print ("Line encoded =", Input_line.encode(encoding="ascii",errors="replace"))
        # The following is a fudge to deal with non-ascii characters
        Input_text = TXT_write_string (Input_line.encode(encoding="ascii",errors="replace"))
        codelengthModels (Input_text, 'Calculate Cross-Entropy' in Arguments,
                          'Debug Chars' in Arguments)

    TLM_release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
