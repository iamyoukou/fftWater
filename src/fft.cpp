#include "fft.h"

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

void fft2(CArray2D &x) {
  int nOfRows = x.size();
  int nOfCols = x[0].size();

  // fft for each row
  for (size_t i = 0; i < nOfRows; i++) {
    CArray data(nOfRows);

    // implant data
    for (size_t j = 0; j < data.size(); j++) {
      data[j] = x[i][j];
    }

    fft(data);

    // update data
    for (size_t j = 0; j < data.size(); j++) {
      x[i][j] = data[j];
    }
  } // end fft for each row

  // fft for each column
  for (size_t i = 0; i < nOfCols; i++) {
    CArray data(nOfCols);

    // implant data
    for (size_t j = 0; j < data.size(); j++) {
      data[j] = x[j][i];
    }

    fft(data);

    // update data
    for (size_t j = 0; j < data.size(); j++) {
      x[j][i] = data[j];
    }
  } // end fft for each column
}

void ifft2(CArray2D &x) {
  int nOfRows = x.size();
  int nOfCols = x[0].size();

  // ifft for each row
  for (size_t i = 0; i < nOfRows; i++) {
    CArray data(nOfRows);

    // implant data
    for (size_t j = 0; j < data.size(); j++) {
      data[j] = x[i][j];
    }

    ifft(data);

    // update data
    for (size_t j = 0; j < data.size(); j++) {
      x[i][j] = data[j];
    }
  } // end ifft for each row

  // ifft for each column
  for (size_t i = 0; i < nOfCols; i++) {
    CArray data(nOfCols);

    // implant data
    for (size_t j = 0; j < data.size(); j++) {
      data[j] = x[j][i];
    }

    ifft(data);

    // update data
    for (size_t j = 0; j < data.size(); j++) {
      x[j][i] = data[j];
    }
  } // end ifft for each column
}

/* Test code */
// CArray2D test = {{0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}};
//
// /* FFT 2D */
// fft2(test);
//
// std::cout << "after 2D fft:" << '\n';
// for (size_t i = 0; i < 4; i++) {
//   std::cout << test[i][0] << ", ";
//   std::cout << test[i][1] << ", ";
//   std::cout << test[i][2] << ", ";
//   std::cout << test[i][3] << '\n';
// }
// std::cout << '\n';
//
// /* IFFT 2D */
// ifft2(test);
//
// std::cout << "after 2D ifft: " << '\n';
// for (size_t i = 0; i < 4; i++) {
//   std::cout << test[i][0].real() << ", ";
//   std::cout << test[i][1].real() << ", ";
//   std::cout << test[i][2].real() << ", ";
//   std::cout << test[i][3].real() << '\n';
// }
