include ../../CONFIG

Tawa.so:
	$(CC) -fPIC -c model.c ppm_model.c ppmo_model.c ppmq_model.c ppm_context.c ppm_trie.c ppm_cache.c ptable.c pt_model.c pt_ptable.c pt_ctable.c cpt_model.c cpt_ptable.c cpt_ctable.c samm_model.c hmm_model.c sss_model.c tag_model.c user_model.c transform.c paths.c confusion.c hash.c tuples.c track.c list.c index.c table.c word.c text.c coderanges.c io.c sets.c coder.c
	$(CC) -shared -o pyTawa.so model.o ppm_model.o ppmo_model.o ppmq_model.o ppm_context.o ppm_trie.o ppm_cache.o ptable.o pt_model.o pt_ptable.o pt_ctable.o cpt_model.o cpt_ptable.o cpt_ctable.o samm_model.o hmm_model.o sss_model.o tag_model.o user_model.o transform.o paths.o confusion.o hash.o tuples.o track.o list.o index.o table.o word.o text.o coderanges.o io.o sets.o coder.o
	cp pyTawa.so ../../pyTawa
	cp pyTawa.so ../../pyTawa/pyTawa

clean:
	rm -f *.o *.so

install: libTawa.a
	cp *.a ../lib
	cp *.h ../include
