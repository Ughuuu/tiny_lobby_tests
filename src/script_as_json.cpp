#include "script_as_json.h"

#include <yyjson.h>

#include <cassert>
#include <fstream>
#include <sstream>

#include "any_type.h"
#include "script_as.h"
#include "script_as_functions.h"

BEGIN_AS_NAMESPACE

// Decode from string
CScriptAny *DecodeJSON_from_string(const std::string &json) {
    yyjson_doc *doc = yyjson_read(json.c_str(), json.length(), 0);
    if (!doc) return nullptr;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root) {
        yyjson_doc_free(doc);
        return nullptr;
    }

    AnyElement root_value;
    std::string error = decode_value(root, root_value);
    if (!error.empty()) {
        yyjson_doc_free(doc);
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx) ctx->SetException(("JSON parse error: " + error).c_str());
        return nullptr;
    }

    asIScriptEngine *engine = asGetActiveContext()->GetEngine();
    CScriptAny *result = ConvertToAny(engine, root_value);

    yyjson_doc_free(doc);
    return result;
}

// Encode to string
std::string EncodeJSON_to_string(CScriptAny *any) {
    if (!any) return "";

    asIScriptEngine *engine = asGetActiveContext()->GetEngine();
    AnyElement val = ConvertFromAny(engine, any);
    return val.to_string();
}

// Decode from file
CScriptAny *DecodeJSON_from_file(const std::string &filename) {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());
    std::string base_path = self->scripts_folder + "/" + self->folder_name;
    if (filename.find("..") != std::string::npos) {
        ctx->SetException(("Filename invalid for: " + filename).c_str());
    }
    std::ifstream file(base_path + "/" + filename);
    if (!file.is_open()) {
        ctx->SetException(("Failed to open file: " + filename).c_str());
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return DecodeJSON_from_string(buffer.str());
}

// Encode to file
void EncodeJSON_to_file(CScriptAny *any, const std::string &filename) {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());
    std::string base_path = self->scripts_folder + "/" + self->folder_name;
    if (filename.find("..") != std::string::npos) {
        ctx->SetException(("Filename invalid for: " + filename).c_str());
        return;
    }
    std::string json = EncodeJSON_to_string(any);
    std::ofstream file(base_path + "/" + filename);
    if (!file.is_open()) {
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx) ctx->SetException(("Failed to write to file: " + filename).c_str());
        return;
    }

    file << json;
    file.close();
}

// Register global functions in JSON namespace
void RegisterScriptJSON(asIScriptEngine *engine) {
    int r = 0;
    r = engine->RegisterGlobalFunction("any@ decodeFromString(const string &in)",
                                       asFUNCTION(DecodeJSON_from_string), asCALL_CDECL);
    assert(r >= 0);
    r = engine->RegisterGlobalFunction("string encodeToString(any@)",
                                       asFUNCTION(EncodeJSON_to_string), asCALL_CDECL);
    assert(r >= 0);

    r = engine->RegisterGlobalFunction("any@ decodeFromFile(const string &in)",
                                       asFUNCTION(DecodeJSON_from_file), asCALL_CDECL);
    assert(r >= 0);
    r = engine->RegisterGlobalFunction("void encodeToFile(any@, const string &in)",
                                       asFUNCTION(EncodeJSON_to_file), asCALL_CDECL);
    assert(r >= 0);
}

END_AS_NAMESPACE
