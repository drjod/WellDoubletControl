cd build
cmake -Dlogging=$1 ..
make
./run_tests

cd ..
