""" Plot the compression codelength for an input text file. """

import Tawa

from matplotlib import pyplot as plt

import sys, getopt

def usage ():
    print(
        "Usage: plot-codelength [options]",
	"",
	"options:",
	"  -a n\tsize of alphabet=n",
	"  -e c\tescape method for the model=c",
	"  -E\tplot entropy rather than codelength",
        "  -i\tinput filenames=fn (required argument; contains a list of input filenames (for multiple plots)",
	"  -L\tuse log scales",
        "  -m\tmodel filename=fn (for loading existing model)"
	"  -O n\tmax order of model=n",
	"  -p n\tplot codelength every n chars",
        "  -S\tplot size of the model instead of codelength",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-a': ('Alphabet Size', 'Int'),
        '-e': ('Escape Method', 'Str'),
        '-E': ('Plot Entropy', False),
        '-i': ('Input Files', 'Str'),
        '-L': ('Log Scales', False),
        '-m': ('Model Filename', 'Str'),
        '-O': ('Max Order', 'Int'),
        '-p': ('Plot Interval', 'Int'),
        '-S': ('Plot Sizeof Model', False),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    Tawa.init_arguments (arguments, "a:e:Ei:ILm:O:p:S", ["help"], opts_dict,
                         "Plot-codelength: option not recognized",
                         [['Input Files']], usage)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    Tawa.set_default_PPM_arguments (Arguments)

    if not ('Plot Interval' in Arguments):
        Arguments ['Plot Interval'] = 0
    plot_entropy = 'Plot Entropy' in Arguments
    log_scales = 'Log Scales' in Arguments
    plot_size = 'Plot Sizeof Model' in Arguments

    if ('Input Files' in Arguments):
        with open(Arguments ['Input Files']) as f:
            input_lines = f.read().splitlines()
            Input_Files = [(s.split(' ')[0], s.split(' ')[1].encode(encoding="ascii",errors="replace"))
                           for s in input_lines]

    if ('Model Filename' in Arguments):
        # Load pre-existing model
        model_filename = Arguments ['Model Filename']
        Model = Tawa.load_model (model_filename)

    Tawa.create_codelength_plot (plot_entropy, log_scales, plot_size)

    for (label, filename) in Input_Files:
        Input_text = Tawa.load_text (filename)

        if (not 'Model Filename' in Arguments):
            # Create an empty (unprimed) dynamic model instead
            Model = Tawa.create_PPM_model (Arguments ['Alphabet Size'], Arguments ['Max Order'],
		                           Arguments ['Escape Method'])

        Tawa.plot_codelength_text (Model, Input_text, label, Arguments ['Plot Interval'],
                                   plot_entropy, log_scales, plot_size)

    plt.legend () #(loc = 'lower left')
    plt.show()

    Tawa.release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
