#pragma once
#include <angelscript.h>

#include "server_logger.h"
#include "INIReader.h"
#include "scriptarray/scriptarray.h"
#include "scriptdictionary/scriptdictionary.h"
#include "scriptstdstring/scriptstdstring.h"

class ScriptAngelScript {
    bool enabled = false;
    asIScriptEngine *engine = nullptr;
    asIScriptModule *mod = nullptr;
    asIScriptFunction *func = nullptr;

   public:
    ScriptAngelScript(std::string &script_language, std::string &scripts_folder,
                      std::string &folder_name, std::string &script_entrypoint,
                      ServerLogger &logger, INIReader &config_reader) {
        if (config_reader.Get("game", "language", "lua") != "angelscript") {
            return;
        }

        // Create AngelScript engine
        engine = asCreateScriptEngine();
        RegisterStdString(engine);
        RegisterStdStringUtils(engine);
        RegisterScriptArray(engine, true);
        RegisterScriptDictionary(engine);
        if (!engine) {
            logger.error_log("[GameThread] Failed to create AngelScript engine");
            return;
        }

        // Create or load the script module
        mod = engine->GetModule("default", asGM_ALWAYS_CREATE);
        if (!mod) {
            engine->Release();
            logger.error_log("[GameThread] Failed to create AngelScript module");
            return;
        }

        // Load the script
        std::string script_path = scripts_folder + "/" + folder_name + "/" + script_entrypoint;
        /* TODO fix when enabling angelscript
        if (!std::filesystem::exists(script_path)) {
            logger.error_log("[GameThread] Script file does not exist: " + script_path);
            return;
        }
        */

        std::ifstream script_file(script_path);
        std::string script_content((std::istreambuf_iterator<char>(script_file)),
                                   std::istreambuf_iterator<char>());
        mod->AddScriptSection("my_script", script_content.c_str(), script_content.size(), 0);
        int build_result = mod->Build();
        if (build_result < 0) {
            logger.error_log("[GameThread] Failed to build AngelScript module");
            return;
        }

        // Retrieve the function to be called (e.g., 'main')
        func = mod->GetFunctionByName("main");
        if (!func) {
            logger.error_log("[GameThread] Function 'main' not found in the script");
            return;
        }

        enabled = true;
    }

    ~ScriptAngelScript() {
        if (engine) {
            engine->Release();
        }
    }

    AnyElement func_call(std::string &func_name, std::vector<AnyElement> &args) {
        if (!enabled) {
            return AnyElement{"AngelScript is not enabled"};
        }

        asIScriptContext *context = engine->CreateContext();
        if (!context) {
            context->Release();
            return AnyElement{"Failed to create AngelScript context"};
        }
        context->Prepare(func);

        for (size_t i = 0; i < args.size(); ++i) {
            const AnyElement &arg = args[i];
            if (std::holds_alternative<int64_t>(arg.value)) {
                context->SetArgDWord(i, std::get<int64_t>(arg.value));
            } else if (std::holds_alternative<double>(arg.value)) {
                context->SetArgFloat(i, std::get<double>(arg.value));
            } else if (std::holds_alternative<std::string>(arg.value)) {
                std::string str = std::get<std::string>(arg.value);
                context->SetArgObject(i, &str);
            } else if (std::holds_alternative<bool>(arg.value)) {
                context->SetArgByte(i, std::get<bool>(arg.value) ? 1 : 0);
            } else {
                return AnyElement{"Unsupported argument type"};
            }
        }

        int result = context->Execute();
        if (result != asEXECUTION_FINISHED) {
            context->Release();
            return AnyElement{"Error executing AngelScript function"};
        }

        // Get the result from the function call
        AnyElement return_value = decode_angelscript_value(context);

        context->Release();
        return return_value;
    }

    AnyElement decode_angelscript_value(asIScriptContext *context) {
        asDWORD flags;
        int type_id = func->GetReturnTypeId(&flags);
        switch (type_id) {
            case asTYPEID_BOOL: {
                bool value = context->GetReturnByte();
                return AnyElement{value};
            }
            case asTYPEID_INT8: {
                int64_t value = context->GetReturnByte();
                return AnyElement{value};
            }
            case asTYPEID_INT16: {
                int64_t value = context->GetReturnWord();
                return AnyElement{value};
            }
            case asTYPEID_INT32: {
                int64_t value = context->GetReturnDWord();
                return AnyElement{value};
            }
            case asTYPEID_INT64: {
                int64_t value = context->GetReturnQWord();
                return AnyElement{value};
            }

            case asTYPEID_FLOAT: {
                float value = context->GetReturnFloat();
                return AnyElement{value};
            }
            case asTYPEID_DOUBLE: {
                double value = context->GetReturnDouble();
                return AnyElement{value};
            }
            default: {
                // Handle unsupported types (if needed)
                return AnyElement{std::monostate{}};
            }
        }
    }
};
