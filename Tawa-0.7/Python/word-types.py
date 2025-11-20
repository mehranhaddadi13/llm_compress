# This program generates a list of word types for an input text.

import Tawa
import sys, getopt
import re

def usage ():
    print(
        "Usage: plot-word-codelengths [options]",
	"",
	"options:",
	"  -i fn\tinput filename=fn (required argument)",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-i': ('Input Filename', 'Str'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    Tawa.init_arguments (arguments, "hi:m:", ["help"], opts_dict,
                        "plot-word-codelengths: option not recognized",
                         [['Input Filename']], usage)

def get_word_types(input_filename):
    """ Returns a sorted list of word types from the file input-filename. """
    
    with open(input_filename) as f:
        s = re.sub(r'[^\w\s]','',f.read()) # Remove punctuation
        return sorted(set(s.split()))

def main():
    """ Main program. """

    word_types = []

    Arguments = {}
    init_arguments (Arguments)

    if ('Input Filename' in Arguments):
        input_filename = Arguments ['Input Filename']
        # print ("Loading words from", input_filename)
        word_types = get_word_types (input_filename)
        # print ("Number of word types =", len(word_types))

    for word in word_types:
        print (word, end = " ")

if __name__ == '__main__':
    main()
