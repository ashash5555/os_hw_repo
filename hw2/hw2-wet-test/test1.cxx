#include "hw2_test.h"

int main() {
    double timeout = 5.0;
    double vtime0 = get_vruntime();
    Stopwatch stopwatch;
    while (stopwatch.Read() < timeout); // spin
    double measured_vruntime = get_vruntime() - vtime0;
    AssertRelativeError(timeout, measured_vruntime);
    cout << "===== SUCCESS =====" << endl;
    return 0;
}

