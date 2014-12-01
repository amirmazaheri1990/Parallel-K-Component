rm main.out
reset
g++ -pthread Parallel.cpp -o main.out
./main.out < input.txt
