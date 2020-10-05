#include "fft.h"
#include "common.h"

cFFT::cFFT(unsigned int N) : N(N), reversed(0), W(0), pi2(2 * M_PI) {
  c[0] = c[1] = 0;

  log_2_N = log(N) / log(2);

  reversed = new unsigned int[N]; // prep bit reversals
  for (int i = 0; i < N; i++)
    reversed[i] = reverse(i);

  int pow2 = 1;
  W = new Complex *[log_2_N]; // prep W
  for (int i = 0; i < log_2_N; i++) {
    W[i] = new Complex[pow2];
    for (int j = 0; j < pow2; j++)
      W[i][j] = w(j, pow2 * 2);
    pow2 *= 2;
  }

  c[0] = new Complex[N];
  c[1] = new Complex[N];
  which = 0;
}

cFFT::~cFFT() {
  if (c[0])
    delete[] c[0];
  if (c[1])
    delete[] c[1];
  if (W) {
    for (int i = 0; i < log_2_N; i++)
      if (W[i])
        delete[] W[i];
    delete[] W;
  }
  if (reversed)
    delete[] reversed;
}

unsigned int cFFT::reverse(unsigned int i) {
  unsigned int res = 0;
  for (int j = 0; j < log_2_N; j++) {
    res = (res << 1) + (i & 1);
    i >>= 1;
  }
  return res;
}

Complex cFFT::w(unsigned int x, unsigned int N) {
  return Complex(cos(pi2 * x / N), sin(pi2 * x / N));
}

void cFFT::fft(Complex *input, Complex *output, int stride, int offset) {
  for (int i = 0; i < N; i++)
    c[which][i] = input[reversed[i] * stride + offset];

  int loops = N >> 1;
  int size = 1 << 1;
  int size_over_2 = 1;
  int w_ = 0;

  for (int i = 1; i <= log_2_N; i++) {
    which ^= 1;

#pragma omp parallel for num_threads(4)
    for (int j = 0; j < loops; j++) {
      for (int k = 0; k < size_over_2; k++) {
        c[which][size * j + k] =
            c[which ^ 1][size * j + k] +
            c[which ^ 1][size * j + size_over_2 + k] * W[w_][k];
      }
      for (int k = size_over_2; k < size; k++) {
        c[which][size * j + k] =
            c[which ^ 1][size * j - size_over_2 + k] -
            c[which ^ 1][size * j + k] * W[w_][k - size_over_2];
      }
    }

    loops >>= 1;
    size <<= 1;
    size_over_2 <<= 1;
    w_++;
  }

  for (int i = 0; i < N; i++)
    output[i * stride + offset] = c[which][i];
}
