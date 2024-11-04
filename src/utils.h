#ifndef HEADER_UTILS
#define HEADER_UTILS

#include <ArduinoEigen.h>
using namespace Eigen;

template<class T> inline Print &operator <<(Print &obj, T arg)  // no-cost stream operator as described at http://arduiniana.org/libraries/streaming/
{
    obj.print(arg);
    return obj;
}

void displayMat(const MatrixXf &m);

void displayVec(const VectorXf &v);

#endif