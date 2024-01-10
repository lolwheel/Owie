#include "page.h"

#include "crc8.h"
#include "defer.h"
#include "nvs.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"

namespace {
class BufferStream {
 public:
  BufferStream(const uint8_t* buff, size_t size)
      : read_pos_(buff), last_readable_byte_(buff + size - 1){};

  bool read_byte(uint8_t* dest) {
    if (read_pos_ > last_readable_byte_) {
      return false;
    }
    *dest = *read_pos_;
    read_pos_++;
    return true;
  }

  size_t remaining_bytes() const { return last_readable_byte_ - read_pos_ + 1; }

  // Returns if crc8 of len bytes is zero.
  bool check_crc8_ahead(size_t len) {
    if (remaining_bytes() < len) {
      return false;
    }
    return Crc8(read_pos_, len) == 0;
  }

  // Based on nanopb code, thanks Petteri!
  bool read_varint_u32(uint32_t* dest) {
    uint8_t byte;
    if (!read_byte(&byte)) {
      return false;
    }

    if ((byte & 0x80) == 0) {
      /* Quick case, 1 byte value */
      *dest = byte;
      return true;
    }

    uint32_t result;

    uint32_t bitpos = 7;
    result = byte & 0x7F;

    do {
      if (!read_byte(&byte)) {
        return false;
      }

      if (bitpos >= 32) {
        /* Note: The varint could have trailing 0x80 bytes, or 0xFF for
         * negative. */
        uint8_t sign_extension = (bitpos < 63) ? 0xFF : 0x01;
        bool valid_extension =
            ((byte & 0x7F) == 0x00 ||
             ((result >> 31) != 0 && byte == sign_extension));

        if (bitpos >= 64 || !valid_extension) {
          return false;  // varint overflow
        }
      } else if (bitpos == 28) {
        if ((byte & 0x70) != 0 && (byte & 0x78) != 0x78) {
          return false;  // varint overflow
        }
        result |= (uint32_t)(byte & 0x0F) << bitpos;
      } else {
        result |= (uint32_t)(byte & 0x7F) << bitpos;
      }
      bitpos = (uint_fast8_t)(bitpos + 7);
    } while (byte & 0x80);

    *dest = result;
    return true;
  }

  bool read_next_message(const pb_msgdesc_t* fields, void* dest_struct) {
    // Read length of the message.
    uint32_t len;
    if (!read_varint_u32(&len)) {
      return false;
    }
    if (len > remaining_bytes()) {
      return false;
    }
    if (!check_crc8_ahead(len)) {
      return false;
    }
    // Header check ok.
    auto istream = pb_istream_from_buffer(read_pos_, len);
    if (!pb_decode(&istream, fields, dest_struct)) {
      return false;
    }
    read_pos_ += len;
    return true;
  }

 private:
  const uint8_t* read_pos_;
  const uint8_t* const last_readable_byte_;
};

}  // namespace

bool NVSPage::load(const uint8_t* const buff) {
  // Check header
  if (buff[0] != 0xFA && buff[1] != 0xDE) {
    return false;
  }
  BufferStream reader(buff + 2, size_);
  PageHeader header;
  if (!reader.read_next_message(&PageHeader_msg, &header)) {
    return false;
  }
  sequence_number_ = header.seq_no;
  erase_count_ = header.erase_count;

  ValueUpdate val;
  while (reader.read_next_message(&ValueUpdate_msg, &val)) {
    parse_value_update(val);
  }
  return true;
}

void NVSPage::parse_value_update(const ValueUpdate& val) {
  if (val.has_name) {
    dictionary_.reserve(val.id + 1);
    dictionary_[val.id] = std::string(val.name);
  }
  auto parsed_value = std::make_unique<NVSValue>();
  if (val.has_fixed32_val) {
    parsed_value->numeric_vals.uint32_val = val.fixed32_val;
  } else if (val.has_fixed64_val) {
    parsed_value->numeric_vals.uint64_val = val.fixed64_val;
  } else if (val.has_sint64_val) {
    parsed_value->numeric_vals.int64_val = val.sint64_val;
  } else if (val.has_uint64_val) {
    parsed_value->numeric_vals.uint64_val = val.uint64_val;
  } else if (val.has_bytes) {
    parsed_value->string_val =
        std::string((const char*)(val.bytes.bytes), val.bytes.size);
  } else {
    parsed_value->is_tombstone = true;
  }
  values_.reserve(val.id + 1);
  values_[val.id] = std::move(parsed_value);
}
