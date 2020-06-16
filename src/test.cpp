#include "fft.h"
#include <iostream>
#include <vector>

using namespace std;

typedef vector<vector<float>> Array2D1f;

int main(int argc, char const *argv[]) {

  Complex c(1, 2);
  c *= 2.f;

  std::cout << c << '\n';

  return 0;
}
