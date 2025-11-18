// system sleep processes may appear
#include <bits/stdc++.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

using namespace std;

bool processExists(pid_t pid) {
    if (pid <= 0) return false;
    string statusPath = "/proc/" + to_string(pid) + "/status";
    ifstream f(statusPath);
    if (!f.is_open()) {
        return false;
    }
    string line;
    while (getline(f, line)) {
        if (line.rfind("State:", 0) == 0) {
            if (line.find('Z') != string::npos) {
                return false;
            } else {
                return true;
            }
        }
    }
    return true;
}


pid_t spawnSleep(int seconds = 300) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execl("/bin/sleep", "sleep", to_string(seconds).c_str(), (char*)NULL);
        _exit(127);
    }
    return pid;
}

int runKillerWithArgs(const vector<string>& args, int &outStatus) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        vector<char*> argv;
        argv.push_back(const_cast<char*>("./killer"));
        for (auto &s : args) argv.push_back(const_cast<char*>(s.c_str()));
        argv.push_back(nullptr);
        execv("./killer", argv.data());
        _exit(127);
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        outStatus = status;
        return pid;
    }
}

void waitShort() { usleep(100000); } 

int main() {
    int status;
    cout << "User demo starting\n";

    cout << "\n    Demo 1: kill by --id\n";
    pid_t s1 = spawnSleep(300);
    if (s1 <= 0) {
        cerr << "Failed to spawn sleep process for demo 1\n";
        return 1;
    }
    cout << "Spawned sleep pid=" << s1 << "\n";
    waitShort();
    cout << "Check exists before Killer: " << (processExists(s1) ? "yes" : "no") << "\n";

    string pidStr = to_string(s1);
    
    runKillerWithArgs({"--id", pidStr}, status);
    waitShort();
    waitpid(s1, nullptr, 0);
    cout << "Check exists after Killer: " << (processExists(s1) ? "yes" : "no") << "\n";

    cout << "\n    Demo 2: kill by --name \n";
    pid_t s2 = spawnSleep(300);
    pid_t s3 = spawnSleep(300);
    cout << "Spawned sleep pids=" << s2 << "," << s3 << "\n";
    waitShort();
    cout << "Exists before Killer: " << (processExists(s2) ? "yes" : "no") << ", " << (processExists(s3) ? "yes" : "no") << "\n";

    runKillerWithArgs({"--name", "sleep"}, status);
    waitShort();
    waitpid(s2, nullptr, 0);
    waitpid(s3, nullptr, 0);
    cout << "Exists after Killer: " << (processExists(s2) ? "yes" : "no") << ", " << (processExists(s3) ? "yes" : "no") << "\n";

    cout << "\n    Demo 3: PROCTOKILL env var and no args \n";
    setenv("PROCTOKILL", "sleep", 1);
    cout << "Set PROCTOKILL=sleep\n";

    pid_t s4 = spawnSleep(300);
    pid_t s5 = spawnSleep(300);
    cout << "Spawned sleep pids=" << s4 << "," << s5 << "\n";
    waitShort();
    cout << "Exists before Killer: " << (processExists(s4) ? "yes" : "no") << ", " << (processExists(s5) ? "yes" : "no") << "\n";

    runKillerWithArgs({}, status);
    waitShort();
    waitpid(s4, nullptr, 0);
    waitpid(s5, nullptr, 0);
    cout << "Exists after Killer: " << (processExists(s4) ? "yes" : "no") << ", " << (processExists(s5) ? "yes" : "no") << "\n";

    cout << "\n    Demo 4: PROCTOKILL multiple names \n";
    setenv("PROCTOKILL", "sleep,nonexistent", 1);
    cout << "Set PROCTOKILL=sleep,nonexistent\n";
    pid_t s6 = spawnSleep(300);
    cout << "Spawned sleep pid=" << s6 << "\n";
    waitShort();
    cout << "Exists before Killer: " << (processExists(s6) ? "yes" : "no") << "\n";
    runKillerWithArgs({}, status);
    waitShort();
    waitpid(s6, nullptr, 0);
    cout << "Exists after Killer: " << (processExists(s6) ? "yes" : "no") << "\n";

    cout << "\nRemoving PROCTOKILL environment variable\n";
    unsetenv("PROCTOKILL");
    if (getenv("PROCTOKILL") == nullptr) cout << "PROCTOKILL removed\n";
    else cout << "PROCTOKILL still present: " << getenv("PROCTOKILL") << "\n";

    cout << "\nUser demo finished.\n";
    return 0;
}
