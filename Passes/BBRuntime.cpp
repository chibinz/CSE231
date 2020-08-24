#include <iostream>
#include <map>

// Branch (taken, total) pair
std::pair<int, int> BrCount;

extern "C"
void __updateBrCount__(bool taken) {
  BrCount.first += static_cast<int>(taken);
  BrCount.second += 1;
}

extern "C"
void __printAndClearBrCount__() {
  std::cerr << "taken" << "\t" << BrCount.first << "\n";
  std::cerr << "total" << "\t" << BrCount.second << "\n";
  BrCount = {0, 0};
}