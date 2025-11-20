VERSION = 1.0

.PHONY: demo train

all:
	(cd Tawa; make -f Makefile install INCLUDES=${})
	(cd apps; make -f Makefile)

clean:
	(cd Tawa; make -f Makefile clean)
	(cd apps; make -f Makefile clean)
	(rm -f include/*)
	(find lib -maxdepth 1 -type f -exec rm {} \;)
	(find . -name "core" -exec rm {} \;)
	(find . -name "*~" -exec rm {} \;)

config:
	(cd configdir; make -f Makefile config)

train:	all
	(cd models; make -f Makefile all)

demo:	train
	(cd apps/classify; ./ident_lang -m models.dat)

dist:	clean
	(cd ..; rm -f Tawa-${VERSION}.tar.gz; \
	 tar cvf Tawa-${VERSION}.tar Tawa-${VERSION}; \
	 gzip -v9 Tawa-${VERSION}.tar)
