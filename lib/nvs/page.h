#pragma once
#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "nvs.pb.h"

class NVSValue {
 public:
  bool is_tombstone = false;
  std::string string_val;
  union {
    uint32_t uint32_val;
    int32_t int32_val;
    uint64_t uint64_val;
    int64_t int64_val;
    float float_val;
    double double_val;
  } numeric_vals;
};

class NVSPage {
 public:
  NVSPage(size_t size) : size_(size){};

  bool load(const uint8_t* data);

 private:
  void parse_value_update(const ValueUpdate&);
  std::vector<std::string> dictionary_;
  std::vector<std::unique_ptr<NVSValue>> values_;

  const size_t size_;
  size_t sequence_number_;
  size_t erase_count_;
  bool is_finalized_;
};
