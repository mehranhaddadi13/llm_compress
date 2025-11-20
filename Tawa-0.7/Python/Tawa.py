""" Python Tawa module for compression-based language models (CLMs). """

from pyTawa.TLM import *
from pyTawa.TXT import *
from pyTawa.TAR import *

from matplotlib import pyplot as plt

def init_arguments (arguments, shortopts, longopts, opts_dict, err_message, opt_key_list, usage):
    """ Initialises the arguments dictionary from the command line. """

    ok = TAR_init_arguments (arguments, shortopts, longopts, opts_dict, err_message)
    if (not ok):
        usage()
        sys.exit(2)

    if (not TAR_check_required_arguments (arguments, opts_dict, opt_key_list)):
        usage ()
        sys.exit(2)

def get_argument (arguments, opt_key):
    """ Returns value associated with the opt_key is in the arguments
    dictionary, 0 if it does not exist. """
    return (TAR_get_argument (arguments, opt_key))

def load_text (input_filename):
    """ Creates a new text record and text number, then loads it using text from
    the file. Assumes standard ASCII text. """
    input_file = TXT_open_file (input_filename, b"r", b"Reading input file",
                                b"Codelength: can't open input file" )
    input_text = TXT_load_text (input_file)
    return (input_text)

def load_model (model_filename):
    """ Loads the model from the file named model_filename. """

    model_file = TXT_open_file (model_filename, b"r", b"Reading model file",
                                b"Can't open model file" )
    model = TLM_load_model (model_file)
    return (model)

def load_models (filename):
    """ Load the models and their associated tags from the specified file. """
    TLM_load_models (filename)

def dump_models ():
    TXT_dump_models1 (Stderr_File)

def numberof_models ():
    """ Returns the number of currently valid models. """
    return TLM_numberof_models ()

def release_models ():
    """ Releases the memory used by all the models. """
    TLM_release_models ()

def create_PPM_model (alphabet_size, order, escape_method):
    """ Creates and returns an empty (unprimed) PPM compression-based dynamic language model. """
    model = TLM_create_PPM_model (b"PPM Model", alphabet_size, order, escape_method)
    return (model)

def set_default_PPM_arguments (arguments):
    """ Sets the default arguments for a PPM models. """

    if not ('Alphabet Size' in arguments):
        arguments ['Alphabet Size'] = TLM_PPM_ALPHABET_SIZE
    if not ('Escape Method' in arguments):
        arguments ['Escape Method'] = TLM_PPM_Method_D
    if not ('Max Order' in arguments):
        arguments ['Max Order'] = TLM_PPM_MAX_ORDER

def codelength_text (model, text, debug_range = False, debug_chars = False,
                     encode_sentinel_first = True, encode_sentinel_last = True):
    """ Returns the codelength (in bits) for encoding the text (a Tawa type) using the PPM
    model. Note: Normally the codelengthString method below would be used instead of this method.
    encode_sentinel_first and encode_sentinel_last can be used to turn off encoding of the sentinel
    symbol prior to the first character being encoding and after the last one has been encoded. """

    context = TLM_create_context (model)
    TLM_set_context_operation (TLM_Get_Codelength)

    """
    Insert the sentinel symbol at start of text to ensure first character
    is encoded using a sentinel symbol context rather than an order 0
    context.
    """

    if (debug_range):
        print ("Coding ranges for the sentinel symbol (not included in overall total:", file=sys.stderr);
    if (encode_sentinel_first):
        TLM_update_context (model, context, TXT_sentinel_symbol ())
    if (debug_range):
        print ("", file=sys.stderr);

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

    # Now encode the sentinel symbol again to signify the end of the text
    if (encode_sentinel_last):
        TLM_update_context (model, context, TXT_sentinel_symbol ())
    codelen = TLM_get_codelength ()
    if (debug_chars):
        print ("Codelength for sentinel symbol =", "%.3f" % codelen,
               file = sys.stderr)
    codelength += codelen

    TLM_release_context (model, context)

    return (codelength)

def plot_values (pos, codelength, sizeof_model, x, y, plot_entropy, plot_size):
# Plots the values pos (file position) and codelength into the x and y lists.

    x += [pos]
    if (not plot_entropy):
        if (plot_size):
            y += [sizeof_model]
        else:
            y += [codelength]
    else:
        y += [codelength / pos]

def create_codelength_plot(plot_entropy, log_scales, plot_size):
    """ Creates the codelength vs. file position plot. """

    if (not plot_entropy):
        plt.title("Codelength vs. file position")
    else:
        plt.title("Entropy vs. file position")
    plt.xlabel("File position")
    if (not plot_entropy):
        if (plot_size):
            plt.ylabel("Size of Model")
        else:
            plt.ylabel("Codelength")
    else:
        plt.ylabel("Entropy (bpc)")
    if (log_scales):
        plt.xscale('log') # Use log scale for the X axis                                
        plt.yscale('log') # Use log scale for the Y axis                                

def plot_codelength_text (model, text, label, plot_interval, plot_entropy, log_scales, plot_size):
    """ Returns the codelength (in bits) for encoding the text using the PPM
    model. """

    x = []
    y = []

    context = TLM_create_context (model)
    TLM_set_context_operation (TLM_Get_Codelength)

    codelength = 0.0
    # Now encode each symbol
    pos = 0
    for p in range(TXT_length_text1 (text)):
        pos += 1
        symbol = TXT_getsymbol_text (text, p)
        TLM_update_context (model, context, symbol)
        codelen = TLM_get_codelength ()
        sizeof_model = TLM_sizeof_model (model)
        codelength += codelen
        if (plot_interval == 0) or (pos % plot_interval == 0):
            if (plot_interval):
                print ("Pos = {0} Codelength = {1:.2f} Sizeof Model = {2}".format(pos,
                        codelength, sizeof_model))
            plot_values (pos, codelength, sizeof_model, x, y, plot_entropy, plot_size)

    # Now use the sentinel symbol to signify the end of the text
    TLM_update_context (model, context, TXT_sentinel_symbol ())
    codelen = TLM_get_codelength ()
    codelength += codelen
    plot_values (pos, codelength, sizeof_model, x, y, plot_entropy, plot_size)

    plt.plot (x, y, label = label)

    TLM_release_context (model, context)

def codelength_string (model, string, encode_sentinel_first = True, encode_sentinel_last = True):
    """ Returns the codelength (in bits) for encoding the string using the PPM
    model.

    Note: For efficiency reasons, this will encode the string using ASCII replacing any errors.

    To use this method for non-ASCII strings, first transliterate the string into an
    equivalent ASCII string. Otherwise, use alternative Tawa methods to encode the string using an equivalent
    symbol numbers conversion. """

    string_ascii = string.encode(encoding="ascii",errors="replace")
    string_text = TXT_write_string (string_ascii)
    codelength = codelength_text (model, string_text,
                                  encode_sentinel_first = encode_sentinel_first,
                                  encode_sentinel_last = encode_sentinel_last)
    #print ("Codelength = {0:5.2f} for string: '{1}'".format(codelength, string))

    return (codelength)

def codelength_models (text, compute_entropy = False,
                      debug_range = False, debug_chars = False):
    """ Returns the codelengths for encoding the text using the loaded
        PPM models. Also returns the tag associated with the minimum codelength. """

    min_codelen = 0.0

    min_tag = ""
    textlen = TXT_length_text (text)

    codelengths = {}
    TLM_reset_modelno ()
    while (model := TLM_next_modelno ()):
        tag = TLM_get_tag (model)
        codelength = codelength_text (model, text, debug_range, debug_chars)
        if (compute_entropy):
            codelength /= textlen
        if ((min_codelen == 0.0) or (codelength < min_codelen)):
            min_codelen = codelength
            min_tag = tag
        codelengths [tag] = codelength

    return (codelengths, min_tag)
