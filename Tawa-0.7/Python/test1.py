from pyTawa.TLM import *

print ("codelength =", TLM_get_codelength()) 
TLM_set_codelength (99.9)
print ("codelength =", TLM_get_codelength()) 

TLM_release_models ()
