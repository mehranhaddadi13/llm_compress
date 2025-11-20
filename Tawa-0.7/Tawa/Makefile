include ../CONFIG
AR = ar
RANLIB = :

libTawa.a: model.o ppm_model.o ppm_context.o ppm_trie.o ptable.o pt_model.o pt_ptable.o pt_ctable.o cpt_model.o cpt_ptable.o cpt_ctable.o sss_model.o transform.o paths.o confusion.o hash.o table.o word.o text.o coderanges.o io.o coder.o
	$(AR) r $@ model.o ppm_model.o ppm_context.o ppm_trie.o ptable.o pt_model.o pt_ptable.o pt_ctable.o cpt_model.o cpt_ptable.o cpt_ctable.o sss_model.o transform.o paths.o confusion.o hash.o table.o word.o text.o coderanges.o io.o coder.o
	$(RANLIB) $@

clean:
	rm -f *.o *~ *.a bits_test

#coderanges:	coderanges.o
#	$(CXX) $(CXXFLAGS) -o coderanges coderanges.o

backup:
	cp model.c model.c.bak
	cp model.h model.h.bak
	cp ppm_model.c ppm_model.c.bak
	cp ppm_model.h ppm_model.h.bak
	cp ppm_context.c ppm_context.c.bak
	cp ppm_context.h ppm_context.h.bak
	cp ppm_trie.c ppm_trie.c.bak
	cp ppm_trie.h ppm_trie.h.bak
	cp ptable.c ptable.c.bak
	cp ptable.h ptable.h.bak
	cp pt_model.c pt_model.c.bak
	cp pt_ptable.c pt_ptable.c.bak
	cp pt_ptable.h pt_ptable.h.bak
	cp cpt_model.c cpt_model.c.bak
	cp cpt_ptable.c cpt_ptable.c.bak
	cp cpt_ptable.h cpt_ptable.h.bak
	cp sss_model.c sss_model.c.bak
	cp sss_model.h sss_model.h.bak
	cp table.c table.c.bak
	cp table.h table.h.bak
	cp text.c text.c.bak
	cp text.h text.h.bak
	cp word.c word.c.bak
	cp word.h word.h.bak
	cp coderanges.c coderanges.c.bak
	cp coderanges.h coderanges.h.bak
	cp transform.c transform.c.bak
	cp transform.h transform.h.bak
	cp confusion.c confusion.c.bak
	cp confusion.h confusion.h.bak
	cp hash.c hash.c.bak
	cp hash.h hash.h.bak
	cp io.c io.c.bak
	cp io.h io.h.bak
	cp coder.c coder.c.bak
	cp coder.h coder.h.bak

install: libTawa.a
	cp *.a ../lib
	cp *.h ../include
