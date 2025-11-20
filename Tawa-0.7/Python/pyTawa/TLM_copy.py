""" Tawa's TLM routines for language models. """

import ctypes

c_int = ctypes.c_int
c_uint = ctypes.c_uint
c_char = ctypes.c_char
c_char_p = ctypes.c_char_p

libTawa = ctypes.CDLL('pyTawa.so')

# Some constants / default values
TLM_Static = 0         # Indicates model is static
TLM_Dynamic = 1        # Indicates model is dynamic i.e. adaptive

# Defines the type of modelling algorithms
TLM_PPM_Method_A = 0   # Indicates PPM model uses escape method A
TLM_PPM_Method_B = 1   # Indicates PPM model uses escape method B
TLM_PPM_Method_C = 2   # Indicates PPM model uses escape method C
TLM_PPM_Method_D = 3   # Indicates PPM model uses escape method D

# Defines the default argument values for PPM models
TLM_PPM_ALPHABET_SIZE = 256  # Default size of the model's alphabet
TLM_PPM_MAX_ORDER = 5        # Default max. order of the model */
TLM_PPM_ESCAPE_METHOD = TLM_PPM_Method_D # Default escape method for PPM model
TLM_PPM_PERFORMS_FULL_EXCLS = 1   # Performs full exclusions by default
TLM_PPM_PERFORMS_UPDATE_EXCLS = 1 # Performs update exclusions by default
BREAK_SYMBOL = 10    # Symbol used for forcing a break (usually an eoln)

# Defines the type of modelling algorithms
TLM_PPM_Model = 0      # Indicates model type is PPM ("Prediction by Partial Matching")
TLM_SSS_Model = 9      # Uses start-step-stop codes
TLM_TAG_Model = 10     # Tag based model (i.e. parts of speech)

# Operations for TLM_next_symbol, TLM_find_symbol and TLM_update_context routines.
TLM_Get_Nothing = 0    # Default operation; no additional information is returned
TLM_Get_Codelength = 1 # Returns the symbol's codelength i.e. cost in bits of encoding the symbol
TLM_Get_Coderanges = 2 # Returns the symbol's coderanges i.e. arithmetic coding ranges
TLM_Get_Maxorder = 3   # Returns codelength assuming max order context only i.e. no escapes occur

# Operations for TLM_load_model set by TLM_set_load_operation.
TLM_Load_No_Operation = 0       # Default operation; no additional information is used
TLM_Load_Change_Model_Type = 1  # The following parameter is used to specify the new type that the model is transformed into as it is loaded.
TLM_Load_Change_Title = 2       # Specifies that the following parameter is used to change themtitle of the model after it gets loaded.

# Operations for TLM_write_model set by TLM_set_write_operation.
TLM_Write_No_Operation = 0      # Default operation; no additional information is used
TLM_Write_Change_Model_Type = 1 # The following parameter is used to specify the new type that the model is transformed into as it is written out.

def TLM_valid_model (model):
    """ Returns non-zero if the model is valid, zero otherwize. """
    return (bool(libTawa.TXT_valid_model (model)))

def TLM_valid_coder (coder):
    """ Returns a non-zero coder number if the coder is a valid coder, zero otherwize. """
    return (libTawa.TLM_valid_coder (coder))

def TLM_create_arithmetic_coder ():
    """ Creates and returns the default arithmetic coder. """
    return (libTawa.TLM_create_arithmetic_coder ())

def TLM_create_arithmetic_encoder (input_fileid, output_fileid):
    """ Creates and returns the arithmetic encoder. """
    return (libTawa.TLM_create_arithmetic_encoder (input_fileid, output_fileid))

def TLM_create_arithmetic_decoder (input_fileid, output_fileid):
    """ Creates and returns the arithmetic decoder. """
    return (libTawa.TLM_create_arithmetic_decoder (input_fileid, output_fileid))

def TLM_release_coder (coder):
    """ Releases the memory allocated to the coder and the coder number (which may
   be reused in later TLM_create_coder calls). """
    libTawa.TLM_release_coder (coder)

def TLM_valid_context (model, context):
    """ Returns non-zero if the model is valid, zero otherwize. """
    return (bool(libTawa.TLM_valid_context (model, context)))

def TLM_create_context (model):
    """ Creates and returns an unsigned integer which provides a reference to a
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. """
    libTawa.TLM_create_context.restype=c_uint
    libTawa.TLM_create_context.argtypes=[c_uint]
    return (libTawa.TLM_create_context (model))

def TLM_copy_context (model, context):
    libTawa.TLM_copy_context.restype=c_uint
    libTawa.TLM_copy_context.argtypes=[c_uint,c_uint]
    """ Creates a new context record, copies the contents of the specified context
   into it, and returns an integer reference to it. A run-time error occurs
   if the context being copied is for a dynamic model. """
    return (libTawa.TLM_copy_context (model, context))

def TLM_clone_context (model, context):
    """ Creates a new context record, copies the contents of the specified context
   into it, and returns an integer reference to it. A run-time error does not
   occur if the context being copied is for a dynamic model. """
    return (libTawa.TLM_clone_context (model, context))

def TLM_overlay_context (model, old_context, context):
    """ Overlays the context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. """
    libTawa.TLM_overlay_context (model, old_context, context)

def TLM_set_context_operation (context_operation):
    """ Sets the type of operation to be performed by the routines
   TLM_next_symbol, TLM_find_symbol and TLM_update_context.
   The argument operation_type is one of the following:
     TLM_Get_Codelength
	 Returned in the global variable TLM_Codelength is a float value 
	 which is set to the codelength for encoding the specified symbol
	 (i.e. the cost in bits of encoding it given the current context).
     TLM_Get_Coderanges
         Returned in the global variable TLM_Coderanges is an unsigned int
	 pointer to the list of arithmetic coding ranges required for
	 encoding the specified symbol given the current context.
     TLM_Get_Maxorder
	 Returned in the global variable TLM_Codelength is a float value 
	 which is set to the codelength for encoding the specified symbol
	 (i.e. the cost in bits of encoding it given the current context)
	 assuming only the maxorder symbols are being coded (i.e. that no
	 escapes occur). """
    libTawa.TLM_set_context_operation (context_operation)

def TLM_symbol():
    return ctypes.c_uint.in_dll(libTawa, "TLM_Symbol").value

def TLM_get_codelength():
    return ctypes.c_float.in_dll(libTawa, "TLM_Codelength").value

def TLM_set_codelength(value):
    ctypes.c_float.in_dll(libTawa, "TLM_Codelength").value = value

def TLM_get_PPM_title ():
    return ctypes.c_uchar_p.in_dll(libTawa, "TLM_PPM_title").value

def TLM_get_PPM_model_type ():
    return ctypes.c_uint.in_dll(libTawa, "TLM_PPM_model_type").value

def TLM_get_PPM_alphabet_size ():
    return ctypes.c_uint.in_dll(libTawa, "TLM_PPM_alphabet_size").value

def TLM_get_PPM_escape_method ():
    return ctypes.c_uchar.in_dll(libTawa, "TLM_PPM_escape_method").value

def TLM_find_symbol (model, context, symbol):
    """ Finds the predicted symbol in the context. """
    libTawa.TLM_find_symbol (model, context, symbol)

def TLM_update_context (model, context, symbol):
    """ Updates the context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. (For faster updates, set the option
   TLM_Get_No_Operation, so that the routine does not return
   any additional information such as the cost of encoding
   the symbol in bits (in TLM_Codelength).

   The ``sentinel symbol'' (whose value is obtained using the
   TXT_sentinel_symbol () routine) is used where there is a break
   required in the updating of the context, such as when the end of
   string has been reached or when more than one model is being used
   to encode different parts of a string. The effect of encoding the
   sentinel symbol is that the prior context is forced to the null
   string i.e. the subsequent context will contain just the sentinel
   symbol itself. This is useful during training if there are statistics
   that differ markedly at the start of some text than in the middle of
   it (for example, individual names, and titles within a long list).

   This routine is often used with the routines TLM_next_symbol,
   TLM_find_symbol. For example,
       TLM_update_context (context, TXT_sentinel_symbol (), ...)
   will update the context record so that the current symbol becomes the
   sentinel symbol. """
    libTawa.TLM_update_context (model, context, symbol)

def TLM_suspend_update (model):
    """ Suspends the update for a dynamic model temporarily (i.e. the
   model becomes a temporary static model and TLM_update_context
   will not update any of the internal statistics of the model.
   The update can be resumed using TLM_resume_update ().

   This is useful if it needs to be determined in advance which
   of two or more dynamic models a sequence of text should be
   added to (based on how much each requires to encode it, say). """
    libTawa.TLM_suspend_update (model)

def TLM_resume_update (model):
    """ Resumes the update for a model. See TLM_suspend_update (). """
    libTawa.TLM_resume_update (model)

def TLM_release_context (model, context):
    """ Releases the memory allocated to the context and the context number (which may
   be reused in later TLM_create_context or TLM_copy_context calls). """
    libTawa.TLM_release_context (model, context)

def TLM_create_coderanges ():
    """ Return a new pointer to a list of coderanges. """
    return (libTawa.TLM_create_coderanges ())

def TLM_append_coderange (coderanges, lbnd, hbnd, totl):
    """ Append a new coderange record onto the tail of the coderange list. """
    libTawa.TLM_append_coderange (coderanges, lbnd, hbnd, totl)

def TLM_reset_coderanges (coderanges):
    """ Resets the position in the list of coderanges associated with the current symbol.
    The next call to TLM_next_coderanges will return the first coderanges on the list. """
    libTawa.TLM_reset_coderanges (coderanges)

def TLM_release_coderanges (coderanges):
    """ Release the coderange list to the used list """
    libTawa.TLM_release_coderanges (coderanges)

def TLM_codelength_coderanges (coderanges):
    """ Returns the code length of the current symbol's coderange in bits. It does this without
   altering the current symbol or the current coderange. """
    return (float (libTawa.TLM_codelength_coderanges (coderanges)))

def TLM_dump_coderanges (fileid, coderanges):
    """ Prints the coderange list for the current symbol in a human readable form.
    It does this without altering the current position in the coderange. list as determined
    by the functions TLM_reset_coderange or TLM_next_coderanges. """
    libTawa.TLM_dump_coderanges (fileid, coderanges)

def TLM_copy_coderanges (coderanges):
    """ Creates a copy of the list of coderanges and returns a pointer to it. """
    return (libTawa.TLM_copy_coderanges (coderanges))

def TLM_reset_symbol (model, context):
    """ Resets the context record to point at the first predicted symbol of the
   current position. """
    libTawa.TLM_reset_symbol (model, context)

def TLM_get_symbol (model, context):
    """ Returns the next predicted symbol in the context and the cost in bits of     
    encoding it. The context record is not updated.                              
                                                                                
    The global variables TLM_Symbol and TLM_Codelength is set to the symbol's    
    value and codelength respectively.                                           
                                                                                
    If a sequence of calls to TLM_get_symbol are made, every symbol in the       
    alphabet will be visited exactly once although the order in which they are   
    visited is undefined being implementation and data dependent. The function   
    returns FALSE when there are no more symbols to process. TLM_reset_symbol    
    will reset the current position to point back at the first predicted symbol  
    of the current context.                                                      
                                                                                
    The codelength value is the same as that returned by TLM_update_context      
    which may use a faster search method to find the symbol's codelength         
    more directly (rather than sequentially as TLM_get_symbol does). A call      
    to TLM_update_context or other routines will have no affect on subsequent    
    calls to TLM_get_symbol. """
    return (libTawa.TLM_get_symbol (model, context))

def TLM_encode_symbol (model, context, symbol, coder):
    """ Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   context becomes the encoded symbol. """
    libTawa.TLM_encode_symbol (model, context, symbol, coder)

def TLM_decode_symbol (model, context, coder):
    """ Returns the symbol decoded using the arithmetic coder. Updates the
   context record so that the last symbol in the context becomes the
   decoded symbol. """
    return (libTawa.TLM_decode_symbol (model, context, coder))

def TLM_create_PPM_model (title = "", alphabet_size = TLM_PPM_ALPHABET_SIZE,
    max_order = TLM_PPM_MAX_ORDER, escape_method = TLM_PPM_ESCAPE_METHOD,
    performs_full_excls = TLM_PPM_PERFORMS_FULL_EXCLS,
    performs_update_excls = TLM_PPM_PERFORMS_UPDATE_EXCLS):
    """ Creates a new empty dynamic PPM model. Returns the new model number allocated
    to it if the model was created successfully, NIL if not.
    The title argument is intended to be a short human readable text description 
    of the origins and content of the model.                                     
    alphabet_size is the size of the alphabet.                                   
    max_order is the maximum order of the PPM model.                             
    escape_method is the escape method being used by the model.
    e.g. TLM_PPM_Method_C for PPMC and TLM_PPM_Method_D for PPMD.
    performs_full_excls is TRUE (1) if the model is to perform full exclusions.
    performs_update_excls is TRUE (1) if the model is to perform update exclusions.
    """
    libTawa.TLM_create_PPM_model.restype=c_uint
    libTawa.TLM_create_PPM_model.argtypes=[c_char_p,c_uint,c_int,c_uint,c_uint,c_uint]
    return (libTawa.TLM_create_PPM_model (title, alphabet_size, max_order, escape_method, performs_full_excls, performs_update_excls))

def TLM_dump_PPM_model (fileid, model):
    """ Prints a human readable version of the PPM model (intended mainly for debugging). """
    libTawa.TLM_dump_PPM_model (fileid, model)

def TLM_load_model (fileid):
    """ Loads a model which has been previously saved to the file into memory and
   allocates it a new model number which is returned. """
    return (libTawa.TLM_load_model (fileid))

def TLM_load_models (filename):
    """ Load the models and their associated tags from the specified file. """
    libTawa.TLM_load_models.argtypes=[c_char_p]
    libTawa.TLM_load_models (filename)

def TLM_read_model (filename, debug_line, error_line):
    """ Reads in the model directly by loading the model from the file
    with the specified filename. """
    libTawa.TLM_read_model.argtypes=[c_char_p,c_char_p,c_char_p]
    libTawa.TLM_read_model (filename, debug_line, error_line)

def TLM_write_model (fileid, model, model_form):
    """ Writes out the model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. """
    libTawa.TLM_write_model (fileid, model, model_form)

def TLM_release_model (model):
    """ Releases the memory allocated to the model and the model number (which may
   be reused in later TLM_create_model or TLM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active contexts pointing at it. """
    libTawa.TLM_release_model (model)

def TLM_copy_model (model):
    """ Copies the model. """
    return (libTawa.TLM_copy_model (model))

def TLM_nullify_model (model):
    """ Replaces the model with the null model and releases the memory allocated
   to it. """
    libTawa.TLM_nullify_model (model)

def TLM_minlength_model (model):
    """ Returns the minimum number of bits needed to write the model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages. """
    return (libTawa.TLM_minlength_model (model))

def TLM_numberof_models ():
    """ Returns the number of currently valid models. """
    return (libTawa.TLM_numberof_models ())

def TLM_reset_modelno ():
    """ Resets the current model number so that the next call to TLM_next_modelno will
   return the first valid model number (or NIL if there are none). """
    libTawa.TLM_reset_modelno ()

def TLM_next_modelno ():
    """ Returns the model number of the next valid model. Returns NIL if
   there isn't any. """
    return (libTawa.TLM_next_modelno ())

def TLM_getcontext_position (model, context):
    """ Returns an integer which uniquely identifies the current position
   associated with the model's context. (One implementation is to return
   a memory location corresponding to the current position. This routine is
   useful if you need to check whether different contexts have encoded
   the same prior symbols as when checking whether the context pathways
   converge in the Viterbi or trellis-based algorithms.) """
    return (libTawa.TLM_getcontext_position (model, context))

def TLM_get_title (model):
    """ Returns the title associated with the model. """
    libTawa.TLM_get_title.restype=c_char_p
    return (libTawa.TLM_get_title (model))

def TLM_set_tag (model, tag):
    """ Sets the tag associated with the model. """
    libTawa.TLM_set_tag.argtypes=[c_uint,c_char_p]
    libTawa.TLM_set_tag (model, tag)

def TLM_get_tag1 (model):
    """ Return the bytestring tag associated with the model. """
    libTawa.TLM_get_tag.restype=c_char_p
    return (libTawa.TLM_get_tag (model))

def TLM_get_tag (model):
    """ Return the tag associated with the model. """
    bytestr = TLM_get_tag1 (model)
    return (bytestr.decode('utf-8'))

def TLM_getmodel_tag (tag):
    """ Returns the model associated with the model's tag. If the tag
   occurs more than once, it will return the lowest model number. """
    libTawa.TLM_getmodel_tag.argtypes=[c_char_p]
    return (libTawa.TLM_getmodel_tag (tag))

def TLM_extend_alphabet_size (model, alphabet_size):
    """ Extends the alphabet size associated with the model. """
    libTawa.TLM_extend_alphabet_size (model, alphabet_size)

def TLM_sizeof_model (model):
    """ Returns the current number of bits needed to store the
   model in memory. """
    return (libTawa.TLM_sizeof_model (model))

def TLM_dump_models1 (fileid):
    """ Writes a human readable version of all the currently valid models to the file. """
    libTawa.TLM_dump_models1 (fileid)

def TLM_release_models ():
    """ Releases the memory used by all the models. """
    libTawa.TLM_release_models ()

def TLM_stats_model (fileid, model):
    """ Writes out statistics about the model in human readable form. """
    libTawa.TLM_stats_model (fileid, model)
