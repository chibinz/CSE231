#pragma once

template <typename T>
static void mapInsertOrIncrement(std::map<T, int> &map, T key, int count) {
  if (map.find(key) == map.end()) {
    // On first time seeing key, insert new entry
    map.insert({key, count});
  } else {
    // Otherwise add count to current value
    map[key] += count;
  }
}