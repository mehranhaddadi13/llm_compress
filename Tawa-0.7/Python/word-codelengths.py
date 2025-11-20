# This program plots the word codelengths for an input text file given a PPM model. This model
# could have been trained on the same text or on another text.
# The purpose of the program is to discover relationships in the way words are spelt.

import Tawa
import sys, getopt
import re

def usage ():
    print(
        "Usage: plot-word-codelengths [options]",
	"",
	"options:",
	"  -i fn\tinput filename=fn (required argument)",
	"  -m fn\tmodel filename=fn (required argument)",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-i': ('Input Filename', 'Str'),
        '-m': ('Model Filename', 'Str'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    Tawa.init_arguments (arguments, "hi:m:", ["help"], opts_dict,
                        "plot-word-codelengths: option not recognized",
                         [['Input Filename'], ['Model Filename']], usage)

def get_word_types(input_filename):
    """ Returns a list of word types from the file input-filename. """
    
    with open(input_filename) as f:
        s = re.sub(r'[^\w\s]','',f.read()) # Remove punctuation
        return set(s.split())

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Input Filename' in Arguments):
        input_filename = Arguments ['Input Filename']
        # print ("Loading words from", input_filename)
        word_types = get_word_types (input_filename)
        # print ("Number of word types =", len(word_types))

    if ('Model Filename' in Arguments):
        model_filename = Arguments ['Model Filename']
        Model = Tawa.load_model (model_filename)

    if (Tawa.numberof_models () < 1):
      usage()

    for word in word_types:
        codelength = Tawa.codelength_string (Model, word + ' ',
                                             encode_sentinel_first = False,
                                             encode_sentinel_last = True)# / len (word)
        print ("{0:6.2f}".format(codelength), word)

if __name__ == '__main__':
    main()
