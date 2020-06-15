#include "fft.h"
#include <iostream>
#include <vector>

using namespace std;

typedef vector<vector<float>> Array2D1f;

Array2D1f a1f;

int main(int argc, char const *argv[]) {
  a1f = Array2D1f(2, vector<float>(3));

  cout << a1f[1][1] << endl;

  return 0;
}
