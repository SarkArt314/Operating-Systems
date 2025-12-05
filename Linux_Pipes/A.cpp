#include <iostream>
#include <sstream>
#include <string>

int main() {
    const long long N = 12;
    std::string line;
    if (!std::getline(std::cin, line)) return 0;
    std::istringstream iss(line);
    long long x;
    bool first = true;
    std::ostringstream oss;
    while (iss >> x) {
        if (!first) oss << ' ';
        oss << (x + N);
        first = false;
    }
    oss << '\n';
    std::cout << oss.str();
    return 0;
}
