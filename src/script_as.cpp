#include "script_as.h"

#include <scriptany/scriptany.h>
#include <scriptarray/scriptarray.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptgrid/scriptgrid.h>
#include <scriptstdstring/scriptstdstring.h>

#include <variant>

CScriptArray *ConvertToArray(asIScriptEngine *engine,
                             const boost::container::vector<AnyElement> &elements);

CScriptDictionary *ConvertToDictionary(
    asIScriptEngine *engine, const boost::container::flat_map<std::string, AnyElement> &map) {
    auto *dict = CScriptDictionary::Create(engine);

    static int string_type_id = engine->GetTypeIdByDecl("const string &");
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
            dict->Set(key, (void *)v->c_str(), string_type_id);
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

    static int string_type_id = engine->GetTypeIdByDecl("const string &");
    static int dict_type_id = engine->GetTypeIdByDecl("dictionary@");
    static int array_type_id = engine->GetTypeIdByDecl("array<any>@");
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

boost::container::flat_set<std::string> ScriptAS::open() {
    if (enabled) {
        close();
    }
    boost::container::flat_set<std::string> empty_set;
    // do not open if folder_name is empty
    if (folder_name.empty()) {
        return empty_set;
    }

    as_engine = asCreateScriptEngine();
    if (!as_engine) {
        return empty_set;
    }

    // Setup basic configuration
    as_engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, false);
    as_engine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, true);
    RegisterScriptArray(as_engine, true);
    RegisterScriptDictionary(as_engine);
    RegisterScriptAny(as_engine);
    // TODO: Load and compile scripts
    enabled = true;

    return empty_set;
}

void ScriptAS::close() {
    if (as_context) {
        as_context->Release();
    }
    if (as_module) {
        as_module->Discard();
    }
    if (as_engine) {
        as_engine->ShutDownAndRelease();
    }

    as_context = nullptr;
    as_module = nullptr;
    as_engine = nullptr;
    enabled = false;
}

AnyElement ScriptAS::func_call(std::string &func_name, boost::container::vector<AnyElement> &args,
                               std::string &peer_id, std::string &lobby_id, std::string &game_id,
                               bool &has_error) {
    has_error = false;
    if (!enabled) {
        return AnyElement{"AngelScript not enabled"};
    }

    // Get the function from the module
    asIScriptFunction *func = as_module->GetFunctionByName(func_name.c_str());
    if (!func) {
        has_error = true;
        return AnyElement{"Function not found"};
    }

    // Prepare context
    if (!as_context) {
        as_context = as_engine->CreateContext();
    }
    int r = as_context->Prepare(func);
    if (r < 0) {
        has_error = true;
        return AnyElement{"Failed to prepare context"};
    }

    // Set arguments
    for (size_t i = 0; i < args.size(); i++) {
        const auto &value = args[i].value;

        if (auto v = std::get_if<int64_t>(&value)) {
            as_context->SetArgQWord(i, *v);
        } else if (auto v = std::get_if<double>(&value)) {
            as_context->SetArgDouble(i, *v);
        } else if (auto v = std::get_if<bool>(&value)) {
            as_context->SetArgByte(i, *v);
        } else if (auto v = std::get_if<std::string>(&value)) {
            as_context->SetArgObject(
                i, const_cast<char *>(v->c_str()));  // const_cast because AS takes void*
        } else if (auto v =
                       std::get_if<boost::container::flat_map<std::string, AnyElement>>(&value)) {
            CScriptDictionary *dict = ConvertToDictionary(as_engine, *v);
            as_context->SetArgObject(i, dict);
            dict->Release();  // AS will hold a reference
        } else if (auto v = std::get_if<boost::container::vector<AnyElement>>(&value)) {
            CScriptArray *arr = ConvertToArray(as_engine, *v);
            as_context->SetArgObject(i, arr);
            arr->Release();  // AS will hold a reference
        }
    }

    // Execute
    r = as_context->Execute();
    if (r != asEXECUTION_FINISHED) {
        has_error = true;
        return AnyElement{"Script execution failed"};
    }

    // Get return value
    if (func->GetReturnTypeId() != 0) {
        // TODO: Implement return value conversion
    }

    return AnyElement{std::monostate{}};  // Return empty if no return value
}
