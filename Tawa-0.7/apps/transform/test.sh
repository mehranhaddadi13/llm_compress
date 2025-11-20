#
./markup -m models.dat < ../../data/demo/e_and_m.txt -p 100 -V -F -d 1 >& jjj
grep Min jjj >jjj1
./markup -m models1.dat < ../../data/demo/e_and_m.txt -p 100 -V -F -d 1 >& jjj
grep Min jjj >jjj2
./markup -m models2.dat < ../../data/demo/e_and_m.txt -p 100 -V -F -d 1 >& jjj
grep Min jjj >jjj3
cat jjj1 jjj2 jjj3
