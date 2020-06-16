#include <complex>
#include <iostream>
#include <valarray>
#include <vector>

using namespace std;

const double PI = 3.141592653589793238460;

typedef complex<double> Complex;
typedef valarray<Complex> CArray;
typedef valarray<valarray<Complex>> CArray2D;

class FFT {
public:
  // # of sample points along one axis
  int N;
  float Lx;

  // a lookup table for exponent terms
  CArray Wk;

  FFT();
  FFT(int);

  void fft(CArray &);
  void ifft(CArray &);
  void fft2(CArray2D &);
  void ifft2(CArray2D &);

  void computeWk();
};
