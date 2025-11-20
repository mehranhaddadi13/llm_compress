config:	configfile copyright install readme

configfile:
	(echo -n "HOME = " >CONFIG.tmp)
	(cd ..; pwd >>configdir/CONFIG.tmp; cd configdir)
	(cat CONFIG.tmp CONFIG >../CONFIG)
	(rm CONFIG.tmp)

copyright:
	(echo "Copyright (C) 2002 Bill Teahan" >COPYRIGHT.tmp)
	(cat COPYRIGHT.tmp EMAIL.ADDRESS COPYRIGHT >../COPYRIGHT)
	(rm COPYRIGHT.tmp)

install:
	(echo -n "This package is currently maintained by Bill Teahan " >INSTALL.tmp)
	(cat INSTALL.tmp EMAIL.ADDRESS INSTALL >../INSTALL)
	(rm INSTALL.tmp)

readme:
	(cat EMAIL.ADDRESSES README EMAIL.ADDRESS >../README)

clean:
	(rm -f *.tmp)
	(rm -f ../CONFIG)
	(rm -f ../COPYRIGHT)
	(rm -f ../INSTALL)
	(rm -f ../README)
