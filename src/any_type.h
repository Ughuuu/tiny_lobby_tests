#pragma once
#include <variant>
#include <vector>
#include <unordered_map>
#include "yyjson.h"

struct AnyElement;

using VariantElement = std::variant<
    std::monostate,
    bool,
    int64_t,
    double,
    std::string,
    std::unordered_map<std::string, AnyElement>,
    std::vector<AnyElement>
>;

struct AnyElement {
    VariantElement value;

    yyjson_mut_val* to_yyjson(yyjson_mut_doc *doc, const AnyElement& elem) {
        if (std::holds_alternative<std::monostate>(elem.value)) {
            return yyjson_mut_null(doc);
        }
        else if (std::holds_alternative<bool>(elem.value)) {
            return yyjson_mut_bool(doc, std::get<bool>(elem.value));
        }
        else if (std::holds_alternative<int64_t>(elem.value)) {
            return yyjson_mut_int(doc, std::get<int64_t>(elem.value));
        }
        else if (std::holds_alternative<double>(elem.value)) {
            return yyjson_mut_real(doc, std::get<double>(elem.value));
        }
        else if (std::holds_alternative<std::string>(elem.value)) {
            return yyjson_mut_str(doc, std::get<std::string>(elem.value).c_str());
        }
        else if (std::holds_alternative<std::unordered_map<std::string, AnyElement>>(elem.value)) {
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            const auto& obj_map = std::get<std::unordered_map<std::string, AnyElement>>(elem.value);
            for (const auto& pair : obj_map) {
                yyjson_mut_val *value_val = to_yyjson(doc, pair.second);
                yyjson_mut_obj_add_val(doc, obj, pair.first.c_str(), value_val);
            }
            return obj;
        }
        else if (std::holds_alternative<std::vector<AnyElement>>(elem.value)) {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            const auto& arr_vec = std::get<std::vector<AnyElement>>(elem.value);
            for (const auto& item : arr_vec) {
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
