#pragma once

#include <stddef.h>
#include <stdint.h>

class NonVolatileStorage {
 public:
  NonVolatileStorage(size_t sector_size, size_t sector_count)
      : sector_size_(sector_size), sector_count_(sector_count){};
  // Loads, parses and verifies up to sector_size_ * sector_count_ bytes from
  // provided buffer.
  void load(const uint8_t* data);

 private:
  size_t sector_size_;
  size_t sector_count_;
};