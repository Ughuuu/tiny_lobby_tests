#include "script_as.h"

#include <datetime/datetime.h>
#include <scriptany/scriptany.h>
#include <scriptarray/scriptarray.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptgrid/scriptgrid.h>
#include <scripthandle/scripthandle.h>
#include <scripthelper/scripthelper.h>
#include <scriptmath/scriptmath.h>
#include <scriptstdstring/scriptstdstring.h>
#include <weakref/weakref.h>

#include <variant>

#include "game_thread.h"
#include "script_as_functions.h"

void as_start_timer_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string timer_id = *static_cast<std::string *>(gen->GetArgObject(0));
    int duration = gen->GetArgDWord(1);
    CScriptArray *args = static_cast<CScriptArray *>(gen->GetArgObject(2));

    // Call the actual function
    self->as_start_timer(timer_id, duration, args);
}

void as_stop_timer_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string timer_id = *static_cast<std::string *>(gen->GetArgObject(0));

    // Call the actual function
    self->as_stop_timer(timer_id);
}

void as_notifty_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string peer_id = *static_cast<std::string *>(gen->GetArgObject(0));
    CScriptAny *message = static_cast<CScriptAny *>(gen->GetArgObject(1));

    // Call the actual function
    self->as_notifty(peer_id, message);
}

void as_broadcast_chat_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string message = *static_cast<std::string *>(gen->GetArgObject(0));

    // Call the actual function
    self->as_broadcast_chat(message);
}

void as_get_lobby_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    gen->SetReturnObject(self);
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

    enabled = true;
    as_engine = asCreateScriptEngine();
    if (!as_engine) {
        return empty_set;
    }

    // Setup basic configuration
    as_engine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, true);
    as_engine->SetEngineProperty(asEP_MAX_STACK_SIZE, 1024);
    RegisterStdString(as_engine);
    RegisterScriptArray(as_engine, true);
    RegisterScriptDictionary(as_engine);
    RegisterScriptAny(as_engine);
    RegisterScriptMath(as_engine);
    RegisterStdStringUtils(as_engine);
    RegisterScriptWeakRef(as_engine);
    RegisterScriptHandle(as_engine);
    RegisterScriptDateTime(as_engine);
    RegisterExceptionRoutines(as_engine);

    int r = as_engine->SetMessageCallback(asFUNCTION(as_MessageCallback), 0, asCALL_CDECL);
    if (r < 0) {
        std::cerr << "Failed to set message callback" << std::endl;
        close();
        return empty_set;
    }

    as_engine->RegisterGlobalFunction("void print(int)", asFUNCTION(as_print_int), asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(float)", asFUNCTION(as_print_float), asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(double)", asFUNCTION(as_print_double), asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(bool)", asFUNCTION(as_print_bool), asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(as_print_string), asCALL_CDECL);

    as_engine->RegisterObjectType("Lobby", 0, asOBJ_REF | asOBJ_NOCOUNT);

    // Register getters
    as_engine->RegisterObjectMethod("Lobby", "string get_id() const property",
                                    asMETHOD(ScriptAS, get_lobby_id), asCALL_THISCALL);

    as_engine->RegisterObjectMethod("Lobby", "string get_game_id() const property",
                                    asMETHOD(ScriptAS, get_game_id), asCALL_THISCALL);

    as_engine->RegisterObjectMethod("Lobby", "string get_calling_peer_id() const property",
                                    asMETHOD(ScriptAS, get_calling_peer_id), asCALL_THISCALL);

    r = as_engine->SetDefaultNamespace("lobby");
    as_engine->RegisterGlobalFunction(
        "void start_timer(string timer_id, int duration, array<any>@ args = array<any>())",
        asFUNCTION(as_start_timer_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void stop_timer(string timer_id)",
                                      asFUNCTION(as_stop_timer_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void notify(string peer_id, any@ message)",
                                      asFUNCTION(as_notifty_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void broadcast_chat(string message)",
                                      asFUNCTION(as_broadcast_chat_wrapper), asCALL_GENERIC);

    as_engine->RegisterGlobalFunction("Lobby@ get()", asFUNCTION(as_get_lobby_wrapper),
                                      asCALL_GENERIC);
    as_engine->SetDefaultNamespace("");
    as_engine->SetUserData(this);
    if (r < 0) {
        std::cerr << "Failed to register global function" << std::endl;
        close();
        return empty_set;
    }

    std::string base_path = scripts_folder + "/" + folder_name;
    std::string main_script_path = base_path + "/" + script_entrypoint;

    CScriptBuilder builder;
    r = builder.StartNewModule(as_engine, "Main");
    if (r < 0) {
        std::cerr << "Failed to start new module" << std::endl;
        close();
        return empty_set;
    }
    IncludeUserData includeData;
    includeData.base_path = base_path;
    builder.SetIncludeCallback(as_SecureIncludeCallback, &includeData);

    r = builder.AddSectionFromFile(main_script_path.c_str());
    if (r < 0) {
        std::cerr << "Failed to add section from file" << std::endl;
        close();
        return empty_set;
    }

    r = builder.BuildModule();
    if (r < 0) {
        std::cerr << "Failed to build module" << std::endl;
        close();
        return empty_set;
    }
    as_module = as_engine->GetModule("Main", asGM_ONLY_IF_EXISTS);

    for (int i = 0; i < as_module->GetFunctionCount(); i++) {
        asIScriptFunction *func = as_module->GetFunctionByIndex(i);
        as_functions.emplace(std::string(func->GetName()), i);
    }
    if (as_functions.contains("_on_create")) {
        empty_set.insert("_on_create");
    }
    if (as_functions.contains("_on_join")) {
        empty_set.insert("_on_join");
    }
    if (as_functions.contains("_on_chat")) {
        empty_set.insert("_on_chat");
    }
    if (as_functions.contains("_on_tags")) {
        empty_set.insert("_on_tags");
    }
    if (as_functions.contains("_on_kick")) {
        empty_set.insert("_on_kick");
    }
    if (as_functions.contains("_on_ready")) {
        empty_set.insert("_on_ready");
    }
    if (as_functions.contains("_on_seal")) {
        empty_set.insert("_on_seal");
    }
    if (as_functions.contains("_on_left")) {
        empty_set.insert("_on_left");
    }
    if (as_functions.contains("_on_tick")) {
        empty_set.insert("_on_tick");
    }
    return empty_set;
}

void ScriptAS::close() {
    if (as_engine) {

        as_engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE);
        as_engine->GarbageCollect(asGC_FULL_CYCLE);
    }
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
}

void ScriptAS::as_start_timer(std::string &timer_id, int duration, CScriptArray *arg) {
    if (duration < 1 || duration > 300) {
        std::cerr << "Invalid duration: " << duration << std::endl;
        return;
    }
    auto &game = game_thread->games[as_game_id];
    game.timer_data.insert_or_assign(timer_id,
                                     TimerData{
                                         .id = timer_id,
                                         .lobby_id = as_lobby_id,
                                         .game_id = as_game_id,
                                         .peer_id = as_peer_id,
                                         .end_time = duration * 1000 + game_thread->get_time(),
                                         .args = ConvertFromArray(as_engine, arg),
                                     });
}

void ScriptAS::as_stop_timer(std::string &timer_id) {
    auto &game = game_thread->games[as_game_id];
    game.timer_data.erase(timer_id);
}

void ScriptAS::as_notifty(std::string &peer_id, CScriptAny *message) {
    auto &game = game_thread->games[as_game_id];
    int typeId = message->GetTypeId();
    void *value = nullptr;
    message->Retrieve(&value, typeId);

    // Now convert properly
    auto message_obj = ConvertFromScriptType(as_engine, value, typeId);
    game_thread->notify_peer(game, as_lobby_id, as_peer_id, message_obj);
}

void ScriptAS::as_broadcast_chat(std::string &message) {
    auto &game = game_thread->games[as_game_id];
    game_thread->send_message(game, as_lobby_id, message);
}

AnyElement ScriptAS::func_call(std::string &func_name, boost::container::vector<AnyElement> &args,
                               std::string &peer_id, std::string &lobby_id, std::string &game_id,
                               bool &has_error) {
    has_error = false;
    as_peer_id = peer_id;
    as_lobby_id = lobby_id;
    as_game_id = game_id;
    if (!enabled) {
        return AnyElement{"AngelScript not enabled"};
    }
    if (autoreload) {
        // TODO
        //close();
        //open();
    }
    if (!as_functions.contains(func_name)) {
        has_error = true;
        return AnyElement{"Function not found"};
    }
    // Get the function from the module
    asIScriptFunction *func = as_module->GetFunctionByIndex(asUINT(as_functions[func_name]));
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

    if (args.size() != func->GetParamCount()) {
        return AnyElement{"Argument count mismatch"};
    }
    static int string_type_id = as_engine->GetTypeIdByDecl("string");
    static int dict_type_id = as_engine->GetTypeIdByDecl("dictionary");
    static int array_type_id = as_engine->GetTypeIdByDecl("array<any>");
    // Set arguments
    for (size_t i = 0; i < args.size(); i++) {
        int typeId;
        int r = func->GetParam(i, &typeId);
        if (r < 0) {
            has_error = true;
            return AnyElement{"Failed to get param"};
        }
        const auto &value = args[i].value;
        std::cout << typeId << std::endl;
        std::cout << string_type_id << std::endl;
        if (typeId == asTYPEID_INT64 || typeId == asTYPEID_UINT64) {
            if (auto v = std::get_if<int64_t>(&value)) {
                as_context->SetArgQWord(i, *v);
                continue;
            }
        } else if (typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32) {
            if (auto v = std::get_if<int64_t>(&value)) {
                as_context->SetArgDWord(i, (int)*v);
                continue;
            }
        } else if (typeId == asTYPEID_DOUBLE) {
            if (auto v = std::get_if<double>(&value)) {
                as_context->SetArgDouble(i, *v);
                continue;
            }
        } else if (typeId == asTYPEID_FLOAT) {
            if (auto v = std::get_if<double>(&value)) {
                as_context->SetArgFloat(i, (float)*v);
                continue;
            }
        } else if (typeId == asTYPEID_BOOL) {
            if (auto v = std::get_if<bool>(&value)) {
                as_context->SetArgByte(i, *v);
                continue;
            }
        } else if (typeId == string_type_id) {
            if (auto v = std::get_if<std::string>(&value)) {
                as_context->SetArgObject(i, (void *)v);
                continue;
            }
        } else if (typeId == dict_type_id) {
            if (auto v = std::get_if<boost::container::flat_map<std::string, AnyElement>>(&value)) {
                CScriptDictionary *dict = ConvertToDictionary(as_engine, *v);
                as_context->SetArgObject(i, dict);
                dict->Release();
                continue;
            }
        } else if (typeId == array_type_id) {
            if (auto v = std::get_if<boost::container::vector<AnyElement>>(&value)) {
                CScriptArray *arr = ConvertToArray(as_engine, *v);
                as_context->SetArgObject(i, arr);
                arr->Release();
                continue;
            }
        }
        has_error = true;
        return AnyElement{"Invalid argument type"};
    }

    // Execute
    r = as_context->Execute();
    if (r != asEXECUTION_FINISHED) {
        has_error = true;
        // The execution didn't complete as expected. Determine what happened.
        if (r == asEXECUTION_EXCEPTION) {
            // An exception occurred, let the script writer know what happened so it can be
            // corrected.
            return AnyElement{as_context->GetExceptionString()};
        }
        return AnyElement{"Execution failed"};
    }

    // Get return value
    if (func->GetReturnTypeId() != 0) {
        int typeId = func->GetReturnTypeId();

        if (typeId == asTYPEID_VOID) {
            return AnyElement{std::monostate{}};
        } else if (typeId == asTYPEID_BOOL) {
            return AnyElement{as_context->GetReturnByte() != 0};
        } else if (typeId == asTYPEID_INT8 || typeId == asTYPEID_UINT8 ||
                   typeId == asTYPEID_INT16 || typeId == asTYPEID_UINT16 ||
                   typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32) {
            return AnyElement{static_cast<int64_t>(as_context->GetReturnDWord())};
        } else if (typeId == asTYPEID_INT64 || typeId == asTYPEID_UINT64) {
            return AnyElement{static_cast<int64_t>(as_context->GetReturnQWord())};
        } else if (typeId == asTYPEID_FLOAT) {
            return AnyElement{static_cast<double>(as_context->GetReturnFloat())};
        } else if (typeId == asTYPEID_DOUBLE) {
            return AnyElement{as_context->GetReturnDouble()};
        } else {
            if (typeId & asTYPEID_OBJHANDLE) {
                void *obj = as_context->GetReturnObject();
                if (!obj) {
                    return AnyElement{"Unsupported return type"};
                }
                return ConvertFromScriptType(as_engine, obj, typeId);
            } else if (typeId & asTYPEID_MASK_OBJECT) {
                void *obj = as_context->GetReturnAddress();
                if (!obj) {
                    return AnyElement{"Unsupported return type"};
                }
                return ConvertFromScriptType(as_engine, obj, typeId);
            }
        }
        return AnyElement{"Unsupported return type"};
    }
    return AnyElement{std::monostate{}};  // Return empty if no return value
}
