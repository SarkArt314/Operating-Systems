#include "NumberLibrary.h"
#include <sstream>
#include <stdexcept>
#include <cmath>

Number::Number() : _num(0.0) {}
Number::Number(double value) : _num(value) {}

Number Number::operator+(const Number& other) const
{
	return Number(_num + other._num);
}

Number Number::operator-(const Number& other) const 
{
	return Number(_num - other._num);
}
Number Number::operator*(const Number& other) const 
{
	return Number(_num * other._num);
}
Number Number::operator/(const Number& other) const 
{
	if (other._num == 0.0) throw std::runtime_error("Division by zero");
	return Number(_num / other._num);
}

double Number::getValue() const
{
	return _num;
}

std::string Number::toString() const {
	std::ostringstream ss;
	ss << _num;
	return ss.str();
}

Number сreateNumber(double num)
{
	return Number(Number (num));
}

const Number NUMBER_ZERO(0.0);
const Number NUMBER_ONE(1.0);

Number numberSqrt(const Number& num) {
	if (num.getValue() < 0.0) throw std::domain_error("sqrt: negative argument");
	return Number(std::sqrt(num.getValue()));
}

Number numberAtan2(const Number& y, const Number& x) {
	return Number(std::atan2(y.getValue(), x.getValue()));
}
