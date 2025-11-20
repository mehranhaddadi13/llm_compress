with open("../data/bible/eng.txt") as textfile1, open("../data/bible/fre.txt") as textfile2: 
    for l1, l2 in zip(textfile1, textfile2):
        str1 = l1.strip()
        str2 = l2.strip()
        print(str1)
        print(str2)
