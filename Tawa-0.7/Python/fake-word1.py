# This method select a random word from a word list first to create a random word.

# Can use NLTK's larger word list:
from nltk.corpus import words
# Instead use Oxford 2000 advanced learner's word list

import random

import Tawa
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

def get_word_list(word_list_filename):
    """ Returns a list of words from the file words-list-filename. """
    
    with open(word_list_filename) as f:
        return f.read().split()

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Model Filename' in Arguments):
        model_filename = Arguments ['Model Filename']
        Model = Tawa.load_model (model_filename)

    if (Tawa.numberof_models () < 1):
      usage()

    # Choose a random word from a word list first
    # Could use word.words() here as the word list
    # Instead select word from the Oxford 2000 advanced learner's word list
    word_list = get_word_list ("Oxford-2000.txt")
    
    print ("Randomly choosing from", len(word_list), "words")
    while True:
        word = random.choice (word_list)
        if (len (word) < 9) and (not word [0].isupper()):
            print ("random word =", word)
            break

    # Creating a list of approx. 100 random new words similar to word
    new_words = []
    for i in range(100):
        p = 0
        new_word_lst = list (word)
        for ch in word:
            if (ch in 'aeiou') and (p < len(word)): # Don't swap vowel at the end of the word
                ch = random.choice ('aeiou')
            if (ch in 'bdpt'):
                ch = random.choice ('bdpt')
            if (ch in 'ck'):
                ch = random.choice ('ck')
            if (ch in 'mn'):
                ch = random.choice ('mn')
            #if (ch in 'sz'):
            #    ch = random.choice ('sz')
            new_word_lst [p] = ch
            p += 1

        new_word = ''.join(new_word_lst)

        # Check that the "new" word doesn't already exist
        if (new_word not in words.words()):
            new_words += [new_word]

    print (len (new_words), "new words:")

    for n, wd in enumerate(new_words, 1):
        print (wd, ' ', end='') 
        if n % 5 == 0:
            print ('')
    print ('')

    print ("Choosing the most compressible of these:")
    best_new_word = ''
    best_codelength = 100000

    for new_word in new_words:
        codelength = Tawa.codelength_string (Model, new_word)
        if (codelength < best_codelength):
            best_codelength = codelength
            best_new_word = new_word

    print ("Best new word [", best_new_word, "] Best Codelength = {0:5.2f}".format(best_codelength))
    

if __name__ == '__main__':
    main()
