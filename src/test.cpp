#include "fft.h"
#include <iostream>

using namespace std;

int main(int argc, char const *argv[]) {
  FFT fft(4);

  CArray2D test = {{1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}};

  /* FFT 2D */
  fft.fft2(test);

  std::cout << "after 2D fft:" << '\n';
  for (size_t i = 0; i < 4; i++) {
    std::cout << test[i][0] << ", ";
    std::cout << test[i][1] << ", ";
    std::cout << test[i][2] << ", ";
    std::cout << test[i][3] << '\n';
  }
  std::cout << '\n';

  /* IFFT 2D */
  fft.ifft2(test);

  std::cout << "after 2D ifft: " << '\n';
  for (size_t i = 0; i < 4; i++) {
    std::cout << test[i][0].real() << ", ";
    std::cout << test[i][1].real() << ", ";
    std::cout << test[i][2].real() << ", ";
    std::cout << test[i][3].real() << '\n';
  }

  return 0;
}
