#
# For testing out the execution speeds of various toolkit commands.
i=0
while [ $i -lt 10 ]
do
  echo $i
  ./segment -m Brown.model -D 50 -i lll1.txt -o lll1_out.txt
  (( i++ ))
done
