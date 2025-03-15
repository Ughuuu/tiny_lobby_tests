#pragma once
#include <variant>
#include <vector>
#include <unordered_map>

struct AnyElement;

using VariantElement = std::variant<
    std::monostate,
    int64_t,
    double,
    std::string,
    std::unordered_map<std::string, AnyElement>,
    std::vector<AnyElement>
>;

struct AnyElement {
    VariantElement value;
};
