#pragma once
#include <angelscript.h>

#include <string>
#include <vector>

#include "any_type.h"
#include "server_logger.h"

class GameThread;
class CScriptDictionary;
class CScriptArray;
class CScriptAny;
class DataHelperAS;

struct LobbyAS {
    enum LobbyType {
        LOBBY_ROOT,
        LOBBY_PUBLIC_DATA,
        LOBBY_PRIVATE_DATA,
        LOBBY_TAGS,
        PEER_ROOT,
        PEER_PUBLIC_DATA,
        PEER_PRIVATE_DATA,
        PEER_USER_DATA,
    } type;
    asIScriptEngine* as_engine;
    asIScriptContext* as_context;
    asIScriptModule* as_module;
    GameThread* game_thread;
    std::string as_game_id;
    std::string as_lobby_id;
    std::string as_peer_id;
    std::string as_calling_peer_id;
    int ref_count = 1;

    void AddRef() { ++ref_count; }
    void Release() {
        if (--ref_count == 0) {
            delete this;
        }
    }

    std::string get_lobby_id();
    std::string get_game_id();
    std::string get_calling_peer_id();
    int64_t get_tick_rate();
    std::string get_name();
    std::string get_host();
    int64_t get_max_players();
    int64_t get_create_time();
    bool is_sealed();
    void set_sealed(bool sealed);
    LobbyAS* get_public_data();
    LobbyAS* get_private_data();
    LobbyAS* get_tags();
    CScriptDictionary* get_peers();
    int64_t get_peers_count();
    CScriptAny* get(const std::string& key);
    int64_t getInt64(const std::string& key);
    bool getBool(const std::string& key);
    double getDouble(const std::string& key);
    CScriptDictionary* getDictionary(const std::string& key);
    std::string getString(const std::string& key);
    CScriptArray* getArray(const std::string& key);
    void set(std::string& key, CScriptAny* value);
    void setInt64(std::string& key, int64_t value);
    void setBool(std::string& key, bool value);
    void setDouble(std::string& key, double value);
    void setString(std::string& key, std::string& value);
    void setDictionary(std::string& key, CScriptDictionary* value);
    void setArray(std::string& key, CScriptArray* value);
    LobbyAS* get_user_data();

    std::string get_peer_id();
    int64_t get_peer_order_id();
    bool get_peer_ready();
    bool get_peer_disconnected();

    AnyElement retrieve(const std::string& key);
    void assign(const std::string& key, AnyElement& value);
    void opIndexAssign_any(CScriptAny* value, const std::string& key);
    CScriptAny* opIndex_any(const std::string& key);
};

struct ScriptAS {
    bool autoreload;
    std::string scripts_folder;
    std::string folder_name;
    std::string script_entrypoint;
    std::string logs_folder;
    GameThread* game_thread;

    asIScriptEngine* as_engine = nullptr;
    asIScriptContext* as_context = nullptr;
    asIScriptModule* as_module = nullptr;
    boost::container::flat_map<std::string, int> as_functions{};
    std::string as_peer_id = "";
    std::string as_lobby_id = "";
    std::string as_game_id = "";
    std::string error_msg = "";

   public:
    bool enabled = false;
    AnyElement func_call(std::string& func_name, boost::container::vector<AnyElement>& args,
                         std::string& peer_id, std::string& lobby_id, std::string& game_id,
                         bool& has_error);
    boost::container::flat_set<std::string> open();
    void as_start_timer(std::string& timer_id, int duration, CScriptArray* arg);
    void as_stop_timer(std::string& timer_id);
    void as_notifty(std::string& peer_id, CScriptAny* message);
    void as_broadcast_chat(std::string& message);
    void close();
    void as_MessageCallback(const asSMessageInfo* msg, void* param);
};
