#ifndef VECTOR_DLL
#define VECTOR_DLL

#ifdef _WIN32
  #ifdef VECTORDLL_EXPORTS
    #define VECTORDLL_API __declspec(dllexport)
  #else
    #define VECTORDLL_API __declspec(dllimport)
  #endif
#else
  #ifdef VECTORDLL_EXPORTS
    #define VECTORDLL_API __attribute__((visibility("default")))
  #else
    #define VECTORDLL_API
  #endif
#endif

#include "NumberLibrary.h"
#include <string>

class VECTORDLL_API Vector {
public:
    Vector();

    Vector(const Number& x, const Number& y);


    Number getX() const;
    Number getY() const;

    Vector operator+(const Vector& other) const;

    Number radius() const;
    Number angle() const;

    std::string toString() const;

private:
    Number _x;
    Number _y;
};

extern VECTORDLL_API const Vector VECTOR_ZERO;
extern VECTORDLL_API const Vector VECTOR_ONE_ONE;

#endif // VECTOR_DLL
