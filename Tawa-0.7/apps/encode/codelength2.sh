# Shell script for testing probs program.
../train/train -a 256 -O 5 -e D -T "Test" -i Doc.1 -o Doc.model -S
./codelength1 -m Doc.model <Doc.1
