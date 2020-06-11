#include <complex>
#include <iostream>
#include <valarray>

const double PI = 3.141592653589793238460;

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

// Cooley–Tukey FFT (in-place, divide-and-conquer)
// Higher memory requirements and redundancy although more intuitive
void fft(CArray &x) {
  const size_t N = x.size();
  if (N <= 1)
    return;

  // divide
  CArray even = x[std::slice(0, N / 2, 2)];
  CArray odd = x[std::slice(1, N / 2, 2)];

  // conquer
  fft(even);
  fft(odd);

  // combine
  for (size_t k = 0; k < N / 2; ++k) {
    Complex t = std::polar(1.0, -2 * PI * k / N) * odd[k];
    x[k] = even[k] + t;
    x[k + N / 2] = even[k] - t;
  }
}

// inverse fft (in-place)
void ifft(CArray &x) {
  // conjugate the complex numbers
  x = x.apply(std::conj);

  // forward fft
  fft(x);

  // conjugate the complex numbers again
  x = x.apply(std::conj);

  // scale the numbers
  x /= x.size();
}

void fft2(CArray &x) {}

int main() {
  // const Complex test[] = {1, 0, 1, 0, 1, 1, 1, 1};
  // CArray data(test, 8);
  //
  // // forward fft
  // fft(data);
  //
  // std::cout << "fft" << std::endl;
  // for (int i = 0; i < data.size(); ++i) {
  //   std::cout << data[i] << std::endl;
  // }
  //
  // // inverse fft
  // ifft(data);
  //
  // std::cout << std::endl << "ifft" << std::endl;
  // for (int i = 0; i < data.size(); ++i) {
  //   std::cout << data[i] << std::endl;
  // }

  Complex test[4][4] = {{0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}};

  // fft for each row
  for (size_t i = 0; i < 4; i++) {
    Complex temp[] = {test[i][0], test[i][1], test[i][2], test[i][3]};

    CArray data(temp, 4);

    fft(data);

    test[i][0] = data[0];
    test[i][1] = data[1];
    test[i][2] = data[2];
    test[i][3] = data[3];
  }

  // fft for each column
  for (size_t i = 0; i < 4; i++) {
    Complex temp[] = {test[0][i], test[1][i], test[2][i], test[3][i]};

    CArray data(temp, 4);

    fft(data);

    test[0][i] = data[0];
    test[1][i] = data[1];
    test[2][i] = data[2];
    test[3][i] = data[3];
  }

  for (size_t i = 0; i < 4; i++) {
    std::cout << test[i][0] << ", ";
    std::cout << test[i][1] << ", ";
    std::cout << test[i][2] << ", ";
    std::cout << test[i][3] << '\n';
  }

  return 0;
}
