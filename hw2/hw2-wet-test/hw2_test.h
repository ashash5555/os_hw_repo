#ifndef test_hw2_H_
#define test_hw2_H_

#include <iostream>
#include <chrono>
#include <unistd.h>
#include <time.h>
#include <cassert>
using namespace std::chrono;
using std::cout;
using std::endl;

// System call wrappers
double get_vruntime(); // returns vruntime in seconds
void increment_vruntime(double delta); // gets delta in seconds

// Auxiliary functions
double AssertRelativeError(double theoretical, double measured, double tolerance = 0.1);

// Auxiliary class
class Stopwatch {
    public:
        Stopwatch();
        void Reset();
        double Read();

    private:
        time_point<steady_clock> time0;
};

#endif // test_hw2_H_

