#include "hw2_test.h"

int main() {
    double timeout = 2.0;
    double vtime0, measured_vruntime;
    double W0 = 1024.0;
    double W3 = 526.0;
    double W5 = 335.0;
    Stopwatch stopwatch;
    for(int i=0;i<10;i++){
        vtime0 = get_vruntime();
        while (stopwatch.Read() < timeout);
        measured_vruntime = get_vruntime() - vtime0;
        AssertRelativeError(timeout, measured_vruntime);
        stopwatch.Reset();
    }
    if (nice(3) == -1)
        return -1;
    for(int i=0;i<10;i++){
        vtime0 = get_vruntime();
        while (stopwatch.Read() < timeout);
        measured_vruntime = get_vruntime() - vtime0;
        AssertRelativeError((W0/W3)*timeout, measured_vruntime);
        stopwatch.Reset();
    }
    if (nice(2) == -1)
        return -1;
    for(int i=1;i<=10;i++){
        vtime0 = get_vruntime();
        while (stopwatch.Read() < i*timeout);
        measured_vruntime = get_vruntime() - vtime0;
        AssertRelativeError((W0/W5)*timeout, measured_vruntime);
    }
    cout << "=====SUCCESS=====" << endl;
    return 0;
}

