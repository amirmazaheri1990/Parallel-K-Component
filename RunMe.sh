rm main.out
reset
g++ -pthread -std=c++11 Parallel.cpp -o main.out
./main.out < input.txt
