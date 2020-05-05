#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <fcntl.h>
#include <algorithm>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
///TODO: Free all the args allocated with malloc
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

bool isInt(const string& s) {
    bool res = s.find_first_not_of("0123456789") == string::npos;
    return res;
}

///Command///
///==================================================================================================================///
Command::Command(const char* cmd_line, bool takes_cpu) : cmd(cmd_line), takes_cpu(takes_cpu) {}
const char* Command::getCommand() const { return cmd.c_str();}
bool Command::takesCPU() const { return takes_cpu;}
///==================================================================================================================///


///BuiltInCommand///
///==================================================================================================================///
BuiltInCommand::BuiltInCommand(const char *cmd_line, bool takesCPU) : Command(cmd_line, takesCPU){}
///==================================================================================================================///


///JobsList///
///==================================================================================================================///
JobsList::JobEntry::JobEntry(int jobID, pid_t pid, const string& cmd, bool isStopped) : jobID(jobID), jobPID(pid),
                                                                                        cmd(cmd), start(time(nullptr)), isStopped(isStopped) {}
int JobsList::JobEntry::getJobID() const { return jobID;}
pid_t JobsList::JobEntry::getJobPID() const { return jobPID;}
string JobsList::JobEntry::getJobCmd() const { return cmd;}
time_t JobsList::JobEntry::getJobStartTime() const { return start;}
bool JobsList::JobEntry::isJobStopped() const { return isStopped;}
void JobsList::JobEntry::setJobID(int id) {jobID = id;}
void JobsList::JobEntry::stopJob() {isStopped = true;}
void JobsList::JobEntry::resumeJob() {isStopped = false;}

/// Prints the job entry as one line: '[<job-id>] <command> : <process-id> <seconds-elapsed> secs (stopped)'
/// \param os - ostream object (file descriptor)
/// \param jobEntry - jobEntry to print
/// \return os
ostream&operator<<(ostream& os, const JobsList::JobEntry& jobEntry) {
    pid_t pid = jobEntry.getJobPID();
    string cmd = jobEntry.getJobCmd();
    int jobID = jobEntry.getJobID();
    bool isStopped = jobEntry.isJobStopped();
    time_t start = jobEntry.getJobStartTime();
    time_t current = time(nullptr);
    int timeElapsed = difftime(current, start) + 0.5;

    os << "[" << jobID << "] " << cmd << " : " << pid << " " << timeElapsed << " secs";

    if (isStopped) {
        os << " (" << "stopped" << ")";
    }

    os << endl;
    return os;
}

JobsList::JobsList() : jobs(vector<JobsList::JobEntry*>()), jobsCount(0) {}
JobsList::~JobsList() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    while (it != jobs.end()) {
        if (*it) {
            delete *it;
            *it = nullptr;
            /// because iterator is undefined after erase
            it = jobs.erase(it);
        } else {
            ++it;
        }
    }
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    this->removeFinishedJobs();
    this->jobsCount++;
    int jobID = jobsCount;
    const char* cmdStr = cmd->getCommand();

    JobEntry* jobToAdd = new JobEntry(jobID, pid, cmdStr, isStopped);
    jobs.push_back(jobToAdd);
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    for (; it != jobs.end(); ++it) {
        if (*it) {
            cout << *(*it);
        }
    }
}

void JobsList::killAllJobs() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    SmallShell& smash = SmallShell::getInstance();
    while (it != jobs.end()) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            string cmd = (*it)->getJobCmd();
            Command* tempCmd = smash.CreateCommand((*it)->getJobCmd().c_str());
            bool isPipe = (typeid(*tempCmd) == typeid(PipeCommand));
            bool isRedir = (typeid(*tempCmd) == typeid(RedirectionCommand));
            delete tempCmd;
            int res = -1;
            if (isPipe || isRedir) {
                res = killpg(pid, SIGKILL);
            }
            else {
                res = kill(pid, SIGKILL);
            }
            if (res != 0) {
                cerr << "smash error: kill: invalid arguments" << endl;
            } else {
                cout << pid << " : " <<  cmd << endl;
            }
            delete *it;
            *it = nullptr;
            it = jobs.erase(it);
        }
    }
    updateJobsCount();
}

void JobsList::removeFinishedJobs() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    int res;
    while (it != jobs.end()) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            /// waitpid with WNOHANG returns the pid if the process finished and 0 if it is still running
            res = waitpid(pid, NULL, WNOHANG);   /// for some reason returns pid even when process is running
            if (res > 0) {
                /// gets here if this process finished
                delete *it;
                *it = nullptr;
                /// because iterator is undefined after erase (erase advances the iterator)
                it = jobs.erase(it);
            } else {
                ++it;
            }
        }
    }

    updateJobsCount();
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    int itJobID;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            itJobID = (*it)->getJobID();
            if (itJobID == jobId) {
                return *it;
            }
        }
    }
    /// gets here if there is no job with the given id
    return nullptr;
}

///kill or waitpid?
void JobsList::removeJobById(int jobId) {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    int res;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            int itJobID = (*it)->getJobID();
            if (itJobID == jobId) {
                /// waitpid with WNOHANG returns the pid if the process finished and 0 if it is still running
                res = waitpid(pid, NULL, WNOHANG);
                if (res > 0) {
                    /// gets here if this process finished
                    delete *it;
                    *it = nullptr;
                    jobs.erase(it);
                }
                break;
            }
        }
    }
    updateJobsCount();
}

void JobsList::killThisFucker(int jobId) {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    for (; it != jobs.end(); ++it) {
        if (*it) {
            int itJobID = (*it)->getJobID();
            if (itJobID == jobId) {
                /// gets here if this process finished
                delete *it;
                *it = nullptr;
                jobs.erase(it);
                break;
            }
        }
    }
    updateJobsCount();
}

JobsList::JobEntry* JobsList::getLastJob(int *lastJobId) {
    this->removeFinishedJobs();
    size_t size = jobs.size();
    if (size == 0) {
        if (lastJobId) *lastJobId = -1;
        return nullptr;
    }
    JobsList::JobEntry* lastJob = jobs.back();
    if (!lastJob && lastJobId) *lastJobId = -1;
    else if (lastJob && lastJobId){
        *lastJobId = lastJob->getJobID();
    }
    return lastJob;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *JobId) {
    JobsList::JobEntry* lastStopped = nullptr;
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    bool isStopped;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            isStopped = (*it)->isJobStopped();
            if (isStopped) {
                if (JobId) *JobId = (*it)->getJobID();
                lastStopped = *it;
            }
        }
    }
    if (!lastStopped && JobId) *JobId = -1;
    return lastStopped;
}

void JobsList::updateJobsCount() {
    size_t size = jobs.size();
    if (size == 0) {
        jobsCount = 0;
    } else {
        JobsList::JobEntry* last = jobs.back();
        jobsCount = last->getJobID();
    }
}

const int JobsList::getJobsCount() const { return jobsCount;}
int JobsList::getTotalJobs() const {return jobs.size();}
///==================================================================================================================///


///ChpromptCommand///
///==================================================================================================================///
ChpromptCommand::ChpromptCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
        BuiltInCommand(cmd.c_str(), takes_cpu), new_prompt("") {
    SmallShell& smash = SmallShell::getInstance();
    if (!args) {
        cerr << "smash error: chprompt: invalid arguments" << endl;
    }
    /// args = [arg1='chprompt', arg2, arg3, ..., argn, NULL]
    if (numOfArgs == 1) new_prompt = "smash> ";
//    else if (numOfArgs > 2) {
//        new_prompt = smash.getPrompt();   /// why commenting this?
//    }
        /// we assume numOfArgs == 2 and ignoring the rest
    else {
        string arrow = "> ";
        new_prompt = args[1] + arrow;
    }
}

void ChpromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.setPrompt(this->new_prompt);
}
///==================================================================================================================///


///GetCurrDirCommand///
///==================================================================================================================///
GetCurrDirCommand::GetCurrDirCommand(const string cmd, bool takes_cpu) : BuiltInCommand(cmd.c_str(), takes_cpu) {}

void GetCurrDirCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();

    /// If any arguments were provided they will be ignored

    char* res = get_current_dir_name();
    string path = res;

    if(!res){
        cerr << "smash error: getcwd failed" << endl;///TODO: Should be perror?
    }else {
        cout << path << endl;
    }

    free(res);
}
///==================================================================================================================///

///ShowPidCommand///
///==================================================================================================================///
ShowPidCommand::ShowPidCommand(const string cmd, bool takes_cpu) : BuiltInCommand(cmd.c_str(), takes_cpu) {}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();

    /// If any arguments were provided they will be ignored
    ///TODO: Check for errors from getpid system call

    pid_t shellPID = smash.getSmashPid();
    if (shellPID < 0) {
        cerr << "smash error: getpid failed" << endl;
    }

    cout << "smash pid is " << shellPID << endl;
}
///==================================================================================================================///


///ChangeDirCommand///
///==================================================================================================================///
ChangeDirCommand::ChangeDirCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
        BuiltInCommand(cmd.c_str(), takes_cpu), newWD(""), currentWD("") {

    if (!args) {
        cerr << "smash error: cd: invalid arguments" << endl;
    }

    SmallShell& smash = SmallShell::getInstance();

    char* res = get_current_dir_name();
    currentWD = res;

    string lastWDCallBack = "-";
    string lastWD = smash.getLastWD();

    if(numOfArgs == 1) {
        newWD = currentWD;
    }
    else if(numOfArgs > 2){
        this->validArgs = false;
        //cerr << "smash error: cd: too many arguments" << endl;
        newWD = currentWD;
    }
        // LastPWD isnt set and got "-" argument(havent yet cd in this instance)
    else if(lastWD == "" && args[1] == lastWDCallBack) {
//        cout << "smash error: cd: OLDPWD not set" << endl;
        hasPrevDir = false;
        //cerr << "smash error: cd: OLDPWD not set" << endl;
        newWD = currentWD;
    }
        // Handling case if we got "-" as the first argument (and already have done at least one cd call)
    else if(args[1] == lastWDCallBack){
        newWD = lastWD;
    }
        // All is find in the world
    else{
        newWD = args[1];
    }

    free(res);
}

void ChangeDirCommand::execute() {

    SmallShell& smash = SmallShell::getInstance();

    if(!validArgs) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    } else if(!hasPrevDir) {
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
    }

    if(newWD == currentWD) {
        return;
    }
    /// if res == 0 hakol tov
    int res = chdir(newWD.c_str());
    if (res != 0){
        // no cd has been done, so we keep lastWD as is
//        cerr << "smash error: chdir failed" << endl;
        perror("smash error: chdir failed");
    }

        //chdir has succeeded and we need to remember the lastWD before cd has been done (saved in currentWD in constructor)
    else{
        smash.setLastWD(currentWD);
    }
}
///==================================================================================================================///


///CopyCommand///
///==================================================================================================================///
CopyCommand::CopyCommand(const char *cmd_line, char **args, int numOfArgs, bool takes_cpu) :
        BuiltInCommand(cmd_line, takes_cpu), src(""), dest("") {
    if (!args) {
        cerr << "smash error: cp: invalid arguments" << endl;
    }

    else if (numOfArgs > 3 && takes_cpu) {///TODO: why takesCPU?
        validArgs = false;
        //cerr << "smash error: too many arguments" << endl;
    } else {
        /// check if pointers are valid
        if (args[1]) {
            string src_path = args[1];
            /// check if the src is from home directory
            size_t res = src_path.find("~");
            if (res == 0) {
                /// get here if it is from home. getting home directory path and adding to rest of src
                string homeDir = getenv("HOME");
                src_path = homeDir + src_path.substr(1);
            }
            this->src = src_path;
        }
        /// doing same as before but for dest
        if (args[2]) {
            string dest_path = args[2];
            size_t res = dest_path.find("~");
            if (res == 0) {
                string homeDir = getenv("HOME");
                dest_path = homeDir + dest_path.substr(1);
            }
            this->dest = dest_path;
        }
    }
}

/// fd1 = open src O_RDONLY (if fd1 < 0 go die in hell)
/// fd2 = open dest O_WRONLY | O_CREAT | O_TRUNC, 0666 (if fd2 < 0 go kill yourself)
/// buffer[buffer size] = {0}
/// int count = -1
/// loop (count != 0)
///     clear the buffer
///     count = read(fd1, buffer, buffer size)
///     if (count == -1) DDDDDIIIIEEEE!!!!!
///     wCount = write(fd2, buffer, count)
///     if (wCount == -1) PLEASE PLEASE DIE ALREADY...
/// close fd1
/// close fd2

void clearBuffer(char* buffer, int size) {
    memset(buffer, 0, size);
}
///TODO: check file per,issions and shit
void CopyCommand::execute() {

    if(!validArgs) {
        cerr << "smash error: too many arguments" << endl;
        return;
    }
    if ((src.compare("") == 0) || (dest.compare("") == 0)) return;
    if (src.compare(dest) == 0) {
        cout << "smash: " << src << " was copied to " << dest << endl;
        return;
    }

    int src_fd = open(src.c_str(), O_RDONLY);
    if (src_fd < 0) {
        perror("smash error: open failed");
        return;
    }

    int dest_fd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd < 0) {
        perror("smash error: open failed");
        return;
    }

    char buffer[BUFFER_SIZE] = {0};
    int count = -1;
    int wCount = -1;
    while (count != 0) {
        clearBuffer(buffer, BUFFER_SIZE);

        count = read(src_fd, buffer, BUFFER_SIZE);
        if (count == -1) {
            perror("smash error: read failed");
            return;
        }

        wCount = write(dest_fd, buffer, count);
        if (wCount == -1) {
            perror("smash error: write failed");
            return;
        }
    }

    int res_src, res_dest;
    res_src = close(src_fd);
    res_dest = close(dest_fd);
    if (res_src != 0 || res_dest != 0) {
        perror("smash error: close failed");
    }
    cout << "smash: " << src << " was copied to " << dest << endl;
}
///==================================================================================================================///



///JobsCommand///
///==================================================================================================================///
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs, bool takes_cpu) : BuiltInCommand(cmd_line, takes_cpu),
                                                                                 jobs(jobs){}
void JobsCommand::execute() {jobs->printJobsList();}
///==================================================================================================================///



///KillCommand///
///==================================================================================================================///
bool isNum(string s)
{
    for (int i = 0; i < s.length(); i++)
        if (isdigit(s[i]) == false)
            return false;

    return true;
}

KillCommand::KillCommand(const char *cmd_line, char **args, int numOfArgs, JobsList *jobs, bool takes_cpu) :
        BuiltInCommand(cmd_line, takes_cpu), signal(-1), jobID(-1), jobs(jobs) {
    if (numOfArgs > 3 || !args[1] || !args[2] || args[3]) {
//        perror("smash error: kill: invalid arguments");
        validArgs = false;
        //cerr << "smash error: kill: invalid arguments" << endl;
    }
    else{
        char dash = *args[1];
        string inputNum =  args[1];
        inputNum = inputNum.substr(1);
        stringstream sigStr(inputNum);
        if (!isInt(inputNum) || !isInt(args[2])) {
            validArgs = false;
            //cerr << "smash error: kill: invalid arguments" << endl;        
        }
        else {
            sigStr >> this->signal;
            stringstream jobIdStr(args[2]);
            jobIdStr >> this->jobID;
            if (dash != '-' || signal <= 0 || jobID <= 0) {    /// change this condition to check the string!
                //cerr << "smash error: kill: invalid arguments" << endl;
                validArgs = false;
                signal = -1;
                jobID = -1;
            }
        }
    }

}


void KillCommand::execute() {
    if(!validArgs) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if (signal < 0 || jobID < 0) return;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* job = jobs->getJobById(jobID);
    if (!job) {
        string jobIdStr = to_string(jobID);
        string errMsg = "smash error: kill: job-id " + jobIdStr + " does not exist";
//        perror(errMsg.c_str());
        cerr << errMsg << endl;
        return;
    }

    jobPID = job->getJobPID();
    int res = -1;
    Command* cmd = smash.CreateCommand(job->getJobCmd().c_str());
    bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
    bool isRedir = (typeid(*cmd) == typeid(RedirectionCommand));
    delete cmd;
    if (isPipe || isRedir) {
        res = killpg(jobPID, signal);
    }
    else {
        res = kill(jobPID, signal);
    }
    if (res != 0) {
//        perror("smash error: kill failed"); ///TODO: should be perror?
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    } else {
        string sigStr = to_string(signal);
        string pidStr = to_string(jobPID);
        string msg = "signal number " + sigStr + " was sent to pid " + pidStr;
        cout << msg << endl;
        if(signal == SIGSTOP || signal == SIGTSTP) {
            job->stopJob();
        }
        else if (signal == SIGCONT) {
            job->resumeJob();
        }
    }
}
///==================================================================================================================///


///ForegroundCommand///
///==================================================================================================================///
ForegroundCommand::ForegroundCommand(const char *cmd_line, char** args, int numOfArgs, JobsList *jobs, bool takes_cpu) :
        BuiltInCommand(cmd_line, takes_cpu), jobID(0), jobs(jobs), jobIDGiven(true), validArg(true){
    /// check more options?
    if (numOfArgs > 2) {
        validArgs = false;
        //cerr << "smash error: fg: invalid arguments" << endl;
    }
    /// no job id given
//    if (!args[1]) jobID = -1;
    if (!args[1]) jobIDGiven = false;

    else if (!isInt(args[1])) {
        validArg = false;
    }
        /// job id given
    else {
        stringstream jobIdStr(args[1]);
        jobIdStr >> this->jobID;
    }
}

void ForegroundCommand::execute() {
    if (!validArg || !validArgs) {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* job = nullptr;
    if (jobIDGiven) {
        if (jobID > 0) {
            job = jobs->getJobById(jobID);
            if (!job) {
                string jobIdStr = to_string(jobID);
                string errMsg = "smash error: fg: job-id " + jobIdStr + " does not exist";
//            perror(errMsg.c_str());
                cerr << errMsg << endl;
                return;
            }
        }
        else if (jobID < 0) {
            string jobIdStr = to_string(jobID);
            string errMsg = "smash error: fg: job-id " + jobIdStr + " does not exist";
//            perror(errMsg.c_str());
            cerr << errMsg << endl;
            return;
        }
    }
    else {        /// jobID == -1
        job = jobs->getLastJob(&jobID);
        if (!job) {
            cerr << "smash error: fg: jobs list is empty" << endl; /// just print and return instead?
        }
    }

    pid_t pid = job->getJobPID();
    string cmd = job->getJobCmd();
    cout << cmd << " : " << pid << endl;

    smash.updateCurrentJob(job);
    bool isStopped = job->isJobStopped();
    Command* tempCmd = smash.CreateCommand(job->getJobCmd().c_str());
    bool isPipe = (typeid(*tempCmd) == typeid(PipeCommand));
    bool isRedir = (typeid(*tempCmd) == typeid(RedirectionCommand));
    delete tempCmd;
    if (isStopped) {
        int res = -1;
        job->resumeJob();
        if (isPipe || isRedir) {
            res = killpg(pid, SIGCONT);
        }
        else {
            res = kill(pid, SIGCONT);     /// we return the job to continue, else it is already running in the background
        }
        if (res != 0) {
            perror("smash error: kill failed");
        }
    }

    int status;
    waitpid(pid, &status, WUNTRACED);   /// "bring job to foreground" by waiting for it to finish
    if (!WIFSTOPPED(status)) {
        jobs->killThisFucker(jobID);
        smash.clearCurrentJob(false);
        return;
    }

    JobsList::JobEntry* res = jobs->getJobById(jobID);
    bool deleteEntry = !res;
    smash.clearCurrentJob(deleteEntry);
}
///==================================================================================================================///




///BackgroundCommand///
///==================================================================================================================///

BackgroundCommand::BackgroundCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu, JobsList* jobs) :
        BuiltInCommand(cmd.c_str(), takes_cpu), inputJobID(-1), jobToStopID(-1), jobs(jobs) {

    if(numOfArgs > 2) {
        validArgs = false;
        //cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }

    //Checking legit argument 2
    if(args[1]) {
        stringstream strStm(args[1]);
        strStm >> inputJobID;
        if(inputJobID <= 0) { // returns 
//            perror("smash error: bg: invalid arguments");
            //cerr << "smash error: bg: job-id " << jobID << " does not exist" <<endl;
            return;
        } else { // Got a legit number in argument 2
            JobsList::JobEntry* job = jobs->getJobById(inputJobID);
            if(!job) {
                cerr << "smash error: bg: job-id " << inputJobID << " does not exist" <<endl;
                jobToStopID = -1;
                return;
            } else { // Found some Job ID
                bool isStopped = job->isJobStopped();
                if(!isStopped) {
                    cerr << "smash error: bg: job-id " << inputJobID <<" is already running in the background" << endl;
                    jobToStopID = -1;
                    return;
                } else { // Job is indeed stopped
                    this->jobToStopID = inputJobID;
                    this->jobToStop = job;
                }
            }
        }
    } else { //Got only bg [no arguments]
        int foundStoppedID;
        JobsList::JobEntry* foundStoppedJob  = jobs->getLastStoppedJob(&foundStoppedID);
        if(numOfArgs == 1){
            if(!foundStoppedJob) {
                cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
                jobToStopID = -1;
                return;
            } else { //using bg without arguments
                this->jobToStopID = foundStoppedID;
                this->jobToStop = foundStoppedJob;
            }
        }
    }
}

void BackgroundCommand::execute() {

    //no execution if command was bad
    if(!validArgs) {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    if(jobToStopID <= 0) {

        return;
    }

    string cmd = jobToStop->getJobCmd();
    int pidToCont;
    pidToCont = jobToStop->getJobPID();
    cout << cmd << " : " << pidToCont << endl;

    SmallShell& smash = SmallShell::getInstance();
    Command* tempCmd = smash.CreateCommand(jobToStop->getJobCmd().c_str());
    bool isPipe = (typeid(*tempCmd) == typeid(PipeCommand));
    bool isRedir = (typeid(*tempCmd) == typeid(RedirectionCommand));
    int res = -1;
    if (isPipe || isRedir) {
        res = killpg(pidToCont, SIGCONT);
    }
    else {
        res = kill(pidToCont, SIGCONT);     /// we return the job to continue, else it is already running in the background
    }
    if (res != 0) {
        perror("smash error: kill failed");
    }

//    int res = kill(pidToCont, SIGCONT);
//    if(res != 0) {
//        perror("smash error: kill failed");
//    }
    else {
        //manage JobsList
        jobToStop->resumeJob();
    }

}
///==================================================================================================================///


///QuitCommand///
///==================================================================================================================///

QuitCommand::QuitCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu, JobsList* jobs) :
        BuiltInCommand(cmd.c_str(), takes_cpu), jobs(jobs), isKilling(false)
{
    string killArg = "kill";
    string input;
    if(args[1]) {
        input = args[1];
    }
    if(killArg.compare(input) == 0) { // delete all working or stopped jobs
        isKilling = true;
    }
    //otherwise ignore all else
}

void QuitCommand::execute() {

    if(!isKilling) return;

    jobs->removeFinishedJobs(); //removing for correct vector size => total jobs to kill

    int numKillsToDo = jobs->getTotalJobs();
    cout << "smash: sending SIGKILL signal to " << numKillsToDo << " jobs:" << endl;

    jobs->killAllJobs(); //printing done inside + kills. The messacare!
}
///==================================================================================================================///


///PipeCommand///
///==================================================================================================================///
PipeCommand::PipeCommand(const char *cmd_line, bool takes_cpu) : Command(cmd_line, takes_cpu), firstCmd(""),
                                                                 secondCmd(""), redirectStdOut(true) {
    string cmd = this->getCommand();
    char* cmdChar = const_cast<char*> (cmd.c_str());
    _removeBackgroundSign(cmdChar);
    cmd = cmdChar;

    size_t res = cmd.find("|&");
    if (res != string::npos) this->redirectStdOut = false;

    if (redirectStdOut) {
        res = cmd.find("|");
        this->firstCmd = cmd.substr(0,res-1);
        this->secondCmd = cmd.substr(res+1,cmd.length()-res+1);
    }
    else {
        /// res = cmd.find("|&") is already known
        this->firstCmd = cmd.substr(0,res-1);
        this->secondCmd = cmd.substr(res+2,cmd.length()-res+1);
    }
}

/// if first command is 'showpid' it return the pid of the first child instead of parent.
/// current implementation is for two external commands...
void PipeCommand::execute() {
    if (firstCmd.empty() || secondCmd.empty()) return;
    SmallShell &smash = SmallShell::getInstance();
    int myPipe[2];
    pipe(myPipe);

    setpgid(getpid(), 0);
    pid_t pidFirst = fork();
    if (pidFirst < 0) {
        perror("smash error: fork failed");
    } else if (pidFirst == 0) {     /// first child - runs first command
        close(myPipe[0]);
        if (redirectStdOut) dup2(myPipe[1], 1);     /// stdout of the first command -> pipe write channel
        else dup2(myPipe[1], 2);     /// stderr of the first command -> pipe write channel
        close(myPipe[1]);
        smash.markFromPipe();
        smash.executeCommand((this->firstCmd).c_str());
        exit(0);
    } else {      /// father
        pid_t pidSecond = fork();
        if (pidSecond < 0) {
            perror("smash error: fork failed");
        } else if (pidSecond == 0) {     /// second child - runs second command
            close(myPipe[1]);
            dup2(myPipe[0], 0);     /// stdin of second command -> pipe read channel
            close(myPipe[0]);
            smash.markFromPipe();
            smash.executeCommand((this->secondCmd).c_str());
            exit(0);
        } else {      /// father
            close(myPipe[0]);
            close(myPipe[1]);
            while (wait(NULL) > 0) {}
        }
    }
}



///==================================================================================================================///


///ExternalCommand///
///==================================================================================================================///
ExternalCommand::ExternalCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
        Command(cmd.c_str(), takes_cpu) {}


void ExternalCommand::execute() {

    ///TODO: What happens in the jobslist? We can add it in the constructor if it succeeds.
    ///Execution is handles in execute and joblist management is not here

    string cmdStr = this->getCommand();
    char* cmdChar = const_cast<char*> (cmdStr.c_str());
    _removeBackgroundSign(cmdChar);
    cmdStr = cmdChar;
    cmdChar = const_cast<char*> (cmdStr.c_str());

    char* const argv[4] = {const_cast<char*>("bash"), const_cast<char*>("-c"), cmdChar, NULL};
    execv("/bin/bash", argv);

    perror("smash error: execv failed");
}
///==================================================================================================================///


///TimeoutEntry///
///==================================================================================================================///
TimeoutEntry::TimeoutEntry(const string cmd_line, pid_t pid, int duration) :
        cmd(cmd_line), pid(pid), start(time(nullptr)), duration(duration){}

time_t TimeoutEntry::getTimeLeft() const {
    time_t current = time(nullptr);
    int timeElapsed = difftime(current, start) + 0.5;
    return duration - timeElapsed;
}
string TimeoutEntry::getCmd() const { return cmd;}
pid_t TimeoutEntry::getPID() const { return pid;}
///==================================================================================================================///


///TimeoutCommand///
///==================================================================================================================///
TimeoutCommand::TimeoutCommand(const char *cmd_line, char **args, int numOfArgs, bool takes_cpu) :
        BuiltInCommand(cmd_line, takes_cpu), cmdToRun(""), start(time(nullptr)), duration(-1) {
    if (!args[1] || !isInt(args[1]) || numOfArgs == 0 || numOfArgs < 3) {
        validArgs = false;
        //cerr << "smash error: timeout: invalid arguments" << endl;
    }
    else {
        stringstream durationStr(args[1]);
        durationStr >> this->duration;
        if (this->duration <= 0) {
            cerr << "smash error: timeout: invalid arguments" << endl;
        }
    }

    for (int i = 2; i < numOfArgs; i++) {
        string toAdd = args[i];
        cmdToRun += toAdd;
        if (i+1 != numOfArgs) cmdToRun += " ";
    }
}

void TimeoutCommand::execute() {
    if(!validArgs) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    if (cmdToRun.empty() || duration <= 0) return;
    SmallShell& smash = SmallShell::getInstance();
    smash.executeCommand(cmdToRun.c_str());
}

int TimeoutCommand::getDuration() const { return duration;}
const char * TimeoutCommand::getCmdToRun() const { return cmdToRun.c_str();}
///==================================================================================================================///




///RedirectionCommand///
///==================================================================================================================///
RedirectionCommand::RedirectionCommand(const char* cmd_line, char** args, int numOfArgs, bool takes_cpu, bool isOverwrite) : Command(cmd_line,takes_cpu), innerCmd(""),
                                                                                                                             isOverWrite(isOverwrite), file("") {


    string cmd = this->getCommand();
    char* cmdChar = const_cast<char*> (cmd.c_str());
    _removeBackgroundSign(cmdChar);
    cmd = cmdChar;

    int cmdLoc = -1;
    int redir_cmd_len = -1;
    if (isOverWrite) { //i.e >>
        cmdLoc = cmd.find(">>");
        redir_cmd_len = 2;
    } else {
        cmdLoc = cmd.find(">");
        redir_cmd_len = 1;
    }

    string temp = "";

    temp = cmd.substr(cmdLoc + redir_cmd_len, cmd.size() - cmdLoc - redir_cmd_len);
    temp = _trim(temp);
    int firstFileLoc = temp.find_first_of(' ');
    //get the first file name only
    this->file = temp.substr(0, temp.find(" "));
    this->innerCmd = cmd.substr(0, cmdLoc);
}

// If chgdir, chgprompt then execute normally but redirect the "nothing" output to file
// else, run in child with output redirection.
// if it doesnt take CPU manage it in executeCommand



void RedirectionCommand:: execute(){
    SmallShell& smash = SmallShell::getInstance();

    Command* otherCmd = smash.CreateCommand(this->innerCmd.c_str());
    //Ignore chdir and change prompt

    bool isExternal = (typeid(*otherCmd) == typeid(ExternalCommand));
    bool isCopy = (typeid(*otherCmd) == typeid(CopyCommand));

    delete otherCmd;

    if (!isExternal && !isCopy) { //will execute the command and redirect the output
        int bkup = dup(1);
        close(1);
        int new_fd1 = -1;
        if (isOverWrite) {
            new_fd1 = open(this->file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (new_fd1 < 0 ) {
                perror("smash error: open failed");
            }
        }
        else { // ">" command
            new_fd1 = open(this->file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (new_fd1 < 0 ) {
                perror("smash error: open failed");
            }
        }
        smash.executeCommand(this->innerCmd.c_str());
        dup2(bkup, 1);

    } else { //Executing external and copy and also isForkable=true

        setpgid(getpid(), 0);
        pid_t pid = fork();


        if (pid < 0) {
            perror("smash error: fork failed");
        }
        else if (pid == 0) {
            int backUpFD = dup(1);
            close(1);
            int new_fd = -1;
            if (isOverWrite) {
                new_fd = open(this->file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                if (new_fd < 0 ) {
                    perror("smash error: open failed");
                }
            }
            else { // ">" command
                new_fd = open(this->file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (new_fd < 0 ) {
                    perror("smash error: open failed");
                }
            }
            smash.markFromRedir();
            smash.executeCommand(this->innerCmd.c_str());

            dup2(backUpFD, 1);
            exit(0);

        } else { //father = manager
            while(wait(NULL) > 0) {}
        }


    }



}


bool RedirectionCommand::isForkable() {
    SmallShell& smash = SmallShell::getInstance();

    Command* otherCmd = smash.CreateCommand(this->innerCmd.c_str());
    bool isExternal = (typeid(*otherCmd) == typeid(ExternalCommand));
    bool isCopy = (typeid(*otherCmd) == typeid(CopyCommand));

    if(isExternal || isCopy) {
        return true;
    }
    else {
        return false;
    }

}
///==================================================================================================================///





///SmallShell///
///==================================================================================================================///
SmallShell::SmallShell() : prompt("smash> "), lastWD(""), jobList(new JobsList()),
                           timedList(vector<TimeoutEntry*>()), currentJob(nullptr), smashPID(getpid()), fromPipe(false) {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    if (jobList) delete jobList;
    if (currentJob) delete currentJob;
    vector<TimeoutEntry*>::iterator it = timedList.begin();
    while (it != timedList.end()) {
        if (*it) {
            delete *it;
            *it = nullptr;
            /// because iterator is undefined after erase
            it = timedList.erase(it);
        } else {
            ++it;
        }
    }
}

/// Creates and returns a pointer to Command class which matches the given command line (cmd_line).
/// \param cmd_line - the command line to return its' relevant Command class.
/// \return A pointer to the Command object for the given command line or nullptr if no matching class was found.
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmdStr = string(cmd_line);
    cmdStr = _trim(cmdStr);

    /// params to send to constructors
    bool takes_cpu = !_isBackgroundComamnd(cmd_line);
    char** args = new char*[COMMAND_MAX_ARGS];
    int numOfArgs = _parseCommandLine(cmd_line, args);
    /// this is a fix for args when getting commands like 'sleep 10&' args = ['sleep', '10&'] so changed to args = ['sleep', '10', '&']
    string lastArg = args[numOfArgs-1];
    if (lastArg.find("&") == lastArg.length()-1 && lastArg.length() > 1) {
        string ampersand = lastArg.substr(lastArg.length()-1);
        string arg = lastArg.substr(0, lastArg.length()-1);
        string endOfCmd = arg + " " + ampersand;
        _parseCommandLine(endOfCmd.c_str(), args+numOfArgs-1);
        numOfArgs++;
    }

    /// remove the background sign if needed
    char* cmdChar = new char[cmdStr.length() + 1];
    strcpy(cmdChar, cmdStr.c_str());
    if (!takes_cpu) {
        /// if we got here, the command needs to run in the background
        _removeBackgroundSign(cmdChar);
        cmdStr = string(cmdChar);
    }
    Command* command = nullptr;

    /// check what command was sent and returns the relevant command object
    /// string.find("some string") return the index of the first occurrence of the given string, so we check the
    /// first word.
    ///TODO: we need to send the relevant parameters to each constructor. This is just a skeleton.
    ///TODO: Does the commands get freed by themselves?

    if (cmdStr.find("timeout") == 0) {
        command = new TimeoutCommand(cmd_line, args, numOfArgs, takes_cpu);
    }

    else if (cmdStr.find("|") != string::npos) {
        command = new PipeCommand(cmd_line, takes_cpu);
    }

    else if (cmdStr.find(">>") != string::npos) {
        command = new RedirectionCommand(cmd_line, args, numOfArgs, takes_cpu, true);
    }

    else if (cmdStr.find(">") != string::npos) {
        command = new RedirectionCommand(cmd_line, args, numOfArgs,takes_cpu, false);
    }

    else if (cmdStr.find("chprompt") == 0) {
        command = new ChpromptCommand(cmd_line, args, numOfArgs, takes_cpu);
    }
    else if (cmdStr.find("pwd") == 0) {
        command = new GetCurrDirCommand(cmd_line, takes_cpu);
    }
    else if (cmdStr.find("showpid") == 0) {
        command = new ShowPidCommand(cmd_line, takes_cpu);
    }
    else if (cmdStr.find("cd") == 0) {
        command = new ChangeDirCommand(cmd_line, args, numOfArgs, takes_cpu);
    }
    else if (cmdStr.find("cp") == 0) {
        command = new CopyCommand(cmd_line, args, numOfArgs, takes_cpu);
    }
    else if (cmdStr.find("jobs") == 0) {
        command = new JobsCommand(cmd_line, jobList, takes_cpu);
    }
    else if (cmdStr.find("kill") == 0) {
        command = new KillCommand(cmd_line, args, numOfArgs, jobList, takes_cpu);   /// this is working but still need to change some checks
    }
    else if (cmdStr.find("fg") == 0) {
        command = new ForegroundCommand(cmd_line, args, numOfArgs, jobList, takes_cpu); /// this is working but still need to change some checks
    }
    else if (cmdStr.find("bg") == 0) {
        command = new BackgroundCommand(cmd_line, args, numOfArgs, takes_cpu, jobList);
    }
    else if (cmdStr.find("quit") == 0) {
        command = new QuitCommand(cmd_line, args, numOfArgs, takes_cpu, jobList);
    }
    else {
        command = new ExternalCommand(cmd_line, args, numOfArgs, takes_cpu);
    }

    for (int i = 0; i < numOfArgs; i++) {
        if (args[i]) free(args[i]);
    }
    delete[] args;
    delete[] cmdChar;
    return command;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);

    if(!cmd) return;

    //TODO: clear finished jobs
    jobList->removeFinishedJobs();
    int duration = -1;
    string timedCmd = cmd->getCommand();
    bool isTimed = (typeid(*cmd) == typeid(TimeoutCommand));
    if (isTimed) {
        duration = static_cast<TimeoutCommand&>(*cmd).getDuration();
        if (!(cmd->takesCPU())) {
            cmd = CreateCommand(static_cast<TimeoutCommand&>(*cmd).getCmdToRun());
        }
        else {
            if (duration != -1) {
                this->addTimedCommand(timedCmd, getpid(), duration);
                int alarm_duration = this->getMinTimeLeft();
                alarm(alarm_duration);
            }
        }
    }

    bool isRediraction = (typeid(*cmd) == typeid(RedirectionCommand));
    bool isForkable = false;
    if(isRediraction) {
        isForkable = static_cast<RedirectionCommand&>(*cmd).isForkable();
    }
    bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
    bool isExternal = (typeid(*cmd) == typeid(ExternalCommand));
    bool isCopy = (typeid(*cmd) == typeid(CopyCommand));
    bool isQuit = (typeid(*cmd) == typeid(QuitCommand));
    if (isExternal || isCopy || isQuit || isPipe || (isTimed && !(cmd->takesCPU())) || (isRediraction && isForkable)) {
        pid_t pid = fork();

        if(pid == -1) {
            perror("smash error: fork failed");
            //TODO: return?
        }

        if(pid == 0){ //Child
            if (!fromPipe && !fromRedir) setpgrp();
            cmd->execute();
            delete cmd;
            cmd = nullptr;
            exit(0);

        } else { //Parent
            //TODO: add job

            if(isQuit) {
                waitpid(pid, NULL, WUNTRACED);
                delete cmd;
                cmd = nullptr;
                exit(0);
            }
            bool takesCPU = cmd->takesCPU();
            if (isTimed) {
                if (duration != -1) {
                    this->addTimedCommand(timedCmd, getpid(), duration);
                    int alarm_duration = this->getMinTimeLeft();
                    alarm(alarm_duration);
                }
//                this->addTimedCommand(timedCmd, pid, duration);
//                int alarm_duration = this->getMinTimeLeft();
//                alarm(alarm_duration);
            }
            if (takesCPU) {
                updateCurrentJob(new JobsList::JobEntry(-1, pid, cmd_line, false));
                waitpid(pid, NULL, WUNTRACED);
                delete cmd;
                cmd = nullptr;
                JobsList::JobEntry* job = jobList->getLastJob(nullptr);
                pid_t jobPID = -1;
                bool deleteEntry = true;
                if (job) {
                    jobPID = job->getJobPID();
                    if (jobPID == pid) deleteEntry = false;
                }
                clearCurrentJob(deleteEntry);
//                //remove
//                jobList->removeJobById(addedChildJobId);
            }
            else {
                //TODO: clear finished jobs
                jobList->removeFinishedJobs();
                if (isTimed) {
                    Command* toAdd = CreateCommand(cmd_line);
                    jobList->addJob(toAdd, pid, false);//isStopped=false
                }else {
                    jobList->addJob(cmd, pid, false);//isStopped=false
                }
            }
//            delete cmd;
            jobList->removeFinishedJobs();
        }
    }

    else {
        cmd->execute();
    }
    delete cmd;
}

string SmallShell::getPrompt() const { return prompt;}
void SmallShell::setPrompt(string newPrompt) {prompt = newPrompt;}
string SmallShell::getLastWD() const { return lastWD;}
void SmallShell::setLastWD(string path) {lastWD = path;}
bool SmallShell::isTimedEmpty() const { return timedList.empty();}
void SmallShell::addTimedCommand(const string cmd, pid_t pid, int duration)  {
    /// do some checks...
    TimeoutEntry* entry = new TimeoutEntry(cmd, pid, duration);
    timedList.push_back(entry);
    std::sort(timedList.begin(), timedList.end(), comparePtrs());
}
int SmallShell::getMinTimeLeft() {
    std::sort(timedList.begin(), timedList.end(), comparePtrs());
    int min = timedList.front()->getTimeLeft();
    return min;
}
TimeoutEntry * SmallShell::getEntryToKill() {
    TimeoutEntry* entry = timedList.front();
    timedList.erase(timedList.begin());
    return entry;
}
void SmallShell::updateCurrentJob(JobsList::JobEntry *job) {currentJob = job;}
void SmallShell::clearCurrentJob(bool deleteEntry) {
    if (deleteEntry && currentJob) delete currentJob;
    currentJob = nullptr;
}
bool SmallShell::jobInList(int jobId) const {
    JobsList::JobEntry* job = jobList->getJobById(jobId);
    if (job) return true;
    return false;
}
JobsList::JobEntry * SmallShell::getCurrentJob() { return currentJob;}
void SmallShell::addSleepingJob(JobsList::JobEntry *job) {
    int jobID = job->getJobID();
    JobsList::JobEntry* res = jobList->getJobById(jobID);
    if (res) {
        job->stopJob();
    }
    else {
        string cmdStr = job->getJobCmd();
        Command* cmd = CreateCommand(cmdStr.c_str());
        pid_t pid = job->getJobPID();
        jobList->addJob(cmd, pid, true);
    }
}
pid_t SmallShell::getSmashPid() const { return smashPID;}
///==================================================================================================================///