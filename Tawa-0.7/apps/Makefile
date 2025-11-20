
all:
	(cd classify; make -f Makefile)
	(cd encode; make -f Makefile)
	(cd transform; make -f Makefile)
	(cd train; make -f Makefile)

clean:
	(cd classify; make -f Makefile clean)
	(cd encode; make -f Makefile clean)
	(cd transform; make -f Makefile clean)
	(cd train; make -f Makefile clean)

demo:
	(cd encode; make -f Makefile demo)
