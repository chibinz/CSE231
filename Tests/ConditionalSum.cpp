#include <iostream>
using namespace std;

int main() {

  // call foo(a=100)
  unsigned a = 100;
  unsigned x = 10;
  unsigned y = 10;
  unsigned i = 0;

  while (i++ < 10) {

    if (a > 5) {
      x = x + 5;
      y = y + 5;
    }
    else {
      x = x + 50;
      y = y + 50;
    }
  }

  cerr << "foo(a=100) returns: " << x + y + a << endl;
  
  return 0;
}
