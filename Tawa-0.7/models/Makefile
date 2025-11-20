BIN=../apps/train
DATA=../data

./english.model : $(DATA)/bible/english.txt ;
	$(BIN)/train -i $(DATA)/bible/english.txt -o english.model -S -p 1000000 -O 5 -e 'D' -T "English model" -a 256

./french.model : $(DATA)/bible/french.txt ;
	$(BIN)/train -i $(DATA)/bible/french.txt -o french.model -S -p 1000000 -O 5 -e 'D' -T "French model" -a 256

./german.model : $(DATA)/bible/german.txt ;
	$(BIN)/train -i $(DATA)/bible/german.txt -o german.model -S -p 1000000 -O 5 -e 'D' -T "German model" -a 256

./italian.model : $(DATA)/bible/italian.txt ;
	$(BIN)/train -i $(DATA)/bible/italian.txt -o italian.model -S -p 1000000 -O 5 -e 'D' -T "Italian model" -a 256

./latin.model : $(DATA)/bible/latin.txt ;
	$(BIN)/train -i $(DATA)/bible/latin.txt -o latin.model -S -p 1000000 -O 5 -e 'D' -T "Latin model" -a 256

./spanish.model : $(DATA)/bible/spanish.txt ;
	$(BIN)/train -i $(DATA)/bible/spanish.txt -o spanish.model -S -p 1000000 -O 5 -e 'D' -T "Spanish model" -a 256

./chinese.model : $(DATA)/misc/chinese.txt ;
	$(BIN)/train -i $(DATA)/misc/chinese.txt -o chinese.model -S -p 1000000 -O 5 -e 'D' -T "Chinese model" -a 256

./maori.model : $(DATA)/misc/maori.txt ;
	$(BIN)/train -i $(DATA)/bible/maori.txt -o maori.model -S -p 1000000 -O 5 -e 'D' -T "Maori model" -a 256

./swedish.model : $(DATA)/misc/swedish.txt ;
	$(BIN)/train -i $(DATA)/misc/swedish.txt -o swedish.model -S -p 1000000 -O 5 -e 'D' -T "Swedish model" -a 256

all: ./english.model ./italian.model  ./chinese.model \
	./french.model ./spanish.model ./latin.model \
	./german.model ./maori.model ./swedish.model


