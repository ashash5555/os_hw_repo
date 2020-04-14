#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>



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
    vector<JobsList::JobEntry*>::iterator tempIt = it;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            delete *it;
            *it = nullptr;
            /// because iterator is undefined after erase
            tempIt = it;
            jobs.erase(it);
            it = tempIt;
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
            cout << *it;
        }
    }
}

void JobsList::killAllJobs() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    vector<JobsList::JobEntry*>::iterator tempIt = it;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            kill(pid, SIGKILL);    /// check return status?

            delete *it;
            *it = nullptr;
            /// because iterator is undefined after erase
            tempIt = it;
            jobs.erase(it);
            it = tempIt;
        }
    }
    updateJobsCount();
}

void JobsList::removeFinishedJobs() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    vector<JobsList::JobEntry*>::iterator tempIt = it;
    int res;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            /// waitpid with WNOHANG returns the pid if the process finished and 0 if it is still running
            res = waitpid(pid, nullptr, WNOHANG);
            if (res > 0) {
                /// gets here if this process finished
                delete *it;
                *it = nullptr;
                /// because iterator is undefined after erase
                tempIt = it;
                jobs.erase(it);
                it = tempIt;
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
GetCurrDirCommand::GetCurrDirCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
                                                            BuiltInCommand(cmd.c_str(), takes_cpu) {}

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
        std::cout << path << std::endl;
    }
    free(buff);
}
///==================================================================================================================///

                                            ///ShowPidCommand///
///==================================================================================================================///
ShowPidCommand::ShowPidCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
                                                            BuiltInCommand(cmd.c_str(), takes_cpu) {}

void ShowPidCommand::execute() {
    //SmallShell& smash = SmallShell::getInstance();
    
    /// If any arguments were provided they will be ignored
    ///TODO: Check for errors from getpid system call
    int shellPID = getpid();
    std::cout << shellPID << std::endl;

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

    if(numOfArgs == 1) {
        newWD = currentWD;
    }
    else if(numOfArgs > 2){
        std::cout << "smash error: cd: too many arguments" << std::endl;
        newWD = currentWD;
    }
    // LastPWD isnt set and got "-" argument(havent yet cd in this instance)
    else if(smash.getLastWD() == "" && args[1] == lastWDCallBack) {
        std::cout << "smash error: cd: OLDPWD not set" << std::endl;
        newWD = currentWD;
    }
    // Handling case if we got "-" as the first argument (and already have done at least one cd call)
    else if(args[1] == lastWDCallBack){
        newWD = smash.getLastWD();
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
    else if(chdir(newWD.c_str()) != 0){
        // no cd has been done, so we keep lastWD as is
        perror("smash error: cd failed");
    }

    //chdir has succeeded and we need to remember the lastWD before cd has been done (saved in currentWD in constructor)
    else{
        smash.setLastWD(currentWD);
    }
    
}
///==================================================================================================================///



///==================================================================================================================///

                                            ///ExternalCommand///
///==================================================================================================================///



ExternalCommand::ExternalCommand(const string cmd, char **args, int numOfArgs, bool takes_cpu) :
                                                            Command(cmd.c_str(), takes_cpu) {

    char* firstArg = (char*)malloc(3); ///TODO: Delete here

    memset(firstArg, 0, 3);
    strcpy(firstArg, "-c");
    bashArgs.push_back(firstArg);
    for(int i=0; i < numOfArgs; i++){
        bashArgs.push_back(args[i]);
    }
    bashArgs.push_back(nullptr);                                          
}



void ExternalCommand::execute() {

    ///TODO: What happens in the jobslist?
    SmallShell& smash = SmallShell::getInstance();

    pid_t pid = fork();

    if(pid == -1){
        perror("smash error: fork failed");
    }

    // fork the shell to run the external program
    if(pid == 0){ //child's code
        execv("/bin/bash", &bashArgs.front());

    } else {
        if(takesCPU() == true){ // in the front
            ///TODO: Question: Should it be waitpid or wait?
            waitpid(pid, NULL, );
    }
        // 
    // check if the command should run in bg and the child needs to
        // if so, fork the current shell
        // add to the jobs list
    // 
    

}
///==================================================================================================================///






















                                                ///SmallShell///
///==================================================================================================================///
SmallShell::SmallShell() : prompt("smash> "), lastWD(""){
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
    }
    res = cmdStr.find("pwd");
    if (res == 0) {
        command = new GetCurrDirCommand(cmd_line, args, numOfArgs, takes_cpu);
    }
    res = cmdStr.find("showpid");
    if (res == 0) {
        command = new ShowPidCommand(cmd_line, args, numOfArgs, takes_cpu);
    }
   res = cmdStr.find("cd");
   if (res == 0) {
       command = new ChangeDirCommand(cmd_line, args, numOfArgs, takes_cpu);
   }
//   res = cmdStr.find("jobs");
//   if (res == 0) {
//       command = new JobsCommand(cmd_line);
//   }
//    res = cmdStr.find("kill");
//    if (res == 0) {
//        command = new KillCommand(cmd_line);
//    }
//    res = cmdStr.find("fg");
//    if (res == 0) {
//        command = new ForegroundCommand(cmd_line);
//    }
//    res = cmdStr.find("bg");
//    if (res == 0) {
//        command = new BackgroundCommand(cmd_line);
//    }
//    res = cmdStr.find("quit");
//    if (res == 0) {
//        command = new QuitCommand(cmd_line);
//    }
    else {
        command = new ExternalCommand(cmd_line, args, numOfArgs, takes_cpu);
    }

    /// should not get here
    return command;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPrompt() const { return prompt;}
void SmallShell::setPrompt(string newPrompt) {prompt = newPrompt;}
string SmallShell::getLastWD() const { return lastWD;}
void SmallShell::setLastWD(string path) {lastWD = path;}
///==================================================================================================================///
