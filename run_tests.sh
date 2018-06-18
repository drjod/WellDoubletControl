cd build
cmake -Dlogging=$1 ..
make
./allTests

cd ..
