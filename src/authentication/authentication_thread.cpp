#include "authentication_thread.h"
#include "../websocket/websocket_server.h"
#include "../database/database.h"
#include "../common/any_type.h"
#include "login_client.h"

AuthenticationThread::AuthenticationThread(
    moodycamel::BlockingReaderWriterQueue<AuthenticationMessage> &authentication_queue,
    bool verbose, std::string log_folder, struct uWS::Loop *loop)
    : authentication_queue(authentication_queue),
      logger(verbose, log_folder + "/websocket.txt"),
      loop(loop),
      db() {}

void AuthenticationThread::run() {
    if (db.enabled) {
        db.connect_to_db();
    } else {
        logger.debug_log("[WebSocketServer] Database is disabled. Peer state is disabled.");
    }
    while (running) {
        AuthenticationMessage auth_message;
        authentication_queue.wait_dequeue(auth_message);
        PerSocketData user_data = auth_message.user_data;
        auto &res = auth_message.res;
        auto &context = auth_message.context;
        auto &abort_shared = auth_message.abort;
        auto &websocket_key = auth_message.websocket_key;
        auto &websocket_extensions = auth_message.websocket_extensions;
        // probably not a jwt.
        if (!user_data.reconnection_token.contains(".")) {
            loop->defer([res, user_data = std::move(user_data), websocket_key, websocket_extensions,
                         context, abort_shared, this]() mutable {
                if (*abort_shared) {
                    return;
                }
                // check if the user has a peer_id associated with the reconnection_token if db
                // enabled
                if (db.enabled) {
                    user_data.id = db.get_peer_or_insert(user_data.reconnection_token,
                                                         user_data.game_id, user_data.id);
                }
                logger.debug_log("[WebSocketServer] upgraded anon user: ", user_data.game_id, " ",
                                 user_data.id);
                res->template upgrade<PerSocketData>(std::move(user_data), websocket_key,
                                                     "appsinacup", websocket_extensions, context);
            });
            continue;
        }
        LoginClient client("login.appsinacup.com", user_data.game_id);
        std::string error_msg;
        std::string result = client.verify_jwt(user_data.reconnection_token, error_msg);
        loop->defer([res, result, user_data = std::move(user_data), error_msg, websocket_key,
                     websocket_extensions, context, abort_shared, this]() mutable {
            if (*abort_shared) {
                return;
            }
            if (error_msg != "") {
                res->writeStatus("400 Bad Request")->write("Failed to authenticate. " + error_msg);
                res->end();
                this->logger.error_log("[WebSocketServer] error: ", error_msg);
            } else {
                yyjson_doc *doc = yyjson_read(result.c_str(), result.size(), 0);
                if (!doc) {
                    res->writeStatus("400 Bad Request")
                        ->write("Failed to authenticate. Cannot read login doc result.");
                    res->end();
                    this->logger.error_log("[WebSocketServer] error: ",
                                           "Cannot read login doc result");
                    return;
                }

                yyjson_val *root = yyjson_doc_get_root(doc);
                if (!root) {
                    res->writeStatus("400 Bad Request")
                        ->write("Failed to authenticate. Cannot read login root result.");
                    res->end();
                    this->logger.error_log("[WebSocketServer] error: ",
                                           "Cannot read login root result");
                    return;
                }
                boost::container::flat_map<std::string, AnyElement> auth_result;
                std::string decode_error = decode_object(root, auth_result);
                yyjson_doc_free(doc);
                if (!decode_error.empty()) {
                    res->writeStatus("400 Bad Request")
                        ->write("Failed to authenticate. Cannot decode login result. " +
                                decode_error);
                    res->end();
                    this->logger.error_log("[WebSocketServer] error: ", decode_error);
                    return;
                }
                if (auth_result.find("success") == auth_result.end()) {
                    res->writeStatus("400 Bad Request")
                        ->write("Failed to authenticate. Cannot find success in login result.");
                    res->end();
                    this->logger.error_log("[WebSocketServer] error: ",
                                           "Cannot find success in login result");
                    return;
                }
                auto &sucess_value = auth_result["success"];
                if (std::holds_alternative<bool>(sucess_value.value)) {
                    bool success = std::get<bool>(sucess_value.value);
                    if (!success) {
                        res->writeStatus("400 Bad Request")
                            ->write("Failed to authenticate. Failed to login.");
                        res->end();
                        this->logger.error_log("[WebSocketServer] error: ", "Failed to login");
                        return;
                    }
                } else {
                    res->writeStatus("400 Bad Request")
                        ->write("Failed to authenticate. Success is not a boolean.");
                    res->end();
                    this->logger.error_log("[WebSocketServer] error: ", "Success is not a boolean");
                    return;
                }
                if (auth_result.find("data") == auth_result.end()) {
                    res->writeStatus("400 Bad Request")
                        ->write("Failed to authenticate. Cannot find data in login result.");
                    res->end();
                    this->logger.error_log("[WebSocketServer] error: ",
                                           "Cannot find data in login result");
                    return;
                }
                if (auto auth_data =
                        std::get_if<boost::container::flat_map<std::string, AnyElement>>(
                            &auth_result["data"].value)) {
                    if (auth_data->find("token") == auth_data->end()) {
                        res->writeStatus("400 Bad Request")
                            ->write("Failed to authenticate. Cannot find token in login result.");
                        res->end();
                        this->logger.error_log("[WebSocketServer] error: ",
                                               "Cannot find token in login result");
                        return;
                    }
                    if (auto token =
                            std::get_if<boost::container::flat_map<std::string, AnyElement>>(
                                &(*auth_data)["token"].value)) {
                        if (token->find("name") == token->end()) {
                            res->writeStatus("400 Bad Request")
                                ->write(
                                    "Failed to authenticate. Cannot find name in login result.");
                            res->end();
                            this->logger.error_log("[WebSocketServer] error: ",
                                                   "Cannot find name in login result");
                            return;
                        }
                        if (token->find("platform") == token->end()) {
                            res->writeStatus("400 Bad Request")
                                ->write(
                                    "Failed to authenticate. Cannot find platform in login "
                                    "result.");
                            res->end();
                            this->logger.error_log("[WebSocketServer] error: ",
                                                   "Cannot find platform in login result");
                            return;
                        }
                        if (token->find("platform_id") == token->end()) {
                            res->writeStatus("400 Bad Request")
                                ->write(
                                    "Failed to authenticate. Cannot find platform_id in login "
                                    "result.");
                            res->end();
                            this->logger.error_log("[WebSocketServer] error: ",
                                                   "Cannot find platform_id in login result");
                            return;
                        }
                        user_data.platform = std::get<std::string>((*token)["platform"].value);
                        user_data.name = std::get<std::string>((*token)["name"].value);
                        user_data.platform_id =
                            std::get<std::string>((*token)["platform_id"].value);
                        user_data.reconnection_token =
                            user_data.platform + ":" + user_data.platform_id;
                    }
                }
                this->logger.debug_log("[WebSocketServer] upgraded authenticated user: ",
                                       user_data.game_id, " ", user_data.id);
                res->template upgrade<PerSocketData>(std::move(user_data), websocket_key,
                                                     "appsinacup", websocket_extensions, context);
            }
        });
    }
}

void AuthenticationThread::stop() {
    running = false;
    if (db.enabled) {
        db.close_connection();
    }
    logger.debug_log("[WebSocketServer] Authentication thread stopped.");
}
