#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/vector.hpp>
#include <string>
#include <variant>

#include "yyjson.h"

#define EMPTY_STRING ""

struct AnyElement;

using VariantElement = std::variant<std::monostate, bool, int64_t, double, std::string,
                                    boost::container::flat_map<std::string, AnyElement>,
                                    boost::container::vector<AnyElement>>;

struct AnyElement {
    VariantElement value;

    yyjson_mut_val *to_yyjson(yyjson_mut_doc *doc, const AnyElement &elem) {
        if (std::holds_alternative<std::monostate>(elem.value)) {
            return yyjson_mut_null(doc);
        } else if (std::holds_alternative<bool>(elem.value)) {
            return yyjson_mut_bool(doc, std::get<bool>(elem.value));
        } else if (std::holds_alternative<int64_t>(elem.value)) {
            return yyjson_mut_int(doc, std::get<int64_t>(elem.value));
        } else if (std::holds_alternative<double>(elem.value)) {
            return yyjson_mut_real(doc, std::get<double>(elem.value));
        } else if (std::holds_alternative<std::string>(elem.value)) {
            return yyjson_mut_str(doc, std::get<std::string>(elem.value).c_str());
        } else if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                       elem.value)) {
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            const auto &obj_map =
                std::get<boost::container::flat_map<std::string, AnyElement>>(elem.value);
            for (const auto &pair : obj_map) {
                yyjson_mut_val *value_val = to_yyjson(doc, pair.second);
                yyjson_mut_obj_add_val(doc, obj, pair.first.c_str(), value_val);
            }
            return obj;
        } else if (std::holds_alternative<boost::container::vector<AnyElement>>(elem.value)) {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            const auto &arr_vec = std::get<boost::container::vector<AnyElement>>(elem.value);
            for (const auto &item : arr_vec) {
                yyjson_mut_val *value_val = to_yyjson(doc, item);
                yyjson_mut_arr_append(arr, value_val);
            }
            return arr;
        }
        return NULL;
    }

    std::string to_string() {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);

        yyjson_mut_val *root = to_yyjson(doc, *this);
        yyjson_mut_doc_set_root(doc, root);

        char *json = yyjson_mut_write(doc, 0, NULL);
        std::string json_str = json ? std::string(json) : "";

        if (json) {
            free(json);
        }

        yyjson_mut_doc_free(doc);

        return json_str;
    }
};
static bool operator==(const AnyElement &lhs, const AnyElement &rhs) {
    // Compare if both variants hold the same type and value.
    if (lhs.value.index() != rhs.value.index()) {
        return false;
    }

    if (std::holds_alternative<std::monostate>(lhs.value)) {
        return true;
    } else if (std::holds_alternative<bool>(lhs.value)) {
        return std::get<bool>(lhs.value) == std::get<bool>(rhs.value);
    } else if (std::holds_alternative<int64_t>(lhs.value)) {
        return std::get<int64_t>(lhs.value) == std::get<int64_t>(rhs.value);
    } else if (std::holds_alternative<double>(lhs.value)) {
        return std::get<double>(lhs.value) == std::get<double>(rhs.value);
    } else if (std::holds_alternative<std::string>(lhs.value)) {
        return std::get<std::string>(lhs.value) == std::get<std::string>(rhs.value);
    } else if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                   lhs.value)) {
        return std::get<boost::container::flat_map<std::string, AnyElement>>(lhs.value) ==
               std::get<boost::container::flat_map<std::string, AnyElement>>(rhs.value);
    } else if (std::holds_alternative<boost::container::vector<AnyElement>>(lhs.value)) {
        return std::get<boost::container::vector<AnyElement>>(lhs.value) ==
               std::get<boost::container::vector<AnyElement>>(rhs.value);
    }

    return false;
}
static bool operator!=(const AnyElement &lhs, const AnyElement &rhs) { return !(lhs == rhs); }

std::string decode_string_or_default(yyjson_val *object, std::string key,
                                     std::string default_value);
int decode_int_or_default(yyjson_val *object, std::string key, int default_value);
std::string decode_array(yyjson_val *array, AnyElement &element);
std::string decode_value(yyjson_val *value, AnyElement &element);
std::string decode_object(yyjson_val *object,
                          boost::container::flat_map<std::string, AnyElement> &dict);
bool decode_bool_or_default(yyjson_val *object, std::string key, bool default_value);
