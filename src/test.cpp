#include "fft.h"
#include <iostream>
#include <vector>

using namespace std;

typedef vector<vector<float>> Array2D1f;

int main(int argc, char const *argv[]) {

  Array2D1f a1f(2, vector<float>(3));

  std::cout << a1f[1][2] << '\n';

  return 0;
}
