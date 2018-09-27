
cd build
rm timer.txt
cmake -Dlogging=$1 ..
make
./run_tests

cat timer.txt
cd ..


