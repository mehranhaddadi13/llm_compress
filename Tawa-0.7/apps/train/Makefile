include ../../CONFIG

train: train.o
	$(CC) $(CFLAGS) -o train train.o $(LIBS)

train1: train1.o
	$(CC) $(CFLAGS) -o train1 train1.o $(LIBS)

train2: train2.o
	$(CC) $(CFLAGS) -o train2 train2.o $(LIBS)

train_word: train_word.o
	$(CC) $(CFLAGS) -o train_word train_word.o $(LIBS)

train_tag: train_tag.o
	$(CC) $(CFLAGS) -o train_tag train_tag.o $(LIBS)

train_pt: train_pt.o
	$(CC) $(CFLAGS) -o train_pt train_pt.o $(LIBS)

train_ppmo: train_ppmo.o
	$(CC) $(CFLAGS) -o train_ppmo train_ppmo.o $(LIBS)

train_samm: train_samm.o
	$(CC) $(CFLAGS) -o train_samm train_samm.o $(LIBS)

fast_train0: fast_train0.o
	$(CC) $(CFLAGS) -o fast_train0 fast_train0.o $(LIBS)

abel: label.o
	$(CC) $(CFLAGS) -o label label.o $(LIBS)

demo:
	./train -S -i /Users/eesa0e/Source/Tawa/Tawa-1.1/data/bible/german.txt -o german.model -p 10000 -a 256 -O 5 -e 'D' -T "Order 5 German bible"

demo1:
	./train -S -i /Users/eesa0e/Source/Tawa/Tawa-1.1/data/brown.txt -o brown.model -p 10000 -a 256 -O 5 -e 'D' -T "Order 5 Brown Corpus model"


clean:
	rm -f *.o train train1 train2 train_word train_pt train_ppmo samm_train label *~
