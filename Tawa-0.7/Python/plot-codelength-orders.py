""" Plot the compression codelength for different order models for an input text file. """

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
        "  -i\tinput filename=fn",
	"  -L\tuse log scales",
	"  -M n\tmax order of model=n",
	"  -N n\tmin order of model=n",
	"  -p n\tplot codelength every n chars",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-a': ('Alphabet Size', 'Int'),
        '-e': ('Escape Method', 'Str'),
        '-E': ('Plot Entropy', False),
        '-i': ('Input Filename', 'Str'),
        '-L': ('Log Scales', False),
        '-M': ('Max Order', 'Int'),
        '-N': ('Min Order', 'Int'),
        '-p': ('Plot Interval', 'Int'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    Tawa.init_arguments (arguments, "a:e:Ei:LM:N:p:", ["help"], opts_dict,
                         "Plot-codelength-orders: option not recognized",
                         [['Input Filename']], usage)
    if (arguments ['Min Order'] > arguments ['Max Order']):
        print ("Minimum order is greater then maximum order")
        sys.exit (2)

def plotCodelengthText (model, text, label, plot_interval, plot_entropy, log_scales):
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
            Tawa.plot_values (pos, codelength, sizeof_model, x, y, plot_entropy)

    # Now use the sentinel symbol to signify the end of the text
    TLM_update_context (model, context, TXT_sentinel_symbol ())
    codelen = TLM_get_codelength ()
    codelength += codelen
    Tawa.plot_values (pos, codelength, sizeof_model, x, y, plot_entropy)

    plt.plot (x, y, label = label)

    TLM_release_context (model, context)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    Tawa.set_default_PPM_arguments (Arguments)

    if not ('Plot Interval' in Arguments):
        Arguments ['Plot Interval'] = 0
    plot_entropy = 'Plot Entropy' in Arguments
    log_scales = 'Log Scales' in Arguments

    Tawa.create_codelength_plot (plot_entropy, log_scales, False)

    filename = Arguments ['Input Filename']
    Input_text = Tawa.load_text (filename)

    for order in range (Arguments ['Min Order'], Arguments ['Max Order'] + 1):
        # Create an empty (unprimed) dynamic model
        Model = Tawa.create_PPM_model (Arguments ['Alphabet Size'], order, Arguments ['Escape Method'])
        Tawa.plot_codelength_text (Model, Input_text, "Order " + str(order), Arguments ['Plot Interval'],
                                   plot_entropy, log_scales, False)

    plt.legend () #(loc = 'lower left')
    plt.show()

    Tawa.release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
