#include <cmath>
#include "hw2_test.h"

using namespace std;

const double us_to_s = 1e-6;
const double ns_to_s = 1e-9;
const double s_to_ns = 1e+9;

double get_vruntime() {
    long vruntime_ns = syscall(334);
    assert(vruntime_ns > 0);
    double vruntime = vruntime_ns * ns_to_s;
    return vruntime;
}

void increment_vruntime(double delta) {
    long delta_ns = static_cast<long>(delta * s_to_ns);
    long r = syscall(335, delta_ns);
    assert(r == 0);
    return;
}

double AssertRelativeError(double theoretical, double measured, double tolerance) {
    double relative_error = abs((measured - theoretical) / theoretical);
    assert(relative_error < tolerance);
    return 0;
}

Stopwatch::Stopwatch() {
    Reset();
}

void Stopwatch::Reset() {
    time0 = steady_clock::now();
}

double Stopwatch::Read() {
    duration<double> duration = steady_clock::now() - time0;
    return duration.count();
}

