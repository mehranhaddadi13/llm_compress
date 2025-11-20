""" Try to decrypt a one-part-code message. """

import Tawa
import sys, getopt

def usage ():
    print(
        "Usage: one-part-code [options]",
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
                        "One-part-code: option not recognized", [['Model Filename']], usage)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Model Filename' in Arguments):
        model_filename = Arguments ['Model Filename']
        Model = Tawa.load_model (model_filename)
        print ("Model = ", Model)

    if (Tawa.numberof_models() < 1):
        usage()

    strings = ["In the beginning God created heaven and Earth",
               "In created the heaven beginning and God Earth",
               "Earth and heaven created God beginning the in"]
    for string in strings:
        codelength = Tawa.codelength_string (Model, string)
        print ("Codelength = {0:5.2f} for string: '{1}'".format(codelength, string))

    Tawa.release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
