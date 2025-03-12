gcc -Wall boating.c -o boating
p1=$((RANDOM % 20 + 1))
p2=$((RANDOM % 100 + 60))
echo "Running with parameters: $p1 $p2"
./boating $p1 $p2
