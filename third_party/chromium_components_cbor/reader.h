// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CBOR_READER_H_
#define COMPONENTS_CBOR_READER_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "third_party/chromium_components_cbor/cbor_export.h"
#include "third_party/chromium_components_cbor/values.h"

// Concise Binary Object Representation (CBOR) decoder as defined by
// https://tools.ietf.org/html/rfc7049. This decoder only accepts canonical CBOR
// as defined by section 3.9.
//
// This implementation supports the following major types:
//  - 0: Unsigned integers, up to 64-bit values*.
//  - 1: Signed integers, up to 64-bit values*.
//  - 2: Byte strings.
//  - 3: UTF-8 strings.
//  - 4: Definite-length arrays.
//  - 5: Definite-length maps.
//  - 7: Simple values.
//
//  * Note: For simplicity, this implementation represents both signed and
//    unsigned integers with signed int64_t. This reduces the effective range
//    of unsigned integers.
//
// Requirements for canonical CBOR representation:
//  - Duplicate keys in maps are not allowed.
//  - Keys for maps must be sorted first by length and then by byte-wise
//    lexical order.
//
// Known limitations and interpretations of the RFC (and the reasons):
//  - Does not support indefinite-length data streams or semantic tags (major
//    type 6). (Simplicity; security)
//  - Does not support the floating point and BREAK stop code value types in
//    major type 7. (Simplicity)
//  - Does not support non-character codepoints in major type 3. (Security)
//  - Treats incomplete CBOR data items as syntax errors. (Security)
//  - Treats trailing data bytes as errors. (Security)
//  - Treats unknown additional information formats as syntax errors.
//    (Simplicity; security)
//  - Limits CBOR value inputs to at most 16 layers of nesting. Callers can
//    enforce more shallow nesting by setting |max_nesting_level|. (Efficiency;
//    security)
//  - Only supports CBOR maps with integer or string type keys, due to the
//    cost of serialization when sorting map keys. (Efficiency; simplicity)
//  - Does not support simple values that are unassigned/reserved as per RFC
//    7049, and treats them as errors. (Security)

namespace cbor {

class CBOR_EXPORT Reader {
 public:
  enum class DecoderError {
    CBOR_NO_ERROR = 0,
    UNSUPPORTED_MAJOR_TYPE,
    UNKNOWN_ADDITIONAL_INFO,
    INCOMPLETE_CBOR_DATA,
    INCORRECT_MAP_KEY_TYPE,
    TOO_MUCH_NESTING,
    INVALID_UTF8,
    EXTRANEOUS_DATA,
    OUT_OF_ORDER_KEY,
    NON_MINIMAL_CBOR_ENCODING,
    UNSUPPORTED_SIMPLE_VALUE,
    UNSUPPORTED_FLOATING_POINT_VALUE,
    OUT_OF_RANGE_INTEGER_VALUE,
    UNKNOWN_ERROR,
  };

  // CBOR nested depth sufficient for most use cases.
  static const int kCBORMaxDepth = 16;

  ~Reader();

  // Reads and parses |input_data| into a Value. Returns an empty Optional
  // if the input violates any one of the syntax requirements (including unknown
  // additional info and incomplete CBOR data).
  //
  // The caller can optionally provide |error_code_out| to obtain additional
  // information about decoding failures.
  //
  // If the caller provides it, |max_nesting_level| cannot exceed
  // |kCBORMaxDepth|.
  //
  // Returns an empty Optional if not all the data was consumed, and sets
  // |error_code_out| to EXTRANEOUS_DATA in this case.
  static absl::optional<Value> Read(std::vector<uint8_t> input_data,
                                    DecoderError* error_code_out = nullptr,
                                    int max_nesting_level = kCBORMaxDepth);

  // Never fails with EXTRANEOUS_DATA, but informs the caller of how many bytes
  // were consumed through |num_bytes_consumed|.
  static absl::optional<Value> Read(std::vector<uint8_t> input_data,
                                    size_t* num_bytes_consumed,
                                    DecoderError* error_code_out = nullptr,
                                    int max_nesting_level = kCBORMaxDepth);

  // Translates errors to human-readable error messages.
  static const char* ErrorCodeToString(DecoderError error_code);

 private:
  explicit Reader(std::vector<uint8_t> data);

  // Encapsulates information extracted from the header of a CBOR data item,
  // which consists of the initial byte, and a variable-length-encoded integer
  // (if any).
  struct DataItemHeader {
    // The major type decoded from the initial byte.
    Value::Type type;

    // The raw 5-bit additional information from the initial byte.
    uint8_t additional_info;

    // The integer |value| decoded from the |additional_info| and the
    // variable-length-encoded integer, if any.
    uint64_t value;
  };

  absl::optional<DataItemHeader> DecodeDataItemHeader();
  absl::optional<Value> DecodeCompleteDataItem(int max_nesting_level);
  absl::optional<Value> DecodeValueToNegative(uint64_t value);
  absl::optional<Value> DecodeValueToUnsigned(uint64_t value);
  absl::optional<Value> DecodeToSimpleValue(const DataItemHeader& header);
  absl::optional<uint64_t> ReadVariadicLengthInteger(uint8_t additional_info);
  absl::optional<Value> ReadByteStringContent(const DataItemHeader& header);
  absl::optional<Value> ReadStringContent(const DataItemHeader& header);
  absl::optional<Value> ReadArrayContent(const DataItemHeader& header,
                                         int max_nesting_level);
  absl::optional<Value> ReadMapContent(const DataItemHeader& header,
                                       int max_nesting_level);
  absl::optional<uint8_t> ReadByte();
  absl::optional<std::vector<uint8_t>> ReadBytes(uint64_t num_bytes);
  // TODO(crbug/879237): This function's only caller has to make a copy of a
  // `span<uint8_t>` to satisfy this function's interface. Maybe we can make
  // this function take a `const span<const uint8_t>` and avoid copying?
  bool HasValidUTF8Format(const std::string& string_data);
  bool IsKeyInOrder(const Value& new_key, Value::MapValue* map);
  bool IsEncodingMinimal(uint8_t additional_bytes, uint64_t uint_data);

  DecoderError GetErrorCode() { return error_code_; }

  size_t num_bytes_remaining() const { return rest_.size(); }

  std::vector<uint8_t> rest_;
  DecoderError error_code_;

  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
};

}  // namespace cbor

#endif  // COMPONENTS_CBOR_READER_H_
