""" This is not completed yet. """

""" Plot the compression codelength ratios between two input text files (useful for comparing
    the complexity between parallel text files). It is assumed the two text files are aligned
    according to each corresponding lines in the text files. """

""" This program is not completed so will not run. """

from pyTawa.TLM import *
from pyTawa.TXT import *
from pyTawa.TAR import *

from matplotlib import pyplot as plt

import sys, getopt

def usage ():
    print(
        "Usage: plot-codelength [options]",
	"",
	"options:",
	"  -a n\tsize of alphabet=n",
	"  -e c\tescape method for the model=c",
        "  -1\tinput filename 1=fn",
        "  -2\tinput filename 2=fn",
	"  -L\tuse log scales",
	"  -O n\tmax order of model=n",
	"  -p n\tplot codelength every n chars",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-a': ('Alphabet Size', 'Int'),
        '-e': ('Escape Method', 'Str'),
        '-1': ('Input Filename 1', 'Str'),
        '-2': ('Input Filename 2', 'Str'),
        '-L': ('Log Scales', False),
        '-O': ('Max Order', 'Int'),
        '-p': ('Plot Interval', 'Int'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    ok = TAR_init_arguments (arguments,
        "a:e:1:2:LM:N:p:", ["help"], opts_dict,
        "Plot-codelength-ratio: option not recognized")
    if (not ok):
        usage()
        sys.exit(2)

    if (not TAR_check_required_arguments (arguments, opts_dict,
        [['Input Filename 1'], ['Input Filename 2']])):
        usage ()
        sys.exit(2)

def plotValues (pos, codelength, x, y):
# Plots the values pos (file position) and codelength into the x and y lists.


def createPlot(log_scales):
    """ Creates the codelength vs. file position plot. """

    plt.title("Codelength vs. file position")
    plt.xlabel("File position")
    plt.ylabel("Codelength ratio")
    if (log_scales):
        plt.xscale('log') # Use log scale for the X axis                                
        plt.yscale('log') # Use log scale for the Y axis                                

def plotCodelengthText (model1, model2, text1, text2, plot_interval, log_scales):
    """ Plots the codelength (in bits) for encoding the texts using the PPM
    models. """

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
        codelength += codelen
        if (plot_interval == 0) or (pos % plot_interval == 0):
            if (plot_interval):
                print ("Pos = {0} Codelength = {1:.2f}".format(pos, codelength))
                x += [pos]
                y += [codelength]

    # Now use the sentinel symbol to signify the end of the text
    TLM_update_context (model, context, TXT_sentinel_symbol ())
    codelen = TLM_get_codelength ()
    codelength += codelen
    plotValues (pos, codelength, sizeof_model, x, y)

    plt.plot (x1, y1, label = label1)
    plt.plot (x1, y2, label = label2)

    TLM_release_context (model, context)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if not ('Alphabet Size' in Arguments):
        Arguments ['Alphabet Size'] = TLM_PPM_ALPHABET_SIZE
    if not ('Escape Method' in Arguments):
        Arguments ['Escape Method'] = TLM_PPM_Method_D
    if not ('Max Order' in Arguments):
        Arguments ['Max Order'] = TLM_PPM_MAX_ORDER
    if not ('Min Order' in Arguments):
        Arguments ['Min Order'] = 2

    if not ('Plot Interval' in Arguments):
        Arguments ['Plot Interval'] = 0
    log_scales = 'Log Scales' in Arguments

    createPlot (log_scales)

    filename1 = Arguments ['Input Filename 1']
    Input_file1 = TXT_open_file (filename1, b"r", b"Reading input file",
                                b"Codelength: can't open input file" )
    Input_text1 = TXT_load_text (Input_file1)

    filename2 = Arguments ['Input Filename 2']
    Input_file2 = TXT_open_file (filename2, b"r", b"Reading input file",
                                b"Codelength: can't open input file" )
    Input_text2 = TXT_load_text (Input_file2)

    for order in range (Arguments ['Min Order'], Arguments ['Max Order'] + 1):
        # Create an empty (unprimed) dynamic model
        Model1 = TLM_create_PPM_model (b"PPM Model", Arguments ['Alphabet Size'], order,
		                      Arguments ['Escape Method'])
        Model2 = TLM_create_PPM_model (b"PPM Model", Arguments ['Alphabet Size'], order,
		                      Arguments ['Escape Method'])

        plotCodelengthRatioText (Model1, Model2, Input_text1, Input_text2, "Order " + str(order),
                                 Arguments ['Plot Interval'], log_scales)

    plt.legend () #(loc = 'lower left')
    plt.show()

    TLM_release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
