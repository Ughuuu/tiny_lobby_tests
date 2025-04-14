
#include "script_as_functions.h"

CScriptArray *ConvertToArray(asIScriptEngine *engine,
                             const boost::container::vector<AnyElement> &elements);

CScriptDictionary *ConvertToDictionary(
    asIScriptEngine *engine, const boost::container::flat_map<std::string, AnyElement> &map) {
    auto *dict = CScriptDictionary::Create(engine);

    static int string_type_id = engine->GetTypeIdByDecl("string");
    static int dict_type_id = engine->GetTypeIdByDecl("dictionary@");
    static int array_type_id = engine->GetTypeIdByDecl("array<any>@");
    for (const auto &pair : map) {
        const auto &key = pair.first;
        const auto &value = pair.second.value;

        if (auto v = std::get_if<int64_t>(&value)) {
            dict->Set(key, (void *)v, asTYPEID_INT64);
        } else if (auto v = std::get_if<double>(&value)) {
            dict->Set(key, (void *)v, asTYPEID_DOUBLE);
        } else if (auto v = std::get_if<bool>(&value)) {
            dict->Set(key, (void *)v, asTYPEID_BOOL);
        } else if (auto v = std::get_if<std::string>(&value)) {
            dict->Set(key, (void *)v, string_type_id);
        } else if (auto submap =
                       std::get_if<boost::container::flat_map<std::string, AnyElement>>(&value)) {
            auto *sub_dict = ConvertToDictionary(engine, *submap);
            dict->Set(key, (void *)sub_dict, dict_type_id);
            sub_dict->Release();
        } else if (auto subvec = std::get_if<boost::container::vector<AnyElement>>(&value)) {
            auto *sub_array = ConvertToArray(engine, *subvec);
            dict->Set(key, (void *)sub_array, array_type_id);
            sub_array->Release();
        }
    }

    return dict;
}

CScriptArray *ConvertToArray(asIScriptEngine *engine,
                             const boost::container::vector<AnyElement> &elements) {
    asITypeInfo *type = engine->GetTypeInfoByDecl("array<any>");
    CScriptArray *arr = CScriptArray::Create(type, elements.size());

    static int string_type_id = engine->GetTypeIdByDecl("string");
    static int dict_type_id = engine->GetTypeIdByDecl("dictionary");
    static int array_type_id = engine->GetTypeIdByDecl("array<any>");
    for (size_t i = 0; i < elements.size(); ++i) {
        auto &elem = elements[i];

        if (auto v = std::get_if<int64_t>(&elem.value)) {
            arr->SetValue(i, (void *)v);
        } else if (const double *v = std::get_if<double>(&elem.value)) {
            arr->SetValue(i, (void *)v);
        } else if (const bool *v = std::get_if<bool>(&elem.value)) {
            arr->SetValue(i, (void *)v);
        } else if (const std::string *v = std::get_if<std::string>(&elem.value)) {
            arr->SetValue(i, (void *)v);
        } else if (const boost::container::vector<AnyElement> *v =
                       std::get_if<boost::container::vector<AnyElement>>(&elem.value)) {
            CScriptArray *nested = ConvertToArray(engine, *v);
            arr->SetValue(i, (void *)nested);
            nested->Release();
        } else if (const boost::container::flat_map<std::string, AnyElement> *v =
                       std::get_if<boost::container::flat_map<std::string, AnyElement>>(
                           &elem.value)) {
            auto *dict = ConvertToDictionary(engine, *v);
            arr->SetValue(i, (void *)dict);
            dict->Release();
        }
    }

    return arr;
}

void as_MessageCallback(const asSMessageInfo *msg, void *param) {
    const char *type = "ERR ";
    if (msg->type == asMSGTYPE_WARNING)
        type = "WARN";
    else if (msg->type == asMSGTYPE_INFORMATION)
        type = "INFO";
    printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

void as_print_int(int value) {
    std::cout << value << std::endl;
}

void as_print_float(float value) {
    std::cout << value << std::endl;
}

void as_print_double(double value) {
    std::cout << value << std::endl;
}

void as_print_bool(bool value) {
    std::cout << (value ? "true" : "false") << std::endl;
}

void as_print_string(const std::string &value) {
    std::cout << value << std::endl;
}
AnyElement ConvertFromScriptType(asIScriptEngine *engine, void *value, int typeId) {
    if (!value) {
        return AnyElement{"Uninitialized value"};
    }

    // Handle primitive types
    if (typeId == asTYPEID_VOID) {
        return AnyElement{std::monostate{}};
    } else if (typeId == asTYPEID_BOOL) {
        return AnyElement{*static_cast<bool *>(value)};
    } else if (typeId == asTYPEID_INT8 || typeId == asTYPEID_UINT8 || typeId == asTYPEID_INT16 ||
               typeId == asTYPEID_UINT16 || typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32) {
        return AnyElement{static_cast<int64_t>(*static_cast<int32_t *>(value))};
    } else if (typeId == asTYPEID_INT64 || typeId == asTYPEID_UINT64) {
        return AnyElement{*static_cast<int64_t *>(value)};
    } else if (typeId == asTYPEID_FLOAT) {
        return AnyElement{static_cast<double>(*static_cast<float *>(value))};
    } else if (typeId == asTYPEID_DOUBLE) {
        return AnyElement{*static_cast<double *>(value)};
    }

    // Handle object types
    int baseTypeId = typeId;
    bool isRef = false;
    if (typeId & asTYPEID_OBJHANDLE) {
        isRef = true;
        baseTypeId &= ~asTYPEID_OBJHANDLE;  // Strip the reference flag
    }

    asITypeInfo *typeInfo = engine->GetTypeInfoById(baseTypeId);
    if (!typeInfo) {
        return AnyElement{"Unknown object type"};
    }

    const std::string typeName = typeInfo->GetName();

    if (typeName == "string") {
        std::string result = *static_cast<std::string *>(value);
        return AnyElement{result};
    } else if (typeName == "dictionary") {
        return AnyElement{ConvertFromDictionary(engine, static_cast<CScriptDictionary *>(value))};
    } else if (typeName == "array") {
        return AnyElement{ConvertFromArray(engine, static_cast<CScriptArray *>(value))};
    } else if (typeName == "grid") {
        return AnyElement{ConvertFromGrid(engine, static_cast<CScriptGrid *>(value))};
    }

    return AnyElement{std::monostate{}};
}
boost::container::vector<AnyElement> ConvertFromGrid(asIScriptEngine *engine, CScriptGrid *grid) {
    boost::container::vector<AnyElement> result;

    if (!grid) return result;

    asUINT rows = grid->GetWidth();
    asUINT cols = grid->GetHeight();
    int typeId = grid->GetElementTypeId();

    result.reserve(rows);
    for (asUINT row = 0; row < rows; ++row) {
        boost::container::vector<AnyElement> row_array;
        row_array.reserve(cols);
        for (asUINT col = 0; col < cols; ++col) {
            void *value = grid->At(row, col);
            row_array.push_back(ConvertFromScriptType(engine, value, typeId));
        }
        result.push_back(AnyElement{row_array});
    }

    return result;
}

boost::container::flat_map<std::string, AnyElement> ConvertFromDictionary(asIScriptEngine *engine,
                                                                          CScriptDictionary *dict) {
    boost::container::flat_map<std::string, AnyElement> result;

    auto keys = dict->GetKeys();
    for (asUINT i = 0; i < keys->GetSize(); i++) {
        std::string key = static_cast<const char *>(keys->At(i));
        int typeId;
        void *value;
        if (dict->Get(key.c_str(), value, typeId)) {
            result[key] = ConvertFromScriptType(engine, value, typeId);
        }
    }
    keys->Release();
    return result;
}

boost::container::vector<AnyElement> ConvertFromArray(asIScriptEngine *engine, CScriptArray *arr) {
    boost::container::vector<AnyElement> result;
    result.reserve(arr->GetSize());

    for (asUINT i = 0; i < arr->GetSize(); i++) {
        void *value = arr->At(i);
        int typeId = arr->GetElementTypeId();

        result.push_back(ConvertFromScriptType(engine, value, typeId));
    }
    return result;
}

bool as_IsPathAllowed(const std::string &path) {
    if (path.find("../") != std::string::npos || path.find("..\\") != std::string::npos) {
        return false;
    }

    return true;
}

int as_SecureIncludeCallback(const char *includePath, const char *fromPath, CScriptBuilder *builder,
                             void *userParam) {
    IncludeUserData *data = static_cast<IncludeUserData *>(userParam);
    if (!as_IsPathAllowed(data->base_path + "/" + includePath)) {
        builder->GetEngine()->WriteMessage(fromPath, 0, 0, asMSGTYPE_ERROR,
                                           "Access denied: Parent directory traversal blocked");
        return -1;
    }
    return builder->AddSectionFromFile((data->base_path + "/" + includePath).c_str());
}
