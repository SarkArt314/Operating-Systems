#include <iostream>
#include "NumberLibrary.h"
#include "VectorDLL.h"

int main()
{
    Number a = сreateNumber(3.0);
    Number b = сreateNumber(4.0);
    Number c = a * b + NUMBER_ONE;
    std::cout << "a = " << a.toString() << ", b = " << b.toString() << ", c = " << c.toString() << "\n";

    Vector v1 = VECTOR_ONE_ONE;
    Vector v2 = Vector(сreateNumber(2.0), сreateNumber(3.0));
    Vector v3 = v1 + v2;

    std::cout << "v1 = " << v1.toString() << "\n";
    std::cout << "v2 = " << v2.toString() << "\n";
    std::cout << "v3 = " << v3.toString() << "\n";

    Number r = v3.radius(); 
    Number ang = v3.angle(); 
    std::cout << "v3 radius = " << r.toString() << ", angle (rad) = " << ang.toString() << "\n";

    return 0;
}
