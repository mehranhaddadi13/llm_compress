python3 word-codelengths.py -i eee.txt -m bible.model >jjj
echo "Using bible tokens model on eee.txt:"
sort <jjj
python3 word-codelengths.py -i eee.txt -m bible_types.model >jjj
echo "Using bible types model on eee.txt:"
sort <jjj
#
python3 word-codelengths.py -i ../data/bible/english.txt -m bible.model >jjj
echo "Using bible tokens model on the bible:"
sort <jjj
python3 word-codelengths.py -i ../data/bible/english.txt -m bible_types.model >jjj
echo "Using bible types model on the bible:"
sort <jjj
