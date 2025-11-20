
train-demo:
	rm pyTawa.so
	cp /Users/eesa0e/Source/Tawa/Tawa-1.1/lib/pyTawa/pyTawa.so .
	echo "Train Demo 1"
	python3 train.py -S -i ../data/demo/e_and_m.txt -o test.model -p 1000 -T "Test model"
	echo "Train Demo 2"
	python3 train.py -S -i ../data/bible/german.txt -o german.model -p 1000000 -a 250 -O 5 -e 'D' -T "Order 5 German bible"

codelength-demo:
	echo "Codelength Demo 1:"
	python3 codelength.py -i "testdata.txt" -m "models.dat"
	echo "Codelength Demo 2"
	python3 codelength.py -I "Now is the time" -M german.model

ident-lang-demo:
	echo "Ident_lang Demo"
	echo "Enter some text and the program will try to identify its language..."
	python3 ident_lang.py -m models.dat
