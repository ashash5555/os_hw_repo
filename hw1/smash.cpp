#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include "Commands.h"
#include "signals.h"


///Tests///
///==================================================================================================================///
void testPwd() {
    //test pwd
    cout << "testing pwd..." << endl;
    //normal execution
    GetCurrDirCommand gwd("pwd", true);
    gwd.execute();
    cout << "^ should show current working dir" << endl;

    //make sure other arguments are ignored in pwd
    cout << endl << "testing extra arguments are ignored" << endl;
    gwd = GetCurrDirCommand("pwd 15", true);
    gwd.execute();
    cout << "^ should show current working dir" << endl;
    gwd = GetCurrDirCommand("pwd 15 abc", true);
    gwd.execute();
    cout << "^ should show current working dir" << endl;
    gwd = GetCurrDirCommand("pwd aa", true);
    gwd.execute();
    cout << "^ should show current working dir" << endl;
    cout << "pwd testing over, check the results" << endl << endl << endl;
}
//
//void testCd() {
//
//
//    //testing cd - check arguments before running the test
//    cout << "testing cd <path>..." << endl;
//    //check an error pops when using "-" argument w/o changing dirs first
//    ChangeDirCommand cd("cd -", NULL); //check my arguments!
//    cd.execute();
//    cout << "an error for no OLDPWD should be printed" << endl;
//
//    //normal execution of cd ..
//    cout << endl << "testing cd .." << endl;
//    gwd.execute();
//    cd = ChangeDirCommand("cd ..", "/home/student", true);
//    cd.execute();
//    gwd.execute();
//    cout << "should show current working dir was changed by .." << endl;
//
//    //normal execution of cd -
//    cout << endl << "testing cd -" << endl;
//    cd = ChangeDirCommand("cd -", "/home", true);
//    cd.execute();
//    gwd.execute();
//    cout << "should show that the current working dir is /home/student" << endl;
//    cd = ChangeDirCommand("cd -", "/home/student", true);
//    cd.execute();
//    gwd.execute();
//    cout << "should show that the current working dir is /home" << endl;
//
//    //normal execution of cd <path>
//    count << endl << "testing cd <path>" << endl;
//    cd = ChangeDirCommand("cd /home/student", "/home/student", true);
//    cd.execute();
//    gwd.execute();
//    out << "should show that the current working dir is /home/student" << endl;
//
//    //error checking for cd
//    cout << endl << "testing cd <path> errors" << endl;
//    cout << "checking cd .. .." <<  endl;
//    cd = ChangeDirCommand("cd .. ..", "/home", true);
//    cd.execute();
//    cout << "should show too many arguments error" << endl;
//    gwd.execute();
//    cout << "make sure working directory is still /home/student" << endl;
//    cout << "checking cd - .." <<  endl;
//    cd = ChangeDirCommand("cd - ..", "/home", true);
//    cd.execute();
//    cout << "should show too many arguments error" << endl;
//    gwd.execute();
//    cout << "make sure working directory is still /home/student" << endl;
//    cout << "checking cd .. -" <<  endl;
//    cd = ChangeDirCommand("cd /home ..", "/home", true);
//    cd.execute();
//    cout << "should show too many arguments error" << endl;
//    gwd.execute();
//    cout << "make sure working directory is still /home/student" << endl;
//    cout << "checking that OLDPWD didn't change while errors happened" << endl;
//    cd = ChangeDirCommand("cd -", "/home/student", true);
//    cd.execute();
//    gwd.execute();
//    cout << "current working directory should be /home/student" << endl;
//
//    //check perror
//    cout << endl << "checking perror message" << endl;
//    cd = ChangeDirCommand("cd abc/asdf/sadfasfe", "/home", true);
//    cd.execute();
//    cout << "perror message should be printed(?) smash error: chdir() failed" << endl;
//    cout << "checking that OLDPWD didn't change while perror happened" << endl;
//    cd = ChangeDirCommand("cd -", "/home/student", true);
//    cd.execute();
//    gwd.execute();
//    cout << "current working directory should be /home/student" << endl;
//    cout << "cd <path> testing over, check the results" << endl << endl << endl;
//
//}
//
//
//void testJobs() {
//    JobsList* jl = new JobsList();
//    jl->addJob(nullptr,-1);
//    jl->addJob(nullptr,-2,true);
//    jl->updateLastStoppedJob();
//    if (jl->getLastStoppedJob()->getJobPid() != -2) cout << "last stopped job didn't update correctly" << endl;
//    if (jl->getNumberOfJobs() != 2) cout << "number of jobs didn't update correctly" << endl;
//    if (jl->getLastJob()->getJobPid() != -1) cout << "last job didn't update correctly" << endl;
//    if (jl->getJobById(1)->getJobPid() != -1) cout << "get job by id didn't work correctly" << endl;
//    cout << "2 jobs should be printed:" << endl;
//    jl->printJobsList();
//    jl->removeJobById(1);
//    cout << "1 job should be printed:" << endl;
//    jl->printJobsList();
//    if (jl->getNumberOfJobs() != 1) cout << "the number of jobs didn't update correctly after remove by id" << endl;
//    jl->removeFinishedJobs();
//    cout << "no job should be printed" << endl;
//    jl->printJobsList();
//    cout << "the number of jobs in the list is: " << jl->getNumberOfJobs() << endl;
//    jl->addJob(nullptr,95685);
//    jl->addJob(nullptr,95686,true);
//    delete jl;
//}
//
//void testKill() {
//
//}

///==================================================================================================================///










int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = &alarmHandler;
    act.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &act, NULL)) {
        perror("smash error: failed to set alarm handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.getPrompt(); // TODO: change this (why?)
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}