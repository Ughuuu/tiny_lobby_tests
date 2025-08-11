#pragma once
#include <limits>
#include <string>

/**
 * Input validation utilities for game server commands.
 * Provides safe input validation with proper bounds checking.
 */
class InputValidator {
   public:
    // String validation
    static bool is_valid_lobby_name(const std::string& name) {
        return !name.empty() && name.length() <= 64 &&
               name.find_first_of("\n\r\t") == std::string::npos;
    }

    static bool is_valid_peer_id(const std::string& id) {
        return !id.empty() && id.length() <= 128;
    }

    static bool is_valid_password(const std::string& password) { return password.length() <= 256; }

    // Numeric validation
    static bool is_valid_max_players(int64_t max_players) {
        return max_players >= 1 && max_players <= 64;
    }

    static bool is_valid_command_id(int command) {
        return command >= 0 && command <= 22;  // Based on COMMAND_STOP_LISTING = 22
    }

    // Chat message validation
    static bool is_valid_chat_message(const std::string& message) {
        return !message.empty() && message.length() <= 1024;
    }

    // Tag validation
    static bool is_valid_tag_key(const std::string& key) {
        return !key.empty() && key.length() <= 32 &&
               key.find_first_of("\n\r\t ") == std::string::npos;
    }

    static bool is_valid_tag_value_string(const std::string& value) {
        return value.length() <= 128;
    }

    // Sanitize string input by removing control characters
    static std::string sanitize_string(const std::string& input) {
        std::string result;
        result.reserve(input.length());
        for (char c : input) {
            if (c >= 32 && c <= 126) {  // Printable ASCII characters only
                result.push_back(c);
            }
        }
        return result;
    }
};