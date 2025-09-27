#ifndef NUMBER_LIBRARY
#define NUMBER_LIBRARY

#include <iostream>
#include <string>

class Number
{
public:
	Number();

	explicit Number(double value);

	Number operator+(const Number& other) const;

	Number operator-(const Number& other) const;

	Number  operator*(const Number& other) const;

	Number operator/(const Number& other) const;

	std::string toString() const;

	double getValue() const;

private:
	double _num;
};

extern const Number NUMBER_ZERO;
extern const Number NUMBER_ONE;
Number ñreateNumber(double num);

Number numberSqrt(const Number& num);
Number numberAtan2(const Number& y, const Number& x);

#endif // NUMBER_LIBRARY