#ifndef FFT_H
#define FFT_H

#include <math.h>
#include <complex>

using namespace std;

typedef complex<double> Complex;

class cFFT {
private:
  unsigned int N, which;
  unsigned int log_2_N;
  float pi2;
  unsigned int *reversed;
  Complex **W;
  Complex *c[2];

protected:
public:
  cFFT(unsigned int N);
  ~cFFT();
  unsigned int reverse(unsigned int i);
  Complex w(unsigned int x, unsigned int N);
  void fft(Complex *input, Complex *output, int stride, int offset);
};

#endif
