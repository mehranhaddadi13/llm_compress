""" Writes out the codelength for the input text for a list of models. """

import Tawa
import json

import sys, getopt

def usage ():
    print(
        "Usage: codelength [options]",
	"",
	"options:",
	"  -d n\tdebug level=n",
	"  -c\tprint out codelengths for each character=n",
	"  -e\tcalculate cross-entropy and not codelength",
        "  -i\tinput filename=fn (required argument if -I not present)",
        "  -I\tinput string=str (required argument if -i not present)",
	"  -m fn\tlist of models filename=fn (required argument if -M not present)",
	"  -M fn\tmodel filename=fn (required argument if -m not present)",
	"  -r\tprint out arithmetic coding ranges",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
	'-c': ('Debug Chars', True),
        '-d': ('Debug Level', 'Int'),
	'-e': ('Calculate Cross-Entropy', True),
        '-i': ('Input Filename', 'Str'),
        '-I': ('Input String', 'Str'),
        '-m': ('Models Filename', 'Str'),
        '-M': ('Model Filename', 'Str'),
	'-r': ('Print Coding Ranges', True),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    Tawa.init_arguments (arguments, "cd:ehi:I:m:M:r", ["help"], opts_dict,
                        "Codelength: option not recognized",
                         [['Input Filename', 'Input String'], ['Models Filename', 'Model Filename']],
                         usage)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Input Filename' in Arguments):
        input_filename = Arguments ['Input Filename']
        Input_text = Tawa.load_text (input_filename)

    if ('Input String' in Arguments):
        Input_text = TXT_write_string (Arguments ['Input String'])
        print ("Input text=", Input_text)

    if ('Models Filename' in Arguments):
        models_filename = Arguments ['Models Filename']
        Tawa.load_models (models_filename)
    if ('Model Filename' in Arguments):
        model_filename = Arguments ['Model Filename']
        Tawa.load_model (model_filename)

    if (Tawa.get_argument (Arguments, 'Debug Level') > 0):
        Tawa.dump_models ()

    if (Tawa.numberof_models () < 1):
        usage()

    codelengths, minimum = Tawa.codelength_models (Input_text, 'Calculate Cross-Entropy' in Arguments,
                                                  'Debug Range' in Arguments,
                                                  'Debug Chars' in Arguments)
    print(json.dumps(codelengths, indent=4, sort_keys=True))
    print("Minimum =", minimum)

    Tawa.release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
