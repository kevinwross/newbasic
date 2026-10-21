#pragma once

#include <cstddef>
#include <string>

#include "lwbasic/value.h"

namespace lwbasic {

// PRINT USING formatting (plan M8).
//
// Implemented and tested against the reference's rules and worked examples:
//   numeric fields   #  .  ,  leading +  trailing +  trailing -
//                    **  (asterisk fill)  $$  (sign beside the digits)  **$
//                    ^^^^  (exponent form)
//   string fields    !  &  \...\
  //   literals, `_` to print the next character even when it is a field character.
//   A literal trailing the last field of a cycle prints with the next value.
//
// The field width is the field text length, and a prefix such as `**` or `$$` also
// contributes digit positions. Status::Unsupported remains for field shapes the
// scanner cannot classify; the caller turns it into an "Advanced Feature" error.
enum class UsingStatus {
  Ok,
  Unsupported,  // field uses a construct that is not implemented yet
  TypeMismatch, // string value in a numeric field or vice versa
};

// Formats one value against the next field of `format`, appending literal text
// and the formatted field to *out. Advances *pos; when the format is exhausted it
// restarts from the beginning for the next value (the format cycles).
UsingStatus AppendUsing(std::string* out, const std::string& format, std::size_t* pos,
                        const Value& value);

// Formatters, exposed so they can be tested directly against the reference
// examples without going through the VM.
UsingStatus FormatUsingNumber(std::string* out, const std::string& field, double value);
UsingStatus FormatUsingString(std::string* out, const std::string& field, const std::string& value);

} // namespace lwbasic
