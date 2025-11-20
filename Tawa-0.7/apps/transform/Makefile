include ../../CONFIG

segment:	segment.o
	$(CXX) $(CXXFLAGS) -o segment segment.o $(LIBS)

segment_eval:	segment_eval.o
	$(CXX) $(CXXFLAGS) -o segment_eval segment_eval.o

segment1:	segment1.o
	$(CXX) $(CXXFLAGS) -o segment1 segment1.o $(LIBS)

segment2:	segment2.o
	$(CXX) $(CXXFLAGS) -o segment2 segment2.o $(LIBS)

markup:	markup.o
	$(CXX) $(CXXFLAGS) -o markup markup.o $(LIBS)

markup1:	markup1.o
	$(CXX) $(CXXFLAGS) -o markup1 markup1.o $(LIBS)

markup2:	markup2.o
	$(CXX) $(CXXFLAGS) -o markup2 markup2.o $(LIBS)

markup-demo:	markup
	./markup -m models.dat < /Users/eesa0e/Source/Tawa/Tawa-1.1/data/demo/e_and_m.txt -p 100 -V -F

segment-demo:	segment
	./segment -m /Users/eesa0e/Source/Tawa/Tawa-1.1/models/english.model -V -i /Users/eesa0e/Source/Tawa/Tawa-1.1/data/demo/segment_English.txt -p 250 -o eee.txt

segment-demo1:	segment
	./segment -m ../../models/chinese.model -V -i ../../data/demo/segment_Chinese.txt -p 250 -o ccc.txt

clean:
	rm -f *.o *.exe segment markup segment1 markup1 Tmarkup Tstats Tencode Tdecode *~
