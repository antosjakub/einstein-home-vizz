
echo "IN:  $1.cpp"
echo "OUT: $1.out"

g++ $1.cpp -o $1.out $(pkg-config --cflags --libs glfw3) -lGL