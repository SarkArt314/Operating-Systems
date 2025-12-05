#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string line;
    if (!std::getline(std::cin, line)) return 0;
    std::istringstream iss(line);
    long long x;
    long long sum = 0;
    while (iss >> x) sum += x;
    std::cout << sum << '\n';
    return 0;
}
