// Property-based tests (RapidCheck, plan section 6.2).
//
// RapidCheck master's Catch2 integration is rc::prop(description, testable)
// (extras/catch); the assertions inside the lambda are the core RC_ASSERT
// macros from <rapidcheck.h>.
#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "newbasic/line_store.h"
#include "newbasic/number_format.h"
#include "newbasic/value.h"

using newbasic::Value;

TEST_CASE("Property: FormatNumber(Int) is exact", "[property]") {
  rc::prop("sign prefix + exact digits", [](std::int16_t v) {
    const std::string s = newbasic::FormatNumber(Value::Int(v));
    RC_ASSERT(!s.empty());
    if (v >= 0) {
      RC_ASSERT(s[0] == ' ');
    } else {
      RC_ASSERT(s[0] == '-');
    }
    RC_ASSERT(s.substr(1) == std::to_string(std::abs(v)));
  });
}

TEST_CASE("Property: FormatNumber(Long) is exact", "[property]") {
  rc::prop("sign prefix + exact digits", [](std::int32_t v) {
    const std::string s = newbasic::FormatNumber(Value::Long(v));
    RC_ASSERT(!s.empty());
    RC_ASSERT(s[0] == ' ' || s[0] == '-');
    RC_ASSERT(s.substr(1) == std::to_string(std::abs(v)));
  });
}

TEST_CASE("Property: FormatNumber(Sgl) parses back within 7-digit tolerance", "[property]") {
  rc::prop("single-precision text round-trips within 1e-6 relative error", [](double v) {
    if (!std::isfinite(v))
      return;
    const std::string s = newbasic::FormatNumber(Value::Sgl(v));
    RC_ASSERT(!s.empty());
    RC_ASSERT(s[0] == ' ' || s[0] == '-');
    const std::string digits = s.substr(1);
    // Only characters the guide allows in numeric output.
    for (char c : digits) {
      RC_ASSERT(c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' ||
                c == '7' || c == '8' || c == '9' || c == '.' || c == 'E' || c == '+' || c == '-');
    }
    const double back = std::strtod(digits.c_str(), nullptr);
    const double a = std::fabs(v);
    if (a == 0.0) {
      RC_ASSERT(back == 0.0);
    } else {
      // Seven significant digits => relative error < 1e-6.
      RC_ASSERT(std::fabs(back - a) / a <= 1e-6);
    }
  });
}

TEST_CASE("Property: LineStore stays sorted and unique under arbitrary ops", "[property]") {
  rc::prop("line numbers remain sorted and unique",
           [](const std::vector<int>& numbers, const std::vector<bool>& add) {
             newbasic::LineStore store;
             const std::size_t n = std::min(numbers.size(), add.size());
             for (std::size_t i = 0; i < n; ++i) {
               const int line = numbers[i];
               if (line < 1 || line > 999999)
                 continue;
               if (add[i]) {
                 store.Add(line, "S");
               } else {
                 store.Remove(line);
               }
             }
             const auto& lines = store.Lines();
             for (std::size_t i = 1; i < lines.size(); ++i) {
               RC_ASSERT(lines[i - 1].line_number < lines[i].line_number);
             }
           });
}
