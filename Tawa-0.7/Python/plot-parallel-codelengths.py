""" This is not completed yet. """

""" Plot the compression codelengths between two input text files (useful for comparing
    the complexity between parallel text files). It is assumed the two text files are aligned
    according to each corresponding lines in the text files. """

from collections import Counter

import Tawa

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
        "  -1\tmodel filename 1=fn",
        "  -2\tmodel filename 2=fn",
        "  -3\tinput filename 1=fn",
        "  -4\tinput filename 2=fn",
        "  -D\tplot codelength differences vs file position",
        "  -P\tplot percentages of codelength differences",
        "  -R\tplot codelength ratios vs file position",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-1': ('Model Filename 1', 'Str'),
        '-2': ('Model Filename 2', 'Str'),
        '-3': ('Input Filename 1', 'Str'),
        '-4': ('Input Filename 2', 'Str'),
        '-x': ('X Axis Label', 'Str'),
        '-y': ('Y Axis Label', 'Str'),
        '-D': ('Plot Differences', False),
        '-P': ('Plot Percentages', False),
        '-R': ('Plot Ratios', True),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    ok = TAR_init_arguments (arguments,
        "DPR1:2:3:4:x:y:", ["help"], opts_dict,
        "Plot-parallel-codelengths: option not recognized")
    if (not ok):
        usage()
        sys.exit(2)

    if (not TAR_check_required_arguments (arguments, opts_dict,
        [['Model Filename 1'], ['Model Filename 2'], ['Input Filename 1'], ['Input Filename 2'],
         ['X Axis Label'], ['X Axis Label']])):
        usage ()
        sys.exit(2)

def plot_parallel_codelengths (model1, model2, filename1, filename2, x_label, y_label,
                               plot_ratios, plot_differences, plot_percentages):
    """ Plots the codelength (in bits) for encoding the texts using the PPM
    models. """

    x = []
    y = []

    differences = []
    filepos = 0
    with open(filename1) as textfile1, open(filename2) as textfile2: 
        for l1, l2 in zip(textfile1, textfile2):
            str1 = l1.strip()
            str2 = l2.strip()

            codelen1 = Tawa.codelength_string (model1, str1)
            codelen2 = Tawa.codelength_string (model2, str2)
            ratio = codelen1 / codelen2
            difference = codelen1 - codelen2
            differences += [round(difference)]

            if (not plot_percentages):
                if (plot_differences):
                    x += [codelen1]
                    y += [codelen2]
                else:
                    x += [filepos]
                    if (plot_ratios):
                        y += [100 * ratio]
                    else:
                        y += [difference / 100]

            filepos += 1

    if not plot_percentages:
        plt.scatter (x, y, s=20, marker=".", alpha=0.5)
    else:
        diffs = Counter (differences)
        plt.scatter(diffs.keys(), diffs.values(), s=2, alpha=0.4)
    plt.show()

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    plot_ratios = 'Plot Ratios' in Arguments
    plot_codelengths = 'Plot Differences' in Arguments
    plot_percentages = 'Plot Percentages' in Arguments

    plt.title("Plotting codelengths for parallel texts:")

    if (plot_ratios):
        # plot horizontal line at y = 1.0 (this is where most codelength ratio points should cluster)
        plt.axhline(y = 100.0, color = 'r', linestyle = 'dashed')

    input_filename1 = Arguments ['Input Filename 1']
    input_filename2 = Arguments ['Input Filename 2']

    x_label = Arguments ['X Axis Label'].decode()
    y_label = Arguments ['Y Axis Label'].decode()

    plt.xlabel(x_label)
    plt.ylabel(y_label)

    if ('Model Filename 1' in Arguments):
        model_filename1 = Arguments ['Model Filename 1']
        model1 = Tawa.load_model (model_filename1)

    if ('Model Filename 2' in Arguments):
        model_filename2 = Arguments ['Model Filename 2']
        model2 = Tawa.load_model (model_filename2)

    plot_parallel_codelengths (model1, model2, input_filename1, input_filename2,
                               x_label, y_label, plot_ratios, plot_codelengths, plot_percentages)

    sys.exit (0)

if __name__ == '__main__':
    main()
