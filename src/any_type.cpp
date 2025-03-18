#include "any_type.h"


std::string decode_string_or_default(yyjson_val *object, std::string key, std::string default_value) {
    if (!object || !yyjson_is_obj(object)) {
        return default_value;
    }

    yyjson_val *value = yyjson_obj_get(object, key.c_str());
    if (!value || !yyjson_is_str(value)) {
        return default_value;
    }

    return std::string(yyjson_get_str(value));
}

int decode_int_or_default(yyjson_val *object, std::string key, int default_value) {
    if (!object || yyjson_get_type(object) != YYJSON_TYPE_OBJ) {
        return default_value;
    }

    yyjson_val *val = yyjson_obj_get(object, key.c_str());
    if (!val) return default_value;

    if (yyjson_is_int(val)) return static_cast<int>(yyjson_get_int(val));
    if (yyjson_is_uint(val)) return static_cast<int>(yyjson_get_uint(val));
    if (yyjson_is_real(val)) return static_cast<int>(yyjson_get_real(val));

    return default_value;
}

std::string decode_array(yyjson_val *array, AnyElement &element) {
    if (!array || yyjson_get_type(array) != YYJSON_TYPE_ARR) {
        return "Not an array";
    }

    std::vector<AnyElement> result;
    yyjson_val *item;
    size_t idx, max;
    max = yyjson_arr_size(array);
    yyjson_arr_foreach(array, idx, max, item) {
        AnyElement child;
        std::string error = decode_value(item, child);
        if (!error.empty()) return error;
        result.push_back(child);
    }

    element.value = result;
    return EMPTY_STRING;
}

std::string decode_value(yyjson_val *value, AnyElement &element) {
    if (!value){
        return "Null value";
    }

    switch (yyjson_get_type(value)) {
        case YYJSON_TYPE_NULL:
            element.value = std::monostate{};
            break;
        case YYJSON_TYPE_BOOL:
            element.value = yyjson_get_bool(value);
            break;
        case YYJSON_TYPE_NUM:
            if (yyjson_is_int(value)){
                element.value = yyjson_get_int(value);
            }
            else if (yyjson_is_uint(value)) {
                element.value = static_cast<int64_t>(yyjson_get_uint(value));
            }
            else {
                element.value = yyjson_get_real(value);
            }
            break;
        case YYJSON_TYPE_STR:
            element.value = std::string(yyjson_get_str(value));
            break;
        case YYJSON_TYPE_ARR: {
            std::string error = decode_array(value, element);
            if (!error.empty()) return error;
            break;
        }
        case YYJSON_TYPE_OBJ: {
            std::unordered_map<std::string, AnyElement> map_result;
            std::string error = decode_object(value, map_result);
            if (!error.empty()) return error;
            element.value = map_result;
            break;
        }
        default:
            return "Unknown JSON type";
    }

    return EMPTY_STRING;
}

std::string decode_object(yyjson_val *object, std::unordered_map<std::string, AnyElement> &dict) {
    if (!object || yyjson_get_type(object) != YYJSON_TYPE_OBJ) {
        return EMPTY_STRING;
    }

    yyjson_val *key, *val;
    yyjson_obj_iter iter = yyjson_obj_iter_with(object);
    while ((key = yyjson_obj_iter_next(&iter))) {
        val = yyjson_obj_iter_get_val(key);
        if (!yyjson_is_str(key)) {
            return "Non-string key in object";
        }

        AnyElement element;
        std::string error = decode_value(val, element);
        if (!error.empty()) return error;

        dict[yyjson_get_str(key)] = element;
    }

    return EMPTY_STRING;
}
