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
    while (it != jobs.end()) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            string cmd = (*it)->getJobCmd();
            cout << pid << " : " <<  cmd << endl;
            kill(pid, SIGKILL); 
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
                res = waitpid(pid, nullptr, WNOHANG);
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

JobsList::JobEntry* JobsList::getLastJob(int *lastJobId) {
    this->removeFinishedJobs();
    size_t size = jobs.size();
    if (size == 0) {
        *lastJobId = -1;
        return nullptr;}
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

    /// args = [arg1='chprompt', arg2, arg3, ..., argn, NULL]
    if (numOfArgs == 1) new_prompt = "smash> ";
    else if (numOfArgs > 2) {
        new_prompt = smash.getPrompt();
    }
    /// we assume numOfArgs == 2
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

    char *buff = nullptr;
    size_t bufSize = 0;
    string path;
    char *res = nullptr;
    res = getcwd(buff, bufSize);
    path = res;

    if(!res){
        perror("smash error: getcwd failed");
    }else {
        cout << path << endl;
    }
    free(buff);
}
///==================================================================================================================///

                                            ///ShowPidCommand///
///==================================================================================================================///
ShowPidCommand::ShowPidCommand(const string cmd, bool takes_cpu) : BuiltInCommand(cmd.c_str(), takes_cpu) {}

void ShowPidCommand::execute() {
    //SmallShell& smash = SmallShell::getInstance();
    
    /// If any arguments were provided they will be ignored
    ///TODO: Check for errors from getpid system call
    int shellPID = getpid();
    if (shellPID < 0) {
        perror("smash error: getpid failed");
    }

    cout << shellPID << endl;

}
///==================================================================================================================///


                                            ///ChangeDirCommand///
///==================================================================================================================///
ChangeDirCommand::ChangeDirCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
                                                            BuiltInCommand(cmd.c_str(), takes_cpu), newWD(""), currentWD("") {
    SmallShell& smash = SmallShell::getInstance();

    ///TODO: What happens if getcwd system call fails? What error do we return?
    char *buf = nullptr;
    size_t bufSize = 0;
    string path;
    path = getcwd(buf, bufSize);
    currentWD = path;
    
    string lastWDCallBack = "-";
    string lastWD = smash.getLastWD();

    if(numOfArgs == 1) {
        newWD = currentWD;
    }
    else if(numOfArgs > 2){
        cout << "smash error: cd: too many arguments" << endl;
        newWD = currentWD;
    }
    // LastPWD isnt set and got "-" argument(havent yet cd in this instance)
    else if(lastWD == "" && args[1] == lastWDCallBack) {
        cout << "smash error: cd: OLDPWD not set" << endl;
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
    free(buf);
}

void ChangeDirCommand::execute() {

    SmallShell& smash = SmallShell::getInstance();

    if(newWD == currentWD) {
        return;
    }
    /// if res == 0 hakol tov
    int res = chdir(newWD.c_str());
    if (res != 0){
        // no cd has been done, so we keep lastWD as is
        perror("smash error: cd failed");
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
    if (numOfArgs > 3) {
        perror("smash error: too many arguments");      /// change?
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
    if ((src.compare("") == 0) || (dest.compare("") == 0)) return;

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
        perror("smash error: kill: invalid arguments");
    }
    char dash = *args[1];
    string inputNum =  args[1];
    inputNum = inputNum.substr(1);
    stringstream sigStr(inputNum);
    sigStr >> this->signal;
    //stringstream sigStr(args[1]);
    //sigStr >> dash >> this->signal;
    stringstream jobIdStr(args[2]);
    jobIdStr >> this->jobID;
    if (dash != '-' || signal <= 0 || jobID <= 0) {    /// change this condition to check the string!
        perror("smash error: kill: invalid arguments");
        signal = -1;
        jobID = -1;
    }
}


void KillCommand::execute() {
    if (signal < 0 || jobID < 0) return;

    JobsList::JobEntry* job = jobs->getJobById(jobID);
    if (!job) {
        string jobIdStr = to_string(jobID);
        string errMsg = "smash error: kill: job-id " + jobIdStr + " does not exist";
        perror(errMsg.c_str());
        return;
    }

    jobPID = job->getJobPID();

    int res = kill(jobPID, signal);
    if (res != 0) {
        perror("smash error: kill failed");
    } else {
        string sigStr = to_string(signal);
        string pidStr = to_string(jobPID);
        string msg = "signal number " + sigStr + " was sent to pid " + pidStr;
        cout << msg << endl;
        if(signal == SIGSTOP || signal == SIGTSTP) {
            job->stopJob();
        }
    }
}
///==================================================================================================================///


                                            ///ForegroundCommand///
///==================================================================================================================///
ForegroundCommand::ForegroundCommand(const char *cmd_line, char** args, int numOfArgs, JobsList *jobs, bool takes_cpu) :
                                                            BuiltInCommand(cmd_line, takes_cpu), jobID(-1), jobs(jobs) {
    /// check more options?
    if (numOfArgs > 2) {
        perror("smash error: fg: invalid arguments");
    }
    /// no job id given
    if (!args[1]) jobID = -1;

    /// job id given
    else {
        stringstream jobIdStr(args[1]);
        jobIdStr >> this->jobID;
            /// change this condition to check the string!
//        if (!isdigit(jobID)) {
//            perror("smash error: fg: invalid arguments");
//        }
    }
}

void ForegroundCommand::execute() {
    JobsList::JobEntry* job = nullptr;
    if (jobID > 0) {
        job = jobs->getJobById(jobID);
        if (!job) {
            string jobIdStr = to_string(jobID);
            string errMsg = "smash error: kill: job-id " + jobIdStr + "does not exist";
            perror(errMsg.c_str());
        }
    } else {        /// jobID == -1
        job = jobs->getLastJob(&jobID);
        if (!job) {
            perror("smash error: fg: jobs list is empty"); /// just print and return instead?
        }
    }

    pid_t pid = job->getJobPID();
    string cmd = job->getJobCmd();
    cout << cmd << " : " << pid << endl;

    bool isStopped = job->isJobStopped();
    if (isStopped) {
        job->resumeJob();
        kill(pid, SIGCONT);     /// we return the job to continue, else it is already running in the background
    }

    waitpid(pid, NULL, WUNTRACED);   /// "bring job to foreground" by waiting for it to finish
}
///==================================================================================================================///




                                            ///BackgroundCommand///
///==================================================================================================================///

BackgroundCommand::BackgroundCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu, JobsList* jobs) :
        BuiltInCommand(cmd.c_str(), takes_cpu), jobID(-1), jobToStopID(-1), jobs(jobs) {
    
    if(numOfArgs > 2) {
        perror("smash error: bg: invalid arguments");
        return;
    }

    //Checking legit argument 2
    if(args[1]) {
        stringstream strStm(args[1]);
        strStm >> jobID;
        if(jobID <= 0) {
            perror("smash error: bg: invalid arguments");
            return;
        } else { // Got a legit number in argument 2
            JobsList::JobEntry* job = jobs->getJobById(jobID);
                if(!job) {
                    cout << "smash error: bg: job-id " << jobID << " does not exist" <<endl;
                    jobToStopID = -1;
                    return;
                } else { // Found some Job ID
                    bool isStopped = job->isJobStopped();
                    if(!isStopped) {
                        cout << "smash error: bg: job-id " << jobID <<" is already running in the background" << endl;
                        jobToStopID = -1;
                        return;
                    } else { // Job is indeed stopped
                        this->jobToStopID = jobID;
                        this->jobToStop = job;
                    }
                }
        }
    } else { //Got only bg [no arguments]
        int foundStoppedID;
        JobsList::JobEntry* foundStoppedJob  = jobs->getLastStoppedJob(&foundStoppedID);
        if(numOfArgs == 1){
            if(!foundStoppedJob) {
                cout << "smash error: bg: there is no stopped jobs to resume" << endl;
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
    if(jobToStopID <= 0) {return;}

    string cmd = jobToStop->getJobCmd();
    int pidToCont;
    pidToCont = jobToStop->getJobPID();
    cout << cmd << " : " << pidToCont << endl;



    int res = kill(pidToCont, SIGCONT);
    if(res != 0) {
        perror("smash error: kill failed");
    } else {
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
    cout << "smash: sending SIGKILL to " << numKillsToDo << " jobs:" << endl;

    jobs->killAllJobs(); //printing done inside + kills. The messacare!
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

    char* const argv[4] = {const_cast<char*>("bash"), const_cast<char*>("-c"), cmdChar, NULL};
    execv("/bin/bash", argv);

    perror("smash error: execv failed");
}
///==================================================================================================================///


                                                ///SmallShell///
///==================================================================================================================///
SmallShell::SmallShell() : prompt("smash> "), lastWD(""), jobList(new JobsList()){
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
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
    size_t res = cmdStr.find("chprompt");
    if (res == 0) {
        command = new ChpromptCommand(cmd_line, args, numOfArgs, takes_cpu);
        return command;
    }
    res = cmdStr.find("pwd");
    if (res == 0) {
        command = new GetCurrDirCommand(cmd_line, takes_cpu);
        return command;
    }
    res = cmdStr.find("showpid");
    if (res == 0) {
        command = new ShowPidCommand(cmd_line, takes_cpu);
        return command;
    }
    res = cmdStr.find("cd");
    if (res == 0) {
        command = new ChangeDirCommand(cmd_line, args, numOfArgs, takes_cpu);
        return command;
    }
    res = cmdStr.find("cp");
    if (res == 0) {
        command = new CopyCommand(cmd_line, args, numOfArgs, takes_cpu);
        return command;
    }
    res = cmdStr.find("jobs");
    if (res == 0) {
        command = new JobsCommand(cmd_line, jobList, takes_cpu);
        return command;
    }
    res = cmdStr.find("kill");
    if (res == 0) {
        command = new KillCommand(cmd_line, args, numOfArgs, jobList, takes_cpu);   /// this is working but still need to change some checks
        return command;
    }
    res = cmdStr.find("fg");
    if (res == 0) {
        command = new ForegroundCommand(cmd_line, args, numOfArgs, jobList, takes_cpu); /// this is working but still need to change some checks
        return command;
    }
    res = cmdStr.find("bg");
    if (res == 0) {
        command = new BackgroundCommand(cmd_line, args, numOfArgs, takes_cpu, jobList);
        return command;
    }
    res = cmdStr.find("quit");
    if (res == 0) {
        command = new QuitCommand(cmd_line, args, numOfArgs, takes_cpu, jobList);
        return command;
    }
    else {
        command = new ExternalCommand(cmd_line, args, numOfArgs, takes_cpu);
    }

    /// should not get here
    return command;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);

    if(!cmd) return;

    //TODO: clear finished jobs

    bool isExternal = (typeid(*cmd) == typeid(ExternalCommand));
    bool isCopy = (typeid(*cmd) == typeid(CopyCommand));
    bool isQuit = (typeid(*cmd) == typeid(QuitCommand));
    if (isExternal || isCopy || isQuit) {
        pid_t pid = fork();

        if(pid == -1) {
            perror("smash error: fork failed");
            //TODO: return?
        }

        if(pid == 0){ //Child
            setpgrp();
            cmd->execute();
            delete cmd;
            exit(0);

        } else { //Parent
            //TODO: add job
            if(isQuit) {
                waitpid(pid, NULL, WUNTRACED);
                delete cmd;
                exit(0);
            }
            bool takesCPU = cmd->takesCPU();
            if (takesCPU) {
                waitpid(pid, NULL, WUNTRACED);
                delete cmd;
//                //remove
//                jobList->removeJobById(addedChildJobId);
            } else {
                //TODO: clear finished jobs
                jobList->removeFinishedJobs();
                jobList->addJob(cmd, pid, false);//isStopped=false
            }
            jobList->removeFinishedJobs();
        }
    }

    else {
        cmd->execute();
    }

}

string SmallShell::getPrompt() const { return prompt;}
void SmallShell::setPrompt(string newPrompt) {prompt = newPrompt;}
string SmallShell::getLastWD() const { return lastWD;}
void SmallShell::setLastWD(string path) {lastWD = path;}
///==================================================================================================================///
