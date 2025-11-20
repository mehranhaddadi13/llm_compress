include ../../CONFIG

all: encode decode encode1 decode1 encodeq decodeq codelength codelength1 codelength2 probs probs1 encode_word decode_word

encode:	encode.o
	$(CC) $(CFLAGS) -o encode encode.o $(LIBS)

decode:	decode.o
	$(CC) $(CFLAGS) -o decode decode.o $(LIBS)

encode1:	encode1.o
	$(CC) $(CFLAGS) -o encode1 encode1.o $(LIBS)

decode1:	decode1.o
	$(CC) $(CFLAGS) -o decode1 decode1.o $(LIBS)

encodeq:	encodeq.o
	$(CC) $(CFLAGS) -o encodeq encodeq.o $(LIBS)

decodeq:	decodeq.o
	$(CC) $(CFLAGS) -o decodeq decodeq.o $(LIBS)

word_utils:	word_utils.c
	$(CC) $(CFLAGS) -c word_utils.c

codelength_utils:	codelength_utils.c
	$(CC) $(CFLAGS) -c codelength_utils.c

encode_word:	encode_word.o word_utils.o
	$(CC) $(CFLAGS) -o encode_word encode_word.o word_utils.o $(LIBS)

decode_word:	decode_word.o word_utils.o
	$(CC) $(CFLAGS) -o decode_word decode_word.o word_utils.o $(LIBS)

word1_utils:	word1_utils.c
	$(CC) $(CFLAGS) -c word1_utils.c

encode_number:	encode_number.o
	$(CC) $(CFLAGS) -o encode_number encode_number.o $(LIBS)

decode_number:	decode_number.o
	$(CC) $(CFLAGS) -o decode_number decode_number.o $(LIBS)

encode_pt:	encode_pt.o
	$(CC) $(CFLAGS) -o encode_pt encode_pt.o $(LIBS)

decode_pt:	decode_pt.o
	$(CC) $(CFLAGS) -o decode_pt decode_pt.o $(LIBS)

encode_sss:	encode_sss.o
	$(CC) $(CFLAGS) -o encode_sss encode_sss.o $(LIBS)

decode_sss:	decode_sss.o
	$(CC) $(CFLAGS) -o decode_sss decode_sss.o $(LIBS)

encode_test:	encode_test.o
	$(CC) $(CFLAGS) -o encode_test encode_test.o $(LIBS)

encode_test1:	encode_test1.o
	$(CC) $(CFLAGS) -o encode_test1 encode_test1.o $(LIBS)

encode_EOL:	encode_EOL.o
	$(CC) $(CFLAGS) -o encode_EOL encode_EOL.o $(LIBS)

encode_EOL1:	encode_EOL1.o
	$(CC) $(CFLAGS) -o encode_EOL1 encode_EOL1.o $(LIBS)

codelength:	codelength.o 
	$(CC) $(CFLAGS) -o codelength codelength.o $(LIBS)

codelength1:	codelength1.o 
	$(CC) $(CFLAGS) -o codelength1 codelength1.o $(LIBS)

codelength2:	codelength2.o 
	$(CC) $(CFLAGS) -o codelength2 codelength2.o $(LIBS)

codelength3:	codelength3.o 
	$(CC) $(CFLAGS) -o codelength3 codelength3.o $(LIBS)

codelength4:	codelength4.o 
	$(CC) $(CFLAGS) -o codelength4 codelength4.o $(LIBS)

codelength5:	codelength5.o 
	$(CC) $(CFLAGS) -o codelength5 codelength5.o $(LIBS)

codelengths:	codelengths.o 
	$(CC) $(CFLAGS) -o codelengths codelengths.o $(LIBS)

codelength_line:	codelength_line.o
	$(CC) $(CFLAGS) -o codelength_line codelength_line.o $(LIBS)

probs:	probs.o 
	$(CC) $(CFLAGS) -o probs probs.o $(LIBS)

probs1:	probs1.o 
	$(CC) $(CFLAGS) -o probs1 probs1.o $(LIBS)

wordd:	encode_word decode_word

wordc:
	rm encode_word decode_word

demo: encode decode encode1 decode1
	echo "Encoding the file encode.c using static English model..."
	./encode1 -m /Users/eesa0e/Source/Tawa/Tawa-1.1/models/english.model <encode.c >demo.test1.encoded
	./decode1 -m /Users/eesa0e/Source/Tawa/Tawa-1.1/models/english.model <demo.test1.encoded >demo.test1.decoded
	echo "...check for any differences (there shouldn't be any!):"
	diff encode.c demo.test1.decoded
	echo "\n\n\n"
	echo "Encoding the file encode.c using dynamic model..."
	./encode -a 256 -o 5 -i encode.c -o demo.test2.encoded
	./decode -a 256 -o 5 -i demo.test2.encoded -o demo.test2.decoded
	echo "...check for any differences (there shouldn't be any!):"
	diff encode.c demo.test2.decoded
	echo "\n\n\n"
	echo "Encoding the file junk.txt using static English model..."
	./encode1 -m /Users/eesa0e/Source/Tawa/Tawa-1.1/models/english.model </Users/eesa0e/Source/Tawa/Tawa-1.1/data/demo/junk.txt >demo.test3.encoded
	./decode1 -m /Users/eesa0e/Source/Tawa/Tawa-1.1/models/english.model <demo.test3.encoded >demo.test3.decoded
	echo "...check for any differences (there shouldn't be any!):"
	diff /Users/eesa0e/Source/Tawa/Tawa-1.1/data/demo/junk.txt demo.test3.decoded
	echo "\n\n\n"
	echo "Encoding the file junk.txt using dynamic model..."
	./encode -a 256 -o 5 -i /Users/eesa0e/Source/Tawa/Tawa-1.1/data/demo/junk.txt -o demo.test4.encoded
	./decode -a 256 -o 5 -i demo.test4.encoded -o demo.test4.decoded
	echo "...check for any differences (there shouldn't be any!):"
	diff /Users/eesa0e/Source/Tawa/Tawa-1.1/data/demo/junk.txt demo.test4.decoded

demo_bible: encode_word decode_word
	echo "\n\n\n"
	echo "Encoding the bible using dynamic order 5 character model..."
	./encode -a 256 -o 5 -i /Users/eesa0e/Source/Tawa/Tawa-1.1/data/bible/english.txt -o /dev/null -p 10000

demo_word: encode_word decode_word
	echo "\n\n\n"
	echo "Encoding the file jefferson.1 using dynamic word model..."
	./encode_word -1 1 -2 1 -3 4 -4 4 -p 1000 </Users/eesa0e/Source/Tawa/Tawa-1.1/data/misc/jefferson.1 >demo.test5.encoded
	./decode_word -1 1 -2 1 -3 4 -4 4 -p 1000 <demo.test5.encoded >demo.test5.decoded
	echo "...check for any differences (there shouldn't be any!):"
	diff /Users/eesa0e/Source/Tawa/Tawa-1.1/apps/data/misc/jefferson.1 demo.test5.decoded
	echo "\n\n\n"

demo_word1: encode_word1 decode_word1
	echo "\n\n\n"
	echo "Encoding the file jefferson.1 using dynamic word1 model..."
	./encode_word1 -1 5 -2 2 -p 1000 </Users/eesa0e/Source/Tawa/Tawa-1.1/data/misc/jefferson.1 >demo.test5.encoded1
	./decode_word1 -1 5 -2 2 -p 1000 <demo.test5.encoded1 >demo.test5.decoded1
	echo "...check for any differences (there shouldn't be any!):"
	diff /Users/eesa0e/Source/Tawa/Tawa-1.1/data/misc/jefferson.1 demo.test5.decoded1
	echo "\n\n\n"

clean:
	rm -f *.o encode decode encode1 decode1 encodeq decodeq encode_word decode_word encode_word1 decode_word1 encode_number decode_number encode_sss decode_sss probs codelength codelength1 codelength2 codelength_line *~ demo.*

