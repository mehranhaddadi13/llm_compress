python3 word-types.py -i ../data/bible/english.txt >bible_types.txt
python3 train.py -S -T "Title of model" -O 5 -i bible_types.txt -o bible_types.model -p 100000
