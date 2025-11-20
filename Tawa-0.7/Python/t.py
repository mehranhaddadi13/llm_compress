import ctypes

libTawa = ctypes.CDLL('pyTawa.so')

#TLM_release_models = lib.TLM_release_models
#
#TLM_release_models()

def TLM_get_codelength():
    return ctypes.c_float.in_dll(libTawa, "TLM_Codelength").value

def TLM_set_codelength(value):
    ctypes.c_float.in_dll(libTawa, "TLM_Codelength").value = value

print ("codelength =", TLM_get_codelength()) 
TLM_set_codelength (99.9)
print ("codelength =", TLM_get_codelength()) 

print (ctypes.c_float.in_dll(libTawa, "TLM_Get_Codelength").value)
