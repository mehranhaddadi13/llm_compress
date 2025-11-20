""" Support routines for processing command line arguments. """

import getopt, sys

def TAR_check_argument (arguments, opt_key, opt_val):
    """ Returns True if the opt_key is in the arguments dictionary, and its
        value equals opt_val. """
    if (not opt_key in arguments):
        return False
    else:
        return (arguments [opt_key] == opt_val)

def TAR_check_required_arguments (arguments, opts_dict, opt_key_list):
    """ Returns True if all the opt_keys in the opt_key_list are in the
    arguments dictionary. If any of them are not, a fatal error is generated. """

    ok = True
    for opt_keys in opt_key_list:
        if (not isinstance (opt_keys, list)):
            # Single required option found
            if (not opt_keys in arguments):
                ok = False
                print ("Fatal error: missing", opt_keys, file=sys.stderr)
        else: # list of optional required aruments found (one of them needs to be present)
            ok1 = False
            for opt_key in opt_keys:
                if (opt_key in arguments):
                    ok1 = True
            if (not ok1):
                ok = False
                print ("Fatal error: missing one of", opt_keys, file=sys.stderr)
    return ok

def TAR_set_argument_defaults (arguments, opts_defaults):
    """ Sets the default values for any missing non-required arguments. """

    for opt_key, opt_default_val in opts_defaults:
        if (not opt_key in arguments):
            arguments [opt_key] = opt_default_val

def TAR_get_argument (arguments, opt_key):
    """ Returns value associated with the opt_key is in the arguments
    dictionary, 0 if it does not exist. """
    if (not opt_key in arguments):
        return 0
    else:
        return (arguments [opt_key])

def insert_argument_string (arguments, opt_key, opt_val):
    # Inserts the opt_val into opt_key if it is a non-zero length string
    # and converts it to ascii. """
    if (isinstance(opt_val, str) and (len (opt_val) > 0)):
        arguments [opt_key] = opt_val.encode('ascii')

def TAR_init_arguments (arguments, shortopts, longopts, opts_dict, err_message):
    """ Initialises the arguments dictionary from the command line. """
    try:
        opts, args = getopt.getopt(sys.argv[1:], shortopts, longopts)
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err_message, file=sys.stderr)
        sys.exit(2)

    ok = True
    for opt, arg in opts:
        if (not opt in opts_dict):
            print (err_message, file=sys.stderr)
            ok = False
        else:
            opt_key, opt_type = opts_dict [opt]
            match opt_type:
                case True | False:
                    arguments [opt_key] = opt_type
                case 'Int':
                    arguments [opt_key] = int(arg)
                case 'Ord':
                    arguments [opt_key] = ord(arg)
                case 'Str':
                    insert_argument_string (arguments, opt_key, arg)
                case 'Exit' | _:
                    ok = False;
    return (ok)
