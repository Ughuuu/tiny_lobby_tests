#pragma once
#include <boost/container/flat_map.hpp>
#include <string>

#include "any_type.h"

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

std::string notification_error(const std::string& message, const std::string& command_id = "",
                               bool is_logical_error = false) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"error"}},
                          {"message", AnyElement{message}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"id", AnyElement{command_id}}}}},
                          {"is_logical_error", AnyElement{is_logical_error}}}}
        .to_string();
}
std::string notification_lobby_host_changed(const std::string& host_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_hosted"}},
                          {"message", AnyElement{"Lobby hosted"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"host_id", AnyElement{host_id}}}}}}}
        .to_string();
}
std::string notification_lobby_created(const AnyElement& lobby, const AnyElement& peers,
                                       const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"lobby_created"}},
            {"message", AnyElement{"Lobby created"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"lobby", lobby}, {"peers", peers}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_unsealed(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_unsealed"}},
                          {"message", AnyElement{"Lobby unsealed"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_sealed(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_sealed"}},
                          {"message", AnyElement{"Lobby sealed"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_max_players(int max_players, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_resized"}},
                          {"message", AnyElement{"Lobby resized"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"max_players", AnyElement{max_players}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_password_protected(bool password_protected,
                                                  const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_passworded"}},
                          {"message", AnyElement{"Lobby password protected"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"password_protected", AnyElement{password_protected}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_lobby_title(std::string title, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"lobby_titled"}},
            {"message", AnyElement{"Lobby title changed"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"lobby_title", AnyElement{title}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_ready(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"peer_ready"}},
            {"message", AnyElement{"Peer ready"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"peer_id", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_unready(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"peer_unready"}},
            {"message", AnyElement{"Peer unready"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"peer_id", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_left(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_left"}},
                          {"message", AnyElement{"Lobby Left"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_left(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"peer_left"}},
            {"message", AnyElement{"Peer left"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"peer_id", AnyElement{peer_id}}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
std::string notification_peer_kicked(const std::string& peer_id, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_left"}},
                          {"message", AnyElement{"Peer kicked"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"peer_id", AnyElement{peer_id}},
                                       {"kicked", AnyElement{true}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_kicked() {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"lobby_kicked"}}, {"message", AnyElement{"Lobby kicked"}}}}
        .to_string();
}

std::string notification_peer_state(const AnyElement& peer_json) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_state"}},
                          {"message", AnyElement{"Initial Message"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"peer", peer_json}}}}}}
        .to_string();
}

std::string notification_lobby_call(const AnyElement& result, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_call"}},
                          {"message", AnyElement{"Lobby Call"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"result", result}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_user_data(const AnyElement& user_data, const std::string& peer_id,
                                   const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_user_data"}},
                          {"message", AnyElement{"User Data"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"user_data", user_data},
                                       {"peer_id", AnyElement{peer_id}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_tags(const AnyElement& tags, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_tags"}},
                          {"message", AnyElement{"Tags Set"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"tags", tags}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_chat(
    const std::string& from_peer, const std::string& chat_data, const std::string& command_id,
    const boost::container::flat_map<std::string, AnyElement>& chat_metadata) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_chat"}},
                          {"message", AnyElement{"Chat"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"from_peer", AnyElement{from_peer}},
                                       {"chat_data", AnyElement{chat_data}},
                                       {"chat_metadata", AnyElement{chat_metadata}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_reconnected(const std::string& peer_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_reconnected"}},
                          {"message", AnyElement{"Peer reconnected"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"peer_id", AnyElement{peer_id}}}}}}}
        .to_string();
}

std::string notification_lobby_joined(const AnyElement& lobby, const AnyElement& peers,
                                      const std::string& command_id) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"joined_lobby"}},
            {"message", AnyElement{"Lobby joined"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"lobby", lobby}, {"peers", peers}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_joined(const AnyElement& peer_json) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_joined"}},
                          {"message", AnyElement{"Peer joined"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"peer", peer_json}}}}}}
        .to_string();
}

std::string notification_peer_disconnected(const std::string& peer_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"peer_disconnected"}},
                          {"message", AnyElement{"Peer disconnected"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"peer_id", AnyElement{peer_id}}}}}}}
        .to_string();
}

std::string notification_lobby_list(const AnyElement& lobbies, const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_list"}},
                          {"message", AnyElement{"List Lobbies"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"lobbies", lobbies}, {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_public_data(const AnyElement& lobby_data,
                                           const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_data"}},
                          {"message", AnyElement{"Lobby Public Data"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"lobby_data", lobby_data},
                                       {"is_private", AnyElement{false}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_lobby_private_data(const AnyElement& lobby_data,
                                            const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"lobby_data"}},
                          {"message", AnyElement{"Lobby Private Data"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"lobby_data", lobby_data},
                                       {"is_private", AnyElement{true}},
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_peer_notify(const AnyElement& peer_data, const std::string& from_peer) {
    return AnyElement{
        boost::container::flat_map<std::string, AnyElement>{
            {"command", AnyElement{"peer_notify"}},
            {"message", AnyElement{"Notification"}},
            {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                         {"peer_data", peer_data}, {"from_peer", AnyElement{from_peer}}}}}}}
        .to_string();
}

std::string notification_peer_public_data(const AnyElement& peer_data, const std::string& from_peer,
                                          const std::string& target_peer) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"data_to"}},
                          {"message", AnyElement{"Peer public data"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
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
                          {"command", AnyElement{"data_to"}},
                          {"message", AnyElement{"Peer private data"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"peer_data", AnyElement{peer_data}},
                                       {"from_peer", AnyElement{from_peer}},
                                       {"target_peer", AnyElement{target_peer}},
                                       {"is_private", AnyElement{true}}}}}}}
        .to_string();
}

std::string notification_data_to_sent(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"data_to_sent"}},
                          {"message", AnyElement{"Data to sent"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}

std::string notification_notify_sent(const std::string& command_id) {
    return AnyElement{boost::container::flat_map<std::string, AnyElement>{
                          {"command", AnyElement{"notify_to_sent"}},
                          {"message", AnyElement{"Notify to sent"}},
                          {"data", AnyElement{boost::container::flat_map<std::string, AnyElement>{
                                       {"id", AnyElement{command_id}}}}}}}
        .to_string();
}
