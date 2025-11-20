from pyTawa.TXT import *

print ("sym =", TXT_to_lower (65))
print ("sym =", TXT_to_lower (102))
print ("sym =", TXT_to_lower (66))
print ("sym =", TXT_to_lower (89))
print ("sym =", TXT_to_lower (77))

print ("Lower? =", TXT_is_lower (65))
print ("Lower? =", TXT_is_lower (102))
print ("Lower? =", TXT_is_lower (66))
print ("Lower? =", TXT_is_lower (89))
print ("Lower? =", TXT_is_lower (77))

TXT_init_files()

filename = "jjj".encode('ascii')
print (TXT_open_file (filename, b"r", b"Loading text from file",
                      b"Test: can't open text file"))

print (TXT_open_file (b"jjj1", b"r", b"Loading text from file",
                      b"Test: can't open text file"))

print (Stdout_File)
TXT_write_file (Stdout_File, b"Hello World\n")
