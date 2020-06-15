#include "fft.h"
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char const *argv[]) {
  vector<int> a = {1, 2, 3};
  vector<int> b = a;
  a[0] -= 1;
  std::cout << b[0] << '\n';

  return 0;
}
