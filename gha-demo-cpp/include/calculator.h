#ifndef CALCULATOR_H
#define CALCULATOR_H

double add(double a, double b);
double subtract(double a, double b);
double multiply(double a, double b);
double divide(double a, double b);  // throws std::invalid_argument if b == 0

#endif  // CALCULATOR_H