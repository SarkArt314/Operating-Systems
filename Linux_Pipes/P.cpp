#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string line;
    if (!std::getline(std::cin, line)) return 0;
    std::istringstream iss(line);
    long long x;
    bool first = true;
    std::ostringstream oss;
    while (iss >> x) {
        if (!first) oss << ' ';
        long long y = x * x * x;
        oss << y;
        first = false;
    }
    oss << '\n';
    std::cout << oss.str();
    return 0;
}
