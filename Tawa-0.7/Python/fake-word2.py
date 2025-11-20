""" Tries to generate fake words using a language model. """

import Tawa
import random
import sys, getopt

def usage ():
    print(
        "Usage: fake-word [options]",
	"",
	"options:",
	"  -m fn\tmodel filename=fn (required argument)",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-m': ('Model Filename', 'Str'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    Tawa.init_arguments (arguments, "hm:", ["help"], opts_dict,
                        "fake-word: option not recognized", [['Model Filename']], usage)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Model Filename' in Arguments):
        model_filename = Arguments ['Model Filename']
        Model = Tawa.load_model (model_filename)

    if (Tawa.numberof_models () < 1):
      usage()

    print ("Testing out some words:")
    strings = ["beginning ", "gebittorn ", "heavenest ",
               "tracketor ", "finlayson ", "tictocker "]
    for string in strings:
        codelength = Tawa.codelength_string (Model, string)
        print (codelength)
        print ("Codelength = {0:5.2f} for string: '{1}'".format(codelength, string))

    # Creating 100 fake words
    for w in range (100):

        wordlen = random.randrange(4,12)
        print ("Creating some random words of length:", wordlen)
        best_word = ''
        best_codelength = 100000
        for rw in range (500000):
            if (best_codelength / wordlen < 5.5):
                break
            if (rw > 0) and (rw % 100000 == 0):
                print ("Generating random word, number", rw,
                       ", best codelength so far: {0:5.2f}".format(best_codelength),
                       ", best word: ", best_word)
            word = ''
            for p in range (wordlen):
                cc = random.choice ('abcdefghijklmnopqrstuvwxyz')
                word += cc
            codelength = Tawa.codelength_string (Model, word)
            #print (rw+1, word, "Codelength = {0:5.2f}".format(codelength))
            if (codelength < best_codelength):
                best_codelength = codelength
                best_word = word

        print ("Best word [", best_word, "] Best Codelength = {0:5.2f}".format(best_codelength))

    Tawa.release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
