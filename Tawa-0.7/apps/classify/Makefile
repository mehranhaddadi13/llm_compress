include ../../CONFIG

all: ident_lang ident_file

classify: classify.o
	$(CC) $(CFLAGS) -o classify classify.o $(LIBS)

ident_lang: ident_lang.o
	$(CC) $(CFLAGS) -o ident_lang ident_lang.o $(LIBS)

ident_file: ident_file.o
	$(CC) $(CFLAGS) -o ident_file ident_file.o $(LIBS)

ident_file1: ident_file1.o
	$(CC) $(CFLAGS) -o ident_file1 ident_file1.o $(LIBS)

ident_file2: ident_file2.o
	$(CC) $(CFLAGS) -o ident_file2 ident_file2.o $(LIBS)

ident_file3: ident_file3.o
	$(CC) $(CFLAGS) -o ident_file3 ident_file3.o $(LIBS)

ident_file4: ident_file4.o
	$(CC) $(CFLAGS) -o ident_file4 ident_file4.o $(LIBS)

ident_file5: ident_file5.o
	$(CC) $(CFLAGS) -o ident_file5 ident_file5.o $(LIBS)

ident_word: ident_word.o
	$(CC) $(CFLAGS) -o ident_word ident_word.o $(LIBS)

radius_file1: radius_file1.o
	$(CC) $(CFLAGS) -o radius_file1 radius_file1.o $(LIBS)

classify_file: classify_file.o
	$(CC) $(CFLAGS) -o classify_file classify_file.o $(LIBS)

demo: classify
	./classify -m models.dat -t testfiles.dat

demo1: ident_lang
	./ident_lang -m models.dat

clean:
	rm -f *.o ident_lang ident_file ident_file1 ident_word ident_min classify_file *~

