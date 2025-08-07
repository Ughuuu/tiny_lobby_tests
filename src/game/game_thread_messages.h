#pragma once
#include <boost/container/flat_map.hpp>
#include <string>

#include "../common/any_type.h"

std::string ERROR_CANNOT_PARSE_JSON = "Cannot parse json";
std::string ERROR_PEER_NOT_FOUND = "Peer not found";
std::string ERROR_PEER_ALREADY_EXISTS = "Peer already exists";
std::string ERROR_PEER_NOT_IN_A_LOBBY = "Peer not in a lobby";
std::string ERROR_CANNOT_KICK_SELF = "Cannot kick self";
std::string ERROR_INVALID_FUNCTION_MSG = "Invalid function";
std::string ERROR_INVALID_ARGUMENTS = "Invalid arguments";
std::string ERROR_PEER_IS_IN_A_LOBBY = "Peer is in a lobby";
std::string ERROR_PEER_NOT_HOST = "Peer is not the host";
std::string ERROR_PEER_IS_READY = "Peer is ready";
std::string ERROR_PEER_IS_NOT_READY = "Peer is not ready";
std::string ERROR_LOBBY_NOT_SEALED = "Lobby is not sealed";
std::string ERROR_LOBBY_SEALED = "Lobby is sealed";
std::string ERROR_LOBBY_NOT_FOUND = "Lobby not found";
std::string ERROR_GAME_NOT_FOUND = "Game not found";
std::string ERROR_GAME_NOT_RELAY = "Game is not in relay mode";
std::string ERROR_UNKOWN_COMMAND = "Unkown command";

// c == COMMAND
// d = DATA
// e = IS_LOGICAL_ERROR
// m = MESSAGE
// COMMAND_IDS
// 0 = error
// 1 = lobby_hosted
// 2 = lobby_created
// 3 = lobby_unsealed
// 4 = lobby_sealed
// 5 = lobby_resized
// 6 = lobby_passworded
// 7 = lobby_titled
// 8 = peer_ready
// 9 = peer_unready
// 10 = lobby_left
// 11 = peer_left
// 12 = lobby_kicked
// 13 = peer_state
// 14 = lobby_call
// 15 = peer_user_data
// 16 = lobby_tags
// 17 = peer_chat
// 18 = peer_reconnected
// 19 = joined_lobby
// 20 = peer_joined
// 21 = peer_disconnected
// 22 = lobby_list
// 23 = lobby_data
// 24 = peer_notify
// 25 = data_to
// 26 = data_to_sent
// 27 = notify_to_sent

// host_id = h
// max_players = m

std::string notification_error(const std::string& message, const std::string& command_id = "",
                               bool is_logical_error = false) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{0}},
                          {"m", AnyElement{message}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}},
                          {"e", AnyElement{is_logical_error}}}}
        .to_string();
}
std::string notification_lobby_host_changed(const std::string& host_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{1}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"h", AnyElement{host_id}}}}}}}
        .to_string();
}
std::string notification_lobby_created(const AnyElement& lobby, const AnyElement& peers,
                                       const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{2}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"lobby", lobby}, {"peers", peers}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_unsealed(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{3}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_sealed(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{4}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_max_players(int max_players, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{5}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"m", AnyElement{max_players}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_password_protected(bool password_protected,
                                                  const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{6}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"password_protected", AnyElement{password_protected}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_title(std::string title, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{7}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"lobby_title", AnyElement{title}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_ready(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{8}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"peer_id", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_unready(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{9}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"peer_id", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_left(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{10}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_left(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{11}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"peer_id", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_kicked(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{11}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer_id", AnyElement{peer_id}},
                                    {"kicked", AnyElement{true}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_kicked() {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{{"c", AnyElement{12}}}}
        .to_string();
}

std::string notification_peer_state(const AnyElement& peer_json) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{13}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer", peer_json}}}}}}
        .to_string();
}

std::string notification_lobby_call(const AnyElement& result, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{14}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"result", result}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_user_data(const AnyElement& user_data, const std::string& peer_id,
                                   const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{15}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"user_data", user_data},
                                    {"peer_id", AnyElement{peer_id}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_tags(const AnyElement& tags, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{16}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"tags", tags}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_chat(
    const std::string& from_peer, const std::string& chat_data, const std::string& command_id,
    const boost::container::flat_map<std::string, AnyElement>& chat_metadata) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{17}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"from_peer", AnyElement{from_peer}},
                                    {"chat_data", AnyElement{chat_data}},
                                    {"chat_metadata", AnyElement{chat_metadata}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_reconnected(const std::string& peer_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{18}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer_id", AnyElement{peer_id}}}}}}}
        .to_string();
}

std::string notification_lobby_joined(const AnyElement& lobby, const AnyElement& peers,
                                      const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{19}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"lobby", lobby}, {"peers", peers}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_joined(const AnyElement& peer_json) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{20}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer", peer_json}}}}}}
        .to_string();
}

std::string notification_peer_disconnected(const std::string& peer_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{21}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer_id", AnyElement{peer_id}}}}}}}
        .to_string();
}

std::string notification_lobby_list(const AnyElement& lobbies, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{22}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"lobbies", lobbies}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_public_data(const AnyElement& lobby_data,
                                           const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{23}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"lobby_data", lobby_data},
                                    {"is_private", AnyElement{false}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_private_data(const AnyElement& lobby_data,
                                            const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{23}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"lobby_data", lobby_data},
                                    {"is_private", AnyElement{true}},
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_notify(const AnyElement& peer_data, const std::string& from_peer) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"c", AnyElement{24}},
            {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                      {"peer_data", peer_data}, {"from_peer", AnyElement{from_peer}}}}}}}
        .to_string();
}

std::string notification_peer_public_data(const AnyElement& peer_data, const std::string& from_peer,
                                          const std::string& target_peer) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{25}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer_data", peer_data},
                                    {"from_peer", AnyElement{from_peer}},
                                    {"target_peer", AnyElement{target_peer}},
                                    {"is_private", AnyElement{false}}}}}}}
        .to_string();
}

std::string notification_peer_private_data(const AnyElement& peer_data,
                                           const std::string& from_peer,
                                           const std::string& target_peer) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{25}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"peer_data", AnyElement{peer_data}},
                                    {"from_peer", AnyElement{from_peer}},
                                    {"target_peer", AnyElement{target_peer}},
                                    {"is_private", AnyElement{true}}}}}}}
        .to_string();
}

std::string notification_data_to_sent(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{26}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_notify_sent(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"c", AnyElement{27}},
                          {"d", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                    {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
