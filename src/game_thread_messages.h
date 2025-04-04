#pragma once
#include <string>

std::string ERROR_CANNOT_PARSE_JSON = "Cannot parse json";
std::string ERROR_PEER_NOT_FOUND = "Peer not found";
std::string ERROR_PEER_ALREADY_EXISTS = "Peer already exists";
std::string ERROR_PEER_NOT_IN_A_LOBBY = "Peer not in a lobby";
std::string ERROR_CANNOT_KICK_SELF = "Cannot kick self";
std::string ERROR_INVALID_FUNCTION = "Invalid function";
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

std::string NOTIFICATION_ERROR =
    "{"
    "\"command\": \"error\","
    "\"message\": \"%s\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOGICAL_ERROR =
    "{"
    "\"command\": \"error\","
    "\"message\": \"%s\","
    "\"data\": {"
    "\"id\": \"%s\""
    "},"
    "\"is_logical_error\": true"
    "}";
std::string NOTIFICATION_LOBBY_CREATED =
    "{"
    "\"command\": \"lobby_created\","
    "\"message\": \"Lobby created\","
    "\"data\": {"
    "\"lobby\": %s,"
    "\"peers\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_UNSEALED =
    "{"
    "\"command\": \"lobby_unsealed\","
    "\"message\": \"Lobby unsealed\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_SEALED =
    "{"
    "\"command\": \"lobby_sealed\","
    "\"message\": \"Lobby sealed\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_READY =
    "{"
    "\"command\": \"peer_ready\","
    "\"message\": \"Peer ready\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_UNREADY =
    "{"
    "\"command\": \"peer_unready\","
    "\"message\": \"Peer unready\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_LEFT =
    "{"
    "\"command\": \"lobby_left\","
    "\"message\": \"Lobby Left\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_LEFT =
    "{"
    "\"command\": \"peer_left\","
    "\"message\": \"Peer left\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_KICKED =
    "{"
    "\"command\": \"peer_left\","
    "\"message\": \"Peer kicked\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"kicked\": true,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_KICKED =
    "{"
    "\"command\": \"lobby_kicked\","
    "\"message\": \"Lobby kicked\""
    "}";
std::string NOTIFICATION_PEER_STATE =
    "{"
    "\"command\": \"peer_state\","
    "\"message\": \"Initial Message\","
    "\"data\": {"
    "\"peer\": %s"
    "}"
    "}";
std::string NOTIFICATION_LOBBY_CALL =
    "{"
    "\"command\": \"lobby_call\","
    "\"message\": \"Lobby Call\","
    "\"data\": {"
    "\"result\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_USER_DATA =
    "{"
    "\"command\": \"peer_user_data\","
    "\"message\": \"User Data\","
    "\"data\": {"
    "\"user_data\": %s,"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_TAGS =
    "{"
    "\"command\": \"lobby_tags\","
    "\"message\": \"Tags Set\","
    "\"data\": {"
    "\"tags\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_CHAT =
    "{"
    "\"command\": \"peer_chat\","
    "\"message\": \"Chat\","
    "\"data\": {"
    "\"from_peer\": \"%s\","
    "\"chat_data\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_RECONNECTED =
    "{"
    "\"command\": \"peer_reconnected\","
    "\"message\": \"Peer reconnected\","
    "\"data\": {"
    "\"peer_id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_JOINED =
    "{"
    "\"command\": \"joined_lobby\","
    "\"message\": \"Lobby joined\","
    "\"data\": {"
    "\"lobby\": %s,"
    "\"peers\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_JOINED =
    "{"
    "\"command\": \"peer_joined\","
    "\"message\": \"Peer joined\","
    "\"data\": {"
    "\"peer\": %s"
    "}"
    "}";
std::string NOTIFICATION_PEER_DICONNECTED =
    "{"
    "\"command\": \"peer_disconnected\","
    "\"message\": \"Peer disconnected\","
    "\"data\": {"
    "\"peer_id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_LIST =
    "{"
    "\"command\": \"lobby_list\","
    "\"message\": \"List Lobbies\","
    "\"data\": {"
    "\"lobbies\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_PUBLIC_DATA =
    "{"
    "\"command\": \"lobby_data\","
    "\"message\": \"Lobby Public Data\","
    "\"data\": {"
    "\"lobby_data\": %s,"
    "\"is_private\": false,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_PRIVATE_DATA =
    "{"
    "\"command\": \"lobby_data\","
    "\"message\": \"Lobby Private Data\","
    "\"data\": {"
    "\"lobby_data\": %s,"
    "\"is_private\": true,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_NOTIFICATION =
    "{"
    "\"command\": \"peer_notify\","
    "\"message\": \"Notification\","
    "\"data\": {"
    "\"peer_data\": %s,"
    "\"from_peer\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_PUBLIC_DATA =
    "{"
    "\"command\": \"data_to\","
    "\"message\": \"Peer public data\","
    "\"data\": {"
    "\"peer_data\": %s,"
    "\"from_peer\": \"%s\","
    "\"target_peer\": \"%s\","
    "\"is_private\": false"
    "}"
    "}";
std::string NOTIFICATION_PEER_PRIVATE_DATA =
    "{"
    "\"command\": \"data_to\","
    "\"message\": \"Peer private data\","
    "\"data\": {"
    "\"peer_data\": %s,"
    "\"from_peer\": \"%s\","
    "\"target_peer\": \"%s\","
    "\"is_private\": true"
    "}"
    "}";
std::string NOTIFICATION_DATA_TO_SENT =
    "{"
    "\"command\": \"data_to_sent\","
    "\"message\": \"Data to sent\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_NOTIFY_SENT =
    "{"
    "\"command\": \"notify_to_sent\","
    "\"message\": \"Notify to sent\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
