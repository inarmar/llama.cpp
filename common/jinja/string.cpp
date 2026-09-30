#include "jinja/string.h"
#include "jinja/value.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace jinja {

//
// string_part
//

bool string_part::is_uppercase() const {
    for (char c : val) {
        if (std::islower(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

bool string_part::is_lowercase() const {
    for (char c : val) {
        if (std::isupper(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

//
// string
//

void string::mark_input() {
    for (auto & part : parts) {
        part.is_input = true;
    }
}

std::string string::str() const {
    if (parts.size() == 1) {
        return parts[0].val;
    }
    std::ostringstream oss;
    for (const auto & part : parts) {
        oss << part.val;
    }
    return oss.str();
}

size_t string::length() const {
    size_t len = 0;
    for (const auto & part : parts) {
        len += part.val.length();
    }
    return len;
}

void string::hash_update(hasher & hash) const noexcept {
    for (const auto & part : parts) {
        hash.update(part.val.data(), part.val.length());
    }
}

bool string::is_uppercase() const {
    for (const auto & part : parts) {
        if (!part.is_uppercase()) {
            return false;
        }
    }
    return true;
}

bool string::is_lowercase() const {
    for (const auto & part : parts) {
        if (!part.is_lowercase()) {
            return false;
        }
    }
    return true;
}

string string::substr(size_t pos, size_t count) const {
    const size_t len = length();
    if (pos > len) {
        throw std::out_of_range("jinja string offset");
    }
    count = std::min(count, len - pos);
    string result;
    for (const auto & part : parts) {
        if (pos >= part.val.size()) {
            pos -= part.val.size();
            continue;
        }
        const size_t n = std::min(count, part.val.size() - pos);
        result.append(string(part.val.substr(pos, n), part.is_input));
        count -= n;
        pos = 0;
        if (count == 0) break;
    }
    return result;
}

bool string::has_input(size_t pos, size_t count) const {
    for (const auto & part : substr(pos, count).parts) {
        if (part.is_input) return true;
    }
    return false;
}

string & string::append(const string & other) {
    if (&other == this) {
        return append(string(other));
    }
    for (const auto & part : other.parts) {
        if (part.val.empty()) continue;
        if (!parts.empty() && parts.back().is_input == part.is_input) {
            parts.back().val += part.val;
        } else {
            parts.push_back(part);
        }
    }
    return *this;
}

// in-place transformation

using transform_fn = std::function<std::string(const std::string&)>;
static string apply_transform(string & self, const transform_fn & fn) {
    for (auto & part : self.parts) {
        part.val = fn(part.val);
    }
    return self;
}

string string::uppercase() {
    return apply_transform(*this, [](const std::string & s) {
        std::string res = s;
        std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return ::toupper(c); });
        return res;
    });
}
string string::lowercase() {
    return apply_transform(*this, [](const std::string & s) {
        std::string res = s;
        std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return ::tolower(c); });
        return res;
    });
}
string string::capitalize() {
    bool first = true;
    for (auto & part : parts) {
        for (char & c : part.val) {
            c = first ? ::toupper(static_cast<unsigned char>(c)) : ::tolower(static_cast<unsigned char>(c));
            first = false;
        }
    }
    return *this;
}

string string::titlecase() {
    bool capitalize_next = true;
    for (auto & part : parts) {
        for (char & c : part.val) {
            if (isspace(static_cast<unsigned char>(c))) {
                capitalize_next = true;
            } else {
                c = capitalize_next ? ::toupper(static_cast<unsigned char>(c)) : ::tolower(static_cast<unsigned char>(c));
                capitalize_next = false;
            }
        }
    }
    return *this;
}

string string::strip(bool left, bool right, std::optional<const std::string_view> chars) {
    static auto strip_part = [](const std::string & s, bool left, bool right, std::optional<const std::string_view> chars) -> std::string {
        size_t start = 0;
        size_t end = s.length();
        auto match_char = [&chars](unsigned char c) -> bool {
            return chars ? (*chars).find(c) != std::string::npos : isspace(c);
        };
        if (left) {
            while (start < end && match_char(static_cast<unsigned char>(s[start]))) {
                ++start;
            }
        }
        if (right) {
            while (end > start && match_char(static_cast<unsigned char>(s[end - 1]))) {
                --end;
            }
        }
        return s.substr(start, end - start);
    };
    if (parts.empty()) {
        return *this;
    }
    if (left) {
        for (size_t i = 0; i < parts.size(); ++i) {
            parts[i].val = strip_part(parts[i].val, true, false, chars);
            if (parts[i].val.empty()) {
                // remove empty part
                parts.erase(parts.begin() + i);
                --i;
                continue;
            } else {
                break;
            }
        }
    }
    if (right) {
        for (size_t i = parts.size(); i-- > 0;) {
            parts[i].val = strip_part(parts[i].val, false, true, chars);
            if (parts[i].val.empty()) {
                // remove empty part
                parts.erase(parts.begin() + i);
                continue;
            } else {
                break;
            }
        }
    }
    return *this;
}

} // namespace jinja
