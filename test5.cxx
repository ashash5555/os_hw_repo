#include "hw2_test.h"
#include <unistd.h>
#include <sys/wait.h>

int main(){
    double before, after;
    for(int i=1; i<=10; i++){
        before = get_vruntime();
        increment_vruntime(5*i);
        after = get_vruntime();
        AssertRelativeError(after, before + 5*i);
    }
    cout << "=====SUCCESS=====" << endl;
    return 0;
}