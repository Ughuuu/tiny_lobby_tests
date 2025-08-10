#pragma once
#include <boost/container/flat_map.hpp>
#include <string>

#include "../common/any_type.h"

#define ERROR_CANNOT_PARSE_JSON "ERROR_CANNOT_PARSE_JSON"
#define ERROR_PEER_NOT_FOUND "ERROR_PEER_NOT_FOUND"
#define ERROR_PEER_ALREADY_EXISTS "ERROR_PEER_ALREADY_EXISTS"
#define ERROR_PEER_NOT_IN_A_LOBBY "ERROR_PEER_NOT_IN_A_LOBBY"
#define ERROR_PEER_IS_IN_A_LOBBY "ERROR_PEER_IS_IN_A_LOBBY"
#define ERROR_PEER_NOT_HOST "ERROR_PEER_NOT_HOST"
#define ERROR_PEER_IS_READY "ERROR_PEER_IS_READY"
#define ERROR_PEER_IS_NOT_READY "ERROR_PEER_IS_NOT_READY"
#define ERROR_CANNOT_KICK_SELF "ERROR_CANNOT_KICK_SELF"
#define ERROR_INVALID_FUNCTION_MSG "ERROR_INVALID_FUNCTION_MSG"
#define ERROR_INVALID_ARGUMENTS "ERROR_INVALID_ARGUMENTS"
#define ERROR_LOBBY_NOT_FOUND "ERROR_LOBBY_NOT_FOUND"
#define ERROR_LOBBY_WRONG_PASSWORD "ERROR_LOBBY_WRONG_PASSWORD"
#define ERROR_LOBBY_FULL "ERROR_LOBBY_FULL"
#define ERROR_GAME_NOT_FOUND "ERROR_GAME_NOT_FOUND"
#define ERROR_GAME_NOT_RELAY "ERROR_GAME_NOT_RELAY"
#define ERROR_GAME_NOT_SCRIPTED "ERROR_GAME_NOT_SCRIPTED"
#define ERROR_UNKOWN_COMMAND "ERROR_UNKOWN_COMMAND"
#define ERROR_GAME_IS_UNLOADED "ERROR_GAME_IS_UNLOADED"
#define ERROR_INVALID_TAGS "ERROR_INVALID_TAGS"

// c == COMMAND
// d = DATA
// e = IS_LOGICAL_ERROR
// m = MESSAGE

const int RESPONSE_ERROR = 0;
const int RESPONSE_LOBBY_HOSTED = 1;
const int RESPONSE_LOBBY_CREATED = 2;
const int RESPONSE_LOBBY_UNSEALED = 3;
const int RESPONSE_LOBBY_SEALED = 4;
const int RESPONSE_LOBBY_RESIZED = 5;
const int RESPONSE_LOBBY_PASSWORDED = 6;
const int RESPONSE_LOBBY_TITLED = 7;
const int RESPONSE_PEER_READY = 8;
const int RESPONSE_PEER_UNREADY = 9;
const int RESPONSE_LOBBY_LEFT = 10;
const int RESPONSE_PEER_LEFT = 11;
const int RESPONSE_LOBBY_KICKED = 12;
const int RESPONSE_PEER_STATE = 13;
const int RESPONSE_LOBBY_CALL = 14;
const int RESPONSE_PEER_USER_DATA = 15;
const int RESPONSE_LOBBY_TAGS = 16;
const int RESPONSE_PEER_CHAT = 17;
const int RESPONSE_PEER_RECONNECTED = 18;
const int RESPONSE_JOINED_LOBBY = 19;
const int RESPONSE_PEER_JOINED = 20;
const int RESPONSE_PEER_DISCONNECTED = 21;
const int RESPONSE_LOBBY_LIST = 22;
const int RESPONSE_LOBBY_DATA = 23;
const int RESPONSE_PEER_NOTIFY = 24;
const int RESPONSE_DATA_TO = 25;
const int RESPONSE_DATA_TO_SENT = 26;
const int RESPONSE_NOTIFY_TO_SENT = 27;

const int COMMAND_LOBBY_DATA = 0;
const int COMMAND_LOBBY_DATA_TO = 1;
const int COMMAND_LOBBY_DATA_TO_ALL = 2;
const int COMMAND_LOBBY_NOTIFY_TO = 3;
const int COMMAND_LOBBY_NOTIFY = 4;
const int COMMAND_LOBBY_CALL = 5;
const int COMMAND_LOBBY_QUICK_JOIN = 6;
const int COMMAND_CREATE_LOBBY = 7;
const int COMMAND_JOIN_LOBBY = 8;
const int COMMAND_LEAVE_LOBBY = 9;
const int COMMAND_LIST_LOBBY = 10;
const int COMMAND_CHAT_LOBBY = 11;
const int COMMAND_LOBBY_TAGS = 12;
const int COMMAND_KICK_PEER = 13;
const int COMMAND_USER_DATA = 14;
const int COMMAND_LOBBY_READY = 15;
const int COMMAND_LOBBY_UNREADY = 16;
const int COMMAND_LOBBY_SEAL = 17;
const int COMMAND_LOBBY_UNSEAL = 18;
const int COMMAND_LOBBY_MAX_PLAYERS = 19;
const int COMMAND_LOBBY_TITLE = 20;
const int COMMAND_LOBBY_PASSWORD = 21;
const int COMMAND_STOP_LISTING = 22;

// host_id = h
// max_players = m
// name = n
// tags = t
// lobby = l
// peers = p

std::string notification_error(const std::string& message, const std::string& command_id = "",
                               bool is_logical_error = false) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_ERROR}},
                          {"m", AnyElement{message}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}},
                          {"e", AnyElement{is_logical_error}}}}
        .to_string();
}
std::string notification_lobby_host_changed(const std::string& host_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_HOSTED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"h", AnyElement{host_id}}}}}}}
        .to_string();
}
std::string notification_lobby_created(const AnyElement& lobby, const AnyElement& peers,
                                       const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_CREATED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"l", lobby}, {"p", peers}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_unsealed(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_UNSEALED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_sealed(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_SEALED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_max_players(int max_players, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{RESPONSE_LOBBY_RESIZED}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"m", AnyElement{max_players}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_password_protected(bool password_protected,
                                                  const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{RESPONSE_LOBBY_PASSWORDED}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"_p", AnyElement{password_protected}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_title(std::string title, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_TITLED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"n", AnyElement{title}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_ready(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_READY}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_unready(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_UNREADY}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_left(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_LEFT}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_left(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_LEFT}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_kicked(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_LEFT}},  // kept same as original logic
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", AnyElement{peer_id}},
                                    {"k", AnyElement{true}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_kicked() {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_KICKED}}}}
        .to_string();
}

std::string notification_peer_state(const AnyElement& peer_json) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_STATE}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", peer_json}}}}}}
        .to_string();
}

std::string notification_lobby_call(const AnyElement& result, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_CALL}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"r", result}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_user_data(const AnyElement& user_data, const std::string& peer_id,
                                   const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{RESPONSE_PEER_USER_DATA}},
            {"d",
             AnyElement{boost::container::flat_map<std::string, AnyElement>{
                 {"ud", user_data}, {"p", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_tags(const AnyElement& tags, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_TAGS}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"t", tags}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_chat(
    const std::string& from_peer, const std::string& chat_data, const std::string& command_id,
    const boost::container::flat_map<std::string, AnyElement>& chat_metadata) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_CHAT}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"fp", AnyElement{from_peer}},
                                    {"c", AnyElement{chat_data}},
                                    {"m", AnyElement{chat_metadata}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_reconnected(const std::string& peer_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_RECONNECTED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", AnyElement{peer_id}}}}}}}
        .to_string();
}

std::string notification_lobby_joined(const AnyElement& lobby, const AnyElement& peers,
                                      const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_JOINED_LOBBY}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"l", lobby}, {"p", peers}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_joined(const AnyElement& peer_json) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_JOINED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", peer_json}}}}}}
        .to_string();
}

std::string notification_peer_disconnected(const std::string& peer_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_DISCONNECTED}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"p", AnyElement{peer_id}}}}}}}
        .to_string();
}

std::string notification_lobby_list(const AnyElement& lobbies, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_LIST}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"l", lobbies}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_stop_listing(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_LOBBY_LIST}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"l", AnyElement{}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_public_data(const AnyElement& lobby_data,
                                           const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{RESPONSE_LOBBY_DATA}},
            {"d",
             AnyElement{boost::container::flat_map<std::string, AnyElement>{
                 {"d", lobby_data}, {"_p", AnyElement{false}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_private_data(const AnyElement& lobby_data,
                                            const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{RESPONSE_LOBBY_DATA}},
            {"d",
             AnyElement{boost::container::flat_map<std::string, AnyElement>{
                 {"d", lobby_data}, {"_p", AnyElement{true}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_notify(const AnyElement& peer_data, const std::string& from_peer) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_PEER_NOTIFY}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"d", peer_data}, {"p", AnyElement{from_peer}}}}}}}
        .to_string();
}

std::string notification_peer_public_data(const AnyElement& peer_data, const std::string& from_peer,
                                          const std::string& target_peer) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_DATA_TO}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"d", peer_data},
                                    {"p", AnyElement{from_peer}},
                                    {"tp", AnyElement{target_peer}},
                                    {"_p", AnyElement{false}}}}}}}
        .to_string();
}

std::string notification_peer_private_data(const AnyElement& peer_data,
                                           const std::string& from_peer,
                                           const std::string& target_peer) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_DATA_TO}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"d", AnyElement{peer_data}},
                                    {"p", AnyElement{from_peer}},
                                    {"tp", AnyElement{target_peer}},
                                    {"_p", AnyElement{true}}}}}}}
        .to_string();
}

std::string notification_data_to_sent(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_DATA_TO_SENT}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_notify_sent(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{RESPONSE_NOTIFY_TO_SENT}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
