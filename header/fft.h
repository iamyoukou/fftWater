#include <complex>
#include <iostream>
#include <valarray>

using namespace std;

const double PI = 3.141592653589793238460;

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;
typedef valarray<valarray<Complex>> CArray2D;

void fft(CArray &);
void ifft(CArray &);
void fft2(CArray2D &);
void ifft2(CArray2D &);
