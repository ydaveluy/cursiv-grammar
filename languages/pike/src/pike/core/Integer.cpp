#include "pike/core/Integer.hpp"
#include <algorithm>
namespace pike {
std::optional<std::string> integer_value(std::string_view text) {
  if (text.empty())
    return {};
  unsigned base = 10;
  if (text.size() > 1 && text.front() == '0') {
    base = 8;
    if (text[1] == 'x' || text[1] == 'X') {
      base = 16;
      text.remove_prefix(2);
    } else if (text[1] == 'b' || text[1] == 'B') {
      base = 2;
      text.remove_prefix(2);
    }
  }
  if (text.empty())
    return {};
  std::string digits(1, '0');
  for (char c : text) {
    unsigned carry = c >= '0' && c <= '9'   ? c - '0'
                     : c >= 'a' && c <= 'f' ? c - 'a' + 10
                     : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                            : base;
    if (carry >= base)
      return {};
    for (char &d : digits) {
      const unsigned value = unsigned(d - '0') * base + carry;
      d = char('0' + value % 10);
      carry = value / 10;
    }
    while (carry) {
      digits.push_back(char('0' + carry % 10));
      carry /= 10;
    }
  }
  std::reverse(digits.begin(), digits.end());
  return digits;
}
} // namespace pike
