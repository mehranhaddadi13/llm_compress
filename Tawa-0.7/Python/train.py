""" Loads or creates a model, adds some text to the model, then writes out the changed
   model. For testing the loading and dumping model routines. """

from pyTawa.TLM import *
from pyTawa.TXT import *
from pyTawa.TAR import *

import sys, getopt

def usage ():
    print(
	"Usage: train [options]",
	"",
	"options:",
	"  -B\tforce a break at each eoln",
	"  -N\tinput text is a sequence of unsigned numbers",
	"  -S\twrite out the model as a static model",
	"  -T n\tlong description (title) of model (required argument)",
	"  -U\tdo not perform update exclusions",
	"  -X\tdo not perform full exclusions",
	"  -a n\tsize of alphabet=n",
	"  -d n\tdebugging level=n",
	"  -e n\tescape method=c",
        "  -h\tprint help",
	"  -i fn\tinput filename=fn (required argument)",
	"  -m fn\tmodel filename=fn (optional)",
	"  -o fn\toutput filename=fn (required argument)",
	"  -O n\tmax order of model=n",
	"  -p n\tprogress report every n chars.",
	"  -t n\ttruncate input size after n bytes",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-B': ('Break Eoln', True),
        '-N': ('Load Numbers', True),
        '-S': ('Static Model', True),
        '-T': ('Title', 'Str'),
        '-U': ('Performs Update Excls', False),
        '-X': ('Performs Full Excls', False),
        '-a': ('Alphabet Size', 'Int'),
        '-d': ('Debug Level', 'Int'),
        '-e': ('Escape Method', 'Ord'),
        '-i': ('Input Filename', 'Str'),
        '-m': ('Model Filename', 'Str'),
        '-o': ('Output Filename', 'Str'),
        '-O': ('Max Order', 'Int'),
        '-p': ('Debug Progress', 'Int'),
        '-t': ('Max Input Size', 'Int'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    ok = TAR_init_arguments (arguments,
        "BNST:UXa:d:e:hi:m:o:O:p:t:", ["help"], opts_dict,
        "Train: option not recognized")
    if (not ok):
        usage()
        sys.exit(2)

    if (not 'Model Filename' in arguments):
        if ('Escape Method' in arguments):
            escape = arguments ['Escape Method'] - ord('A')
            assert (escape >= 0)
            arguments ['Escape Method'] = escape
        """
        if (not TAR_check_required_arguments (arguments, opts_dict,
            ['Alphabet Size', 'Escape Method', 'Title', 'Max Order'])):
            usage ()
            sys.exit(1)
        """
        print ("\nCreating new model\n", file=sys.stderr)
    else:
        if ('Title' in arguments):
            TLM_set_load_operation (TLM_Load_Change_Title, arguments ['Title'])
        Model = TLM_read_model (arguments ['Model Filename'],
                                b"Loading model from file",
			        b"Train: can't open model file")

    # Set default values for any missing non-required arguments
    TAR_set_argument_defaults (arguments,
        (('Alphabet Size', TLM_PPM_ALPHABET_SIZE),
         ('Max Order', TLM_PPM_MAX_ORDER),
         ('Escape Method', TLM_PPM_ESCAPE_METHOD),
         ('Performs Full Excls', TLM_PPM_PERFORMS_FULL_EXCLS),
         ('Performs Update Excls', TLM_PPM_PERFORMS_UPDATE_EXCLS)))

    if (not TAR_check_required_arguments (arguments, opts_dict,
            ['Title', 'Input Filename', 'Output Filename'])):
        usage ()
        sys.exit (1)            

def train_model (fileid, model, arguments):
    """ Trains the model from the characters in the file with id Fileid. """

    context = TLM_create_context (model)

    TLM_set_context_operation (TLM_Get_Nothing)

    if ('Break Eoln' in arguments):
        # Start off the training with a sentinel symbol to indicate a break
        TLM_update_context (model, context, TXT_sentinel_symbol ())
    Debug_level = TAR_get_argument (arguments, 'Debug Level')
    Debug_progress = TAR_get_argument (arguments, 'Debug Progress')
    Load_numbers = 1 if TAR_get_argument (arguments, 'Load Numbers') else 0

    p = 0;
    while True:
        p += 1;
        if ((Debug_progress > 0) and ((p % Debug_progress) == 0)):
            print ("training pos", p, file=sys.stderr)

        # repeat until EOF or max input
        if ('Max Input Size' in arguments) and (p >= arguments ['Max Input Size']):
            break
        if (not TXT_getsymbol_file (fileid, Load_numbers)):
            break

        sym = TXT_input_symbol()
        
        if (('Break Eoln' in arguments) and (sym == BREAK_SYMBOL)):
            sym = TXT_sentinel_symbol ()

        TLM_update_context (model, context, sym)

        if (Debug_level > 1):
            TLM_dump_PPM_model (Stderr_File, model)

    TLM_update_context (model, context, TXT_sentinel_symbol ())
    TLM_release_context (model, context)

    print ("Trained on", p, "symbols\n", file=sys.stderr)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    input_filename = Arguments ['Input Filename']
    output_filename = Arguments ['Output Filename']

    Input_file = TXT_open_file (input_filename, b"r", b"Reading input file",
                                b"Train: can't open input file" )
    Output_file = TXT_open_file (output_filename, b"w", b"Writing to output file",
                                 b"Train: can't open output file" )

    if not ('Model Filename' in Arguments):
        Model = TLM_create_PPM_model (Arguments ['Title'],
                Arguments ['Alphabet Size'], Arguments ['Max Order'],
                Arguments ['Escape Method'], Arguments ['Performs Full Excls'],
                Arguments ['Performs Update Excls'],)
    else:
        if (not TLM_get_PPM_model (Arguments ['Model Filename'])):
            print ("Fatal error: Invalid model number\n", file=sys.stderr);
            sys.exit (1)
        elif (TLM_get_PPM_model_form () == TLM_Static):
            print ("Fatal error: This implementation does not permit further training when\n",
                   file=sys.stderr)
            print ("a static model has been loaded\n", file=sys.stderr)
            sys.exit (1)

        # Check for consistency of parameters between the loaded model and the model to be written out
        if (not check_argument (Arguments, 'Alphabet Size',
                                TLM_get_PPM_alphabet_size ())):
            print ("\nFatal error: alphabet sizes of output model does not match input model\n\n", file=sys.stderr)
            sys.exit (1)

        if (not check_argument (Arguments, 'Max Order',
                                TLM_get_PPM_max_order ())):
            print ("\nFatal error: max order of output model does not match input model\n\n", file=sys.stderr)
            sys.exit (1)

    train_model (Input_file, Model, Arguments)
    if ('Static Model' in Arguments):
        TLM_write_model (Output_file, Model, TLM_Static)
    else:
        TLM_write_model (Output_file, Model, TLM_Dynamic)

    if (TAR_get_argument (Arguments, 'Debug Level') > 0):
        TLM_dump_PPM_model (Stderr_File, Model)

    TLM_release_model (Model)

    sys.exit (0)

if __name__ == '__main__':
    main()
