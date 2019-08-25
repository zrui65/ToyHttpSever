if [ ! -d "build" ]; then
  mkdir build
fi

mkdir build
cd build
cmake ..
make