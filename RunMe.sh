rm main.out
reset
g++ -pthread  Parallel.cpp -o main.out
./main.out < graph_1000_6.txt
