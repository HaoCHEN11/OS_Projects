for ((i=1;i<10;++i));
do
  c="test0"
  x=".in"
  ./sdriver.pl -t $c$i$x -s .././tsh
done

for i in {10..34}
do
  c="test"
  x=".in"
  ./sdriver.pl -t $c$i$x -s .././tsh
done
