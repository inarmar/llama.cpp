#pragma once

#include <optional>
#include <string>
#include <vector>

#include "utils.h"

namespace jinja {

// Each byte keeps its source through string operations.
struct string_part {
    bool is_input = false; // may skip parsing special tokens if true
    std::string val;

    bool is_uppercase() const;
    bool is_lowercase() const;
};

struct string {
    std::vector<string_part> parts;
    string() = default;
    string(const std::string & v, bool user_input = false) {
        parts.push_back({user_input, v});
    }
    string(int v) {
        parts.push_back({false, std::to_string(v)});
    }
    string(double v) {
        parts.push_back({false, std::to_string(v)});
    }

    // mark all parts as user input
    void mark_input();

    std::string str() const;
    size_t length() const;
    void hash_update(hasher & hash) const noexcept;
    bool is_uppercase() const;
    bool is_lowercase() const;

    string substr(size_t pos, size_t count = std::string::npos) const;
    bool has_input(size_t pos, size_t count) const;
    size_t size() const { return length(); }
    string operator[](size_t pos) const { return substr(pos, 1); }
    void push_back(const string & v) { append(v); }

    string & append(const string & other);

    // in-place transformations

    string uppercase();
    string lowercase();
    string capitalize();
    string titlecase();
    string strip(bool left, bool right, std::optional<const std::string_view> chars = std::nullopt);
};

} // namespace jinja
