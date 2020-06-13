#include <complex>
#include <iostream>
#include <valarray>
#include <vector>

using namespace std;

const double PI = 3.141592653589793238460;

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;
typedef valarray<valarray<Complex>> CArray2D;

class FFT {
public:
  int N;     // # of sample points
  CArray Wk; // lookup table for exponent terms

  FFT();
  FFT(int n) { N = n; }

  void fft(CArray &);
  void ifft(CArray &);
  void fft2(CArray2D &);
  void ifft2(CArray2D &);

  void calWk();
};
