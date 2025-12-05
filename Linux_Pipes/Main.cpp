#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

void die(const char* msg) {
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(1);
}

int spawn_process(const char* path, int in_fd, int out_fd) {
    pid_t pid = fork();
    if (pid < 0) die("fork failed");
    if (pid == 0) {
        if (in_fd != STDIN_FILENO) {
            if (dup2(in_fd, STDIN_FILENO) < 0) die("dup2 in_fd");
        }
        if (out_fd != STDOUT_FILENO) {
            if (dup2(out_fd, STDOUT_FILENO) < 0) die("dup2 out_fd");
        }
        execl(path, path, (char*)nullptr);
        std::cerr << "execl failed for " << path << ": " << std::strerror(errno) << "\n";
        _exit(127);
    }
    return pid;
}

int main() {
    std::string inputLine = "1 2 3 4 5\n";

    int p_main_M[2];
    int p_M_A[2];
    int p_A_P[2];
    int p_P_S[2];
    int p_S_main[2];

    if (pipe(p_main_M) < 0) die("pipe main->M");
    if (pipe(p_M_A) < 0) die("pipe M->A");
    if (pipe(p_A_P) < 0) die("pipe A->P");
    if (pipe(p_P_S) < 0) die("pipe P->S");
    if (pipe(p_S_main) < 0) die("pipe S->main");

    pid_t pidM = spawn_process("./M", p_main_M[0], p_M_A[1]);
    pid_t pidA = spawn_process("./A", p_M_A[0], p_A_P[1]);
    pid_t pidP = spawn_process("./P", p_A_P[0], p_P_S[1]);
    pid_t pidS = spawn_process("./S", p_P_S[0], p_S_main[1]);

    close(p_main_M[0]);
    close(p_M_A[0]); close(p_M_A[1]);
    close(p_A_P[0]); close(p_A_P[1]);
    close(p_P_S[0]); close(p_P_S[1]);
    close(p_S_main[1]);

    ssize_t w = write(p_main_M[1], inputLine.data(), inputLine.size());
    if (w < 0) die("write to M");
    close(p_main_M[1]);

    std::string result;
    char buf[4096];
    ssize_t r;
    while ((r = read(p_S_main[0], buf, sizeof(buf))) > 0) {
        result.append(buf, buf + r);
    }
    if (r < 0) die("read from S");
    close(p_S_main[0]);

    int status;
    waitpid(pidM, &status, 0);
    waitpid(pidA, &status, 0);
    waitpid(pidP, &status, 0);
    waitpid(pidS, &status, 0);

    std::cout << "Final output from chain (S): " << result;

    return 0;
}
