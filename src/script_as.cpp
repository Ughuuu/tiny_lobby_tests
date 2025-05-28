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
#include "script_as_http.h"
#include "script_as_json.h"
#include "script_as_scriptfile.h"
#include "script_lua_functions.h"

void as_start_timer_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string timer_id = *static_cast<std::string *>(gen->GetArgObject(0));
    double duration = gen->GetArgDouble(1);
    CScriptArray *args = static_cast<CScriptArray *>(gen->GetArgObject(2));
    // Call the actual function
    self->as_start_timer(timer_id, duration, args);
    if (args != nullptr) {
        args->Release();
    }
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
    self->as_broadcast_chat(message, nullptr);
}

void as_broadcast_chat_wrapper_with_metadata(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string message = *static_cast<std::string *>(gen->GetArgObject(0));
    CScriptDictionary *metadata_dict = static_cast<CScriptDictionary *>(gen->GetArgObject(1));
    self->as_broadcast_chat(message, metadata_dict);
}

void as_kick_peer_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    // Extract arguments
    std::string peer_id = *static_cast<std::string *>(gen->GetArgObject(0));

    // Call the actual function
    self->as_kick_peer(peer_id);
}

void as_get_lobby_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    gen->SetReturnObject(new LobbyAS{
        .type = LobbyAS::LOBBY_ROOT,
        .as_engine = self->as_engine,
        .as_context = self->as_context,
        .as_module = self->as_module,
        .game_thread = self->game_thread,
        .as_game_id = self->as_game_id,
        .as_lobby_id = self->as_lobby_id,
        .as_calling_peer_id = self->as_peer_id,
    });
}

void as_get_ticks_ms_wrapper(asIScriptGeneric *gen) {
    // Retrieve the ScriptAS instance from the engine's user data
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());
    auto &game_thread = self->game_thread;
    auto &game = game_thread->games[self->as_game_id];
    auto &lobby = game.lobbies[self->as_lobby_id];
    gen->SetReturnQWord((asQWORD)self->game_thread->get_time());
}

void ScriptAS::as_MessageCallback(const asSMessageInfo *msg, void *param) {
    const char *type = "ERR ";
    if (msg->type == asMSGTYPE_WARNING)
        type = "WARN";
    else if (msg->type == asMSGTYPE_INFORMATION)
        type = "INFO";
    // error_msg += sprintf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type,
    // msg->message);
    printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

boost::container::flat_set<std::string> ScriptAS::open() {
    as_functions.clear();

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
    RegisterScriptFileCustom_Generic(as_engine);
    RegisterScriptJSON(as_engine);
    RegisterHTTPInterface(as_engine);

    int r = as_engine->SetMessageCallback(asMETHOD(ScriptAS, as_MessageCallback), this,
                                          asCALL_THISCALL);
    if (r < 0) {
        std::cerr << "Failed to set message callback" << std::endl;
        close();
        return empty_set;
    }

    as_engine->RegisterGlobalFunction("void print(int64)", asFUNCTION(as_print_int64),
                                      asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(double)", asFUNCTION(as_print_double),
                                      asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(bool)", asFUNCTION(as_print_bool), asCALL_CDECL);
    as_engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(as_print_string),
                                      asCALL_CDECL);
    // Lobby
    as_engine->RegisterObjectType("Lobby", 0, asOBJ_REF);
    as_engine->RegisterObjectBehaviour("Lobby", asBEHAVE_ADDREF, "void f()",
                                       asMETHOD(LobbyAS, AddRef), asCALL_THISCALL);
    as_engine->RegisterObjectBehaviour("Lobby", asBEHAVE_RELEASE, "void f()",
                                       asMETHOD(LobbyAS, Release), asCALL_THISCALL);
    // LobbyData
    as_engine->RegisterObjectType("LobbyData", 0, asOBJ_REF);
    as_engine->RegisterObjectBehaviour("LobbyData", asBEHAVE_ADDREF, "void f()",
                                       asMETHOD(LobbyAS, AddRef), asCALL_THISCALL);
    as_engine->RegisterObjectBehaviour("LobbyData", asBEHAVE_RELEASE, "void f()",
                                       asMETHOD(LobbyAS, Release), asCALL_THISCALL);
    // LobbyPeer
    as_engine->RegisterObjectType("LobbyPeer", 0, asOBJ_REF);
    as_engine->RegisterObjectBehaviour("LobbyPeer", asBEHAVE_ADDREF, "void f()",
                                       asMETHOD(LobbyAS, AddRef), asCALL_THISCALL);
    as_engine->RegisterObjectBehaviour("LobbyPeer", asBEHAVE_RELEASE, "void f()",
                                       asMETHOD(LobbyAS, Release), asCALL_THISCALL);

    // Register getters
    as_engine->RegisterObjectMethod("Lobby", "string get_calling_peer_id() const property",
                                    asMETHOD(LobbyAS, get_calling_peer_id), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "string get_id() const property",
                                    asMETHOD(LobbyAS, get_lobby_id), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "int64 get_tick_rate() const property",
                                    asMETHOD(LobbyAS, get_tick_rate), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "string get_name() const property",
                                    asMETHOD(LobbyAS, get_name), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "void set_name(string name)",
                                    asMETHOD(LobbyAS, set_name), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "string get_host() const property",
                                    asMETHOD(LobbyAS, get_host), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "void set_host(string host)",
                                    asMETHOD(LobbyAS, set_host), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "int64 get_max_players() const property",
                                    asMETHOD(LobbyAS, get_max_players), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "void set_max_players(int64 max_players)",
                                    asMETHOD(LobbyAS, set_max_players), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "int64 get_create_time() const property",
                                    asMETHOD(LobbyAS, get_create_time), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "bool get_sealed() const property",
                                    asMETHOD(LobbyAS, is_sealed), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "void set_sealed(bool sealed)",
                                    asMETHOD(LobbyAS, set_sealed), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "string get_game_id() const property",
                                    asMETHOD(LobbyAS, get_game_id), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "int64 get_peers_count() const property",
                                    asMETHOD(LobbyAS, get_peers_count), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "LobbyData@ get_public_data() const property",
                                    asMETHOD(LobbyAS, get_public_data), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "LobbyData@ get_private_data() const property",
                                    asMETHOD(LobbyAS, get_private_data), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "LobbyData@ get_tags() const property",
                                    asMETHOD(LobbyAS, get_tags), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "LobbyData@ get_user_data() const property",
                                    asMETHOD(LobbyAS, get_user_data), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "LobbyData@ get_public_data() const property",
                                    asMETHOD(LobbyAS, get_public_data), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "LobbyData@ get_private_data() const property",
                                    asMETHOD(LobbyAS, get_private_data), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "string get_id() const property",
                                    asMETHOD(LobbyAS, get_peer_id), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "string get_platform() const property",
                                    asMETHOD(LobbyAS, get_peer_platform), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "string get_platform_id() const property",
                                    asMETHOD(LobbyAS, get_peer_platform_id), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "int get_order_id() const property",
                                    asMETHOD(LobbyAS, get_peer_order_id), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "bool get_ready() const property",
                                    asMETHOD(LobbyAS, get_peer_ready), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyPeer", "bool get_disconnected() const property",
                                    asMETHOD(LobbyAS, get_peer_disconnected), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("Lobby", "dictionary@ get_peers() const property",
                                    asMETHOD(LobbyAS, get_peers), asCALL_THISCALL);
    // Set
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, any@ value)",
                                    asMETHOD(LobbyAS, set), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, int64 value)",
                                    asMETHOD(LobbyAS, setInt64), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, bool value)",
                                    asMETHOD(LobbyAS, setBool), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, double value)",
                                    asMETHOD(LobbyAS, setDouble), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, string value)",
                                    asMETHOD(LobbyAS, setString), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, array<any> &value)",
                                    asMETHOD(LobbyAS, setArray), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "void set(string key, dictionary &value)",
                                    asMETHOD(LobbyAS, setDictionary), asCALL_THISCALL);
    // Get
    as_engine->RegisterObjectMethod("LobbyData", "any@ get(string key)", asMETHOD(LobbyAS, get),
                                    asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "int64 get_int(string key)",
                                    asMETHOD(LobbyAS, getInt64), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "bool get_bool(string key)",
                                    asMETHOD(LobbyAS, getBool), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "double get_double(string key)",
                                    asMETHOD(LobbyAS, getDouble), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "array<any>@ get_array(string key)",
                                    asMETHOD(LobbyAS, getArray), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "dictionary@ get_dictionary(string key)",
                                    asMETHOD(LobbyAS, getDictionary), asCALL_THISCALL);
    as_engine->RegisterObjectMethod("LobbyData", "string get_string(string key)",
                                    asMETHOD(LobbyAS, getString), asCALL_THISCALL);

    // setters
    as_engine->RegisterObjectMethod("LobbyData", "void opIndexAssign(const any@, const string &in)",
                                    asMETHOD(LobbyAS, opIndexAssign_any), asCALL_THISCALL);
    // getter
    as_engine->RegisterObjectMethod("LobbyData", "any@ opIndex(const string &in) const",
                                    asMETHOD(LobbyAS, opIndex_any), asCALL_THISCALL);
    as_engine->SetDefaultNamespace("lobby");
    as_engine->RegisterGlobalFunction(
        "void start_timer(string timer_id, double duration, array<any>@ args = array<any>())",
        asFUNCTION(as_start_timer_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void stop_timer(string timer_id)",
                                      asFUNCTION(as_stop_timer_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void notify(string peer_id, any@ message)",
                                      asFUNCTION(as_notifty_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void broadcast_chat(string message)",
                                      asFUNCTION(as_broadcast_chat_wrapper), asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void broadcast_chat(string message, dictionary@ metadata)",
                                      asFUNCTION(as_broadcast_chat_wrapper_with_metadata),
                                      asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("int64 get_ticks_ms()", asFUNCTION(as_get_ticks_ms_wrapper),
                                      asCALL_GENERIC);
    as_engine->RegisterGlobalFunction("void kick_peer(string peer_id)",
                                      asFUNCTION(as_kick_peer_wrapper), asCALL_GENERIC);
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
    std::vector<std::string> expected_functions = get_expected_functions();
    for (int i = 0; i < as_module->GetFunctionCount(); i++) {
        asIScriptFunction *func = as_module->GetFunctionByIndex(i);
        as_functions.emplace(std::string(func->GetName()), i);
    }
    for (const auto &func_name : expected_functions) {
        if (as_functions.find(func_name) != as_functions.end()) {
            empty_set.insert(func_name);
        }
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

void ScriptAS::as_start_timer(std::string &timer_id, double duration, CScriptArray *arg) {
    if (duration < 0.1 || duration > 600) {
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
                                         .end_time = int(duration * 1000) + game_thread->get_time(),
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

void ScriptAS::as_broadcast_chat(std::string &message, CScriptDictionary *metadata_dict) {
    boost::container::flat_map<std::string, AnyElement> chat_metadata;
    if (metadata_dict) {
        chat_metadata = ConvertFromDictionary(as_engine, metadata_dict);
    }
    // Call the actual function
    auto &game = game_thread->games[as_game_id];
    game_thread->send_message(game, as_lobby_id, message, chat_metadata);
}

void ScriptAS::as_kick_peer(std::string &peer_id) {
    auto &game = game_thread->games[as_game_id];
    game_thread->kick_peer(game, as_lobby_id, peer_id);
}

AnyElement ScriptAS::func_call(std::string &func_name, boost::container::vector<AnyElement> &args,
                               std::string &peer_id, std::string &lobby_id, std::string &game_id,
                               bool &has_error) {
    has_error = false;
    error_msg = "";
    as_peer_id = peer_id;
    as_lobby_id = lobby_id;
    as_game_id = game_id;
    if (!enabled) {
        return AnyElement{"AngelScript not enabled"};
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
    int string_type_id = as_engine->GetTypeIdByDecl("string");
    int dict_type_id = as_engine->GetTypeIdByDecl("dictionary");
    int array_type_id = as_engine->GetTypeIdByDecl("array<any>");
    // Set arguments
    for (size_t i = 0; i < args.size(); i++) {
        int typeId;
        int r = func->GetParam(i, &typeId);
        if (r < 0) {
            has_error = true;
            return AnyElement{"Failed to get param"};
        }
        const auto &value = args[i].value;
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

std::string LobbyAS::get_lobby_id() { return as_lobby_id; }
std::string LobbyAS::get_game_id() { return as_game_id; }
std::string LobbyAS::get_calling_peer_id() { return as_calling_peer_id; }
int64_t LobbyAS::get_tick_rate() {
    auto &game = game_thread->games[as_game_id];
    return game.tick_rate;
}
std::string LobbyAS::get_name() {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    return lobby.name;
}
void LobbyAS::set_name(std::string &name) {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    lobby.name = name;
    lobby.name_dirty = true;
}
std::string LobbyAS::get_host() {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    return lobby.host;
}
void LobbyAS::set_host(std::string &host) {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    lobby.host = host;
}
int64_t LobbyAS::get_max_players() {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    return lobby.max_players;
}

void LobbyAS::set_max_players(int64_t max_players) {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    lobby.max_players = max_players;
    lobby.max_players_dirty = true;
}

int64_t LobbyAS::get_create_time() {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    return lobby.create_time;
}

bool LobbyAS::is_sealed() {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    return lobby.sealed;
}

void LobbyAS::set_sealed(bool sealed) {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    lobby.sealed = sealed;
}

LobbyAS *LobbyAS::get_public_data() {
    switch (type) {
        case LobbyType::LOBBY_ROOT:
            return new LobbyAS{
                .type = LobbyType::LOBBY_PUBLIC_DATA,
                .as_engine = as_engine,
                .as_context = as_context,
                .as_module = as_module,
                .game_thread = game_thread,
                .as_game_id = as_game_id,
                .as_lobby_id = as_lobby_id,
                .as_peer_id = as_peer_id,
                .as_calling_peer_id = as_calling_peer_id,
            };
        case LobbyType::PEER_ROOT:
            return new LobbyAS{
                .type = LobbyType::PEER_PUBLIC_DATA,
                .as_engine = as_engine,
                .as_context = as_context,
                .as_module = as_module,
                .game_thread = game_thread,
                .as_game_id = as_game_id,
                .as_lobby_id = as_lobby_id,
                .as_peer_id = as_peer_id,
                .as_calling_peer_id = as_calling_peer_id,
            };
        default:
            if (as_context) {
                as_context->SetException("Cannot get public_data from this object type");
            }
            return nullptr;
    }
}

LobbyAS *LobbyAS::get_private_data() {
    switch (type) {
        case LobbyType::LOBBY_ROOT:
            return new LobbyAS{
                .type = LobbyType::LOBBY_PRIVATE_DATA,
                .as_engine = as_engine,
                .as_context = as_context,
                .as_module = as_module,
                .game_thread = game_thread,
                .as_game_id = as_game_id,
                .as_lobby_id = as_lobby_id,
                .as_peer_id = as_peer_id,
                .as_calling_peer_id = as_calling_peer_id,
            };
        case LobbyType::PEER_ROOT:
            return new LobbyAS{
                .type = LobbyType::PEER_PRIVATE_DATA,
                .as_engine = as_engine,
                .as_context = as_context,
                .as_module = as_module,
                .game_thread = game_thread,
                .as_game_id = as_game_id,
                .as_lobby_id = as_lobby_id,
                .as_peer_id = as_peer_id,
                .as_calling_peer_id = as_calling_peer_id,
            };
        default:
            if (as_context) {
                as_context->SetException("Cannot get public_private from this object type");
            }
            return nullptr;
    }
}

LobbyAS *LobbyAS::get_tags() {
    return new LobbyAS{
        .type = LobbyType::LOBBY_TAGS,
        .as_engine = as_engine,
        .as_context = as_context,
        .as_module = as_module,
        .game_thread = game_thread,
        .as_game_id = as_game_id,
        .as_lobby_id = as_lobby_id,
        .as_peer_id = as_peer_id,
        .as_calling_peer_id = as_calling_peer_id,
    };
}

LobbyAS *LobbyAS::get_user_data() {
    return new LobbyAS{
        .type = LobbyType::PEER_USER_DATA,
        .as_engine = as_engine,
        .as_context = as_context,
        .as_module = as_module,
        .game_thread = game_thread,
        .as_game_id = as_game_id,
        .as_lobby_id = as_lobby_id,
        .as_peer_id = as_peer_id,
        .as_calling_peer_id = as_calling_peer_id,
    };
}

int64_t LobbyAS::get_peers_count() {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    return lobby.peer_ids.size();
}

CScriptDictionary *LobbyAS::get_peers() {
    CScriptDictionary *dict = CScriptDictionary::Create(as_engine);
    int peer_type_id = as_engine->GetTypeIdByDecl("LobbyPeer@");
    assert(peer_type_id >= 0);
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    for (const auto &peer_id : lobby.peer_ids) {
        LobbyAS *peer_obj = new LobbyAS{
            .type = LobbyType::PEER_ROOT,
            .as_engine = as_engine,
            .as_context = as_context,
            .as_module = as_module,
            .game_thread = game_thread,
            .as_game_id = as_game_id,
            .as_lobby_id = as_lobby_id,
            .as_peer_id = peer_id,
            .as_calling_peer_id = as_calling_peer_id,
        };
        dict->Set(peer_id, &peer_obj, peer_type_id);
    }
    return dict;
}

AnyElement LobbyAS::retrieve(const std::string &key) {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    switch (type) {
        case LobbyType::LOBBY_PUBLIC_DATA:
            return lobby.public_data[key];
        case LobbyType::LOBBY_PRIVATE_DATA:
            return lobby.private_data[key];
        case LobbyType::LOBBY_TAGS:
            return lobby.tags[key];
        case LobbyType::PEER_PUBLIC_DATA: {
            auto &peer = game.peers[as_peer_id];
            return peer.public_data[key];
        }
        case LobbyType::PEER_PRIVATE_DATA: {
            auto &peer = game.peers[as_peer_id];
            return peer.private_data[key];
        }
        case LobbyType::PEER_USER_DATA: {
            auto &peer = game.peers[as_peer_id];
            return peer.user_data[key];
        }
        default:
            return AnyElement{std::monostate{}};
    }
}

CScriptAny *LobbyAS::get(const std::string &key) {
    AnyElement any_val = retrieve(key);
    return ConvertToAny(as_engine, any_val);
}

int64_t LobbyAS::getInt64(const std::string &key) {
    AnyElement any_val = retrieve(key);
    if (auto val = std::get_if<int64_t>(&any_val.value)) {
        return *val;
    }
    return 0;
}

bool LobbyAS::getBool(const std::string &key) {
    AnyElement any_val = retrieve(key);
    if (auto val = std::get_if<bool>(&any_val.value)) {
        return *val;
    }
    return false;
}

std::string LobbyAS::getString(const std::string &key) {
    AnyElement any_val = retrieve(key);
    if (auto val = std::get_if<std::string>(&any_val.value)) {
        return *val;
    }
    return "";
}

double LobbyAS::getDouble(const std::string &key) {
    AnyElement any_val = retrieve(key);
    if (auto val = std::get_if<double>(&any_val.value)) {
        return *val;
    }
    return 0.0;
}

CScriptDictionary *LobbyAS::getDictionary(const std::string &key) {
    AnyElement any_val = retrieve(key);
    if (auto val =
            std::get_if<boost::container::flat_map<std::string, AnyElement>>(&any_val.value)) {
        return ConvertToDictionary(as_engine, *val);
    }
    return CScriptDictionary::Create(as_engine);
}

CScriptArray *LobbyAS::getArray(const std::string &key) {
    AnyElement any_val = retrieve(key);
    int any_type_id = as_engine->GetTypeIdByDecl("any@");
    if (auto val = std::get_if<boost::container::vector<AnyElement>>(&any_val.value)) {
        return ConvertToArray(as_engine, *val);
    }
    asITypeInfo *type = as_engine->GetTypeInfoByDecl("array<any>");
    CScriptArray *arr = CScriptArray::Create(type);
    return arr;
}

void LobbyAS::assign(const std::string &key, AnyElement &value) {
    auto &game = game_thread->games[as_game_id];
    auto &lobby = game.lobbies[as_lobby_id];
    switch (type) {
        case LobbyType::LOBBY_PUBLIC_DATA:
            lobby.public_data[key] = value;
            lobby.public_data_dirty = true;
            break;
        case LobbyType::LOBBY_PRIVATE_DATA:
            lobby.private_data[key] = value;
            lobby.private_data_dirty = true;
            break;
        case LobbyType::LOBBY_TAGS:
            lobby.tags[key] = value;
            lobby.tags_dirty = true;
            break;
        case LobbyType::PEER_PUBLIC_DATA: {
            auto &peer = game.peers[as_peer_id];
            peer.public_data[key] = value;
            peer.public_data_dirty = true;
        } break;
        case LobbyType::PEER_PRIVATE_DATA: {
            auto &peer = game.peers[as_peer_id];
            peer.private_data[key] = value;
            peer.private_data_dirty = true;
        } break;
        case LobbyType::PEER_USER_DATA: {
            auto &peer = game.peers[as_peer_id];
            peer.user_data[key] = value;
            peer.user_data_dirty = true;
        } break;
        default:
            break;
    }
}

void LobbyAS::set(std::string &key, CScriptAny *value) {
    AnyElement value_any = ConvertFromAny(as_engine, value);
    value->Release();
    assign(key, value_any);
}

void LobbyAS::setInt64(std::string &key, int64_t value) {
    AnyElement value_any{value};
    assign(key, value_any);
}

void LobbyAS::setBool(std::string &key, bool value) {
    AnyElement value_any{value};
    assign(key, value_any);
}

void LobbyAS::setString(std::string &key, std::string &value) {
    AnyElement value_any{value};
    assign(key, value_any);
}

void LobbyAS::setDouble(std::string &key, double value) {
    AnyElement value_any{value};
    assign(key, value_any);
}

void LobbyAS::setDictionary(std::string &key, CScriptDictionary *value) {
    AnyElement value_any{ConvertFromDictionary(as_engine, value)};
    assign(key, value_any);
}

void LobbyAS::setArray(std::string &key, CScriptArray *value) {
    AnyElement value_any{ConvertFromArray(as_engine, value)};
    assign(key, value_any);
}

std::string LobbyAS::get_peer_id() { return as_peer_id; }
std::string LobbyAS::get_peer_platform() {
    auto &game = game_thread->games[as_game_id];
    auto &peer = game.peers[as_peer_id];
    return peer.platform;
}
std::string LobbyAS::get_peer_platform_id() {
    auto &game = game_thread->games[as_game_id];
    auto &peer = game.peers[as_peer_id];
    return peer.platform_id;
}
int64_t LobbyAS::get_peer_order_id() {
    auto &game = game_thread->games[as_game_id];
    auto &peer = game.peers[as_peer_id];
    return peer.order_id;
}
bool LobbyAS::get_peer_ready() {
    auto &game = game_thread->games[as_game_id];
    auto &peer = game.peers[as_peer_id];
    return peer.ready;
}
bool LobbyAS::get_peer_disconnected() {
    auto &game = game_thread->games[as_game_id];
    auto &peer = game.peers[as_peer_id];
    return peer.disconnected;
}

// Setters
void LobbyAS::opIndexAssign_any(CScriptAny *value, const std::string &key) {
    AnyElement any_val = ConvertFromAny(as_engine, value);
    value->Release();
    assign(key, any_val);
}

// Getters
CScriptAny *LobbyAS::opIndex_any(const std::string &key) {
    auto any_val = retrieve(key);
    return ConvertToAny(as_engine, any_val);
}
