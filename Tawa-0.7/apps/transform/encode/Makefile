include ../../../CONFIG

Tmarkup:	Tmarkup.o
	$(CXX) $(CXXFLAGS) -o Tmarkup Tmarkup.o $(LIBS)

Tstats:	Tstats.o
	$(CXX) $(CXXFLAGS) -o Tstats Tstats.o $(LIBS)

Tencode:	Tencode.o
	$(CXX) $(CXXFLAGS) -o Tencode Tencode.o $(LIBS)

Tdecode:	Tdecode.o
	$(CXX) $(CXXFLAGS) -o Tdecode Tdecode.o $(LIBS)

demo:	Tmarkup Tstats
	(./Tmarkup -m models.dat -p 1000 -V <../../../data/demo/e_and_m.txt >demo.output; Tstats -m models.dat <demo.output; ./Tmarkup -m models.dat -p 1000 -V -M 10 <../../../data/demo/e_and_m.txt >demo.output.1; ./Tstats -m models.dat -M 10 <demo.output.1)

clean:
	rm -f *.o Tmarkup Tstats Tencode Tdecode *~
