#pragma once
#include <iostream>
#include <string>
#include <vector>

/**
 * Configuration validation utilities for the lobby server.
 * Provides validation for configuration files and runtime settings.
 */
class ConfigValidator {
   public:
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    // Validate webserver configuration
    static ValidationResult validate_webserver_config(int port, int max_users,
                                                      int max_payload_length, int idle_timeout) {
        ValidationResult result{true, {}, {}};

        if (port < 1 || port > 65535) {
            result.is_valid = false;
            result.errors.push_back("Port must be between 1 and 65535");
        }

        if (port < 1024) {
            result.warnings.push_back("Port < 1024 may require administrator privileges");
        }

        if (max_users < 1 || max_users > 100000) {
            result.is_valid = false;
            result.errors.push_back("max_users must be between 1 and 100000");
        }

        if (max_payload_length < 256 || max_payload_length > 64 * 1024 * 1024) {
            result.is_valid = false;
            result.errors.push_back("max_payload_length must be between 256 bytes and 64MB");
        }

        if (idle_timeout < 5 || idle_timeout > 3600) {
            result.is_valid = false;
            result.errors.push_back("idle_timeout must be between 5 and 3600 seconds");
        }

        return result;
    }

    // Validate game configuration
    static ValidationResult validate_game_config(int tickrate, int sendrate, int listing_interval,
                                                 int max_reconnection_time) {
        ValidationResult result{true, {}, {}};

        if (tickrate < 0 || (tickrate > 0 && tickrate < 16)) {
            result.warnings.push_back("Tickrate below 16ms may cause high CPU usage");
        }

        if (tickrate > 1000) {
            result.warnings.push_back("Very high tickrate may impact performance");
        }

        if (sendrate < 0 || (sendrate > 0 && sendrate < 16)) {
            result.warnings.push_back("Sendrate below 16ms may cause high network usage");
        }

        if (listing_interval < 100) {
            result.is_valid = false;
            result.errors.push_back("listing_interval must be at least 100ms");
        }

        if (max_reconnection_time < 5000) {
            result.warnings.push_back(
                "Very short reconnection time may cause frequent disconnects");
        }

        if (max_reconnection_time > 3600000) {  // 1 hour
            result.warnings.push_back("Very long reconnection time may consume server resources");
        }

        return result;
    }

    // Validate database configuration
    static ValidationResult validate_database_config(const std::string& host, int port,
                                                     const std::string& database,
                                                     const std::string& username) {
        ValidationResult result{true, {}, {}};

        if (host.empty()) {
            result.is_valid = false;
            result.errors.push_back("Database host cannot be empty");
        }

        if (port < 1 || port > 65535) {
            result.is_valid = false;
            result.errors.push_back("Database port must be between 1 and 65535");
        }

        if (database.empty()) {
            result.is_valid = false;
            result.errors.push_back("Database name cannot be empty");
        }

        if (username.empty()) {
            result.warnings.push_back("Empty database username may cause connection issues");
        }

        return result;
    }

    // Validate file paths
    static ValidationResult validate_paths(const std::string& scripts_folder,
                                           const std::string& logs_folder) {
        ValidationResult result{true, {}, {}};

        if (scripts_folder.empty()) {
            result.is_valid = false;
            result.errors.push_back("Scripts folder path cannot be empty");
        }

        if (logs_folder.empty()) {
            result.warnings.push_back("Logs folder not specified, using default");
        }

        // Basic path validation (could be extended with filesystem checks)
        if (scripts_folder.find("..") != std::string::npos) {
            result.warnings.push_back("Scripts folder contains '..' - potential security risk");
        }

        return result;
    }

    // Print validation results in a user-friendly format
    static void print_validation_results(const std::string& section,
                                         const ValidationResult& result) {
        if (!result.errors.empty()) {
            std::cout << "[ERROR] " << section << " configuration:\n";
            for (const auto& error : result.errors) {
                std::cout << "  × " << error << "\n";
            }
        }

        if (!result.warnings.empty()) {
            std::cout << "[WARNING] " << section << " configuration:\n";
            for (const auto& warning : result.warnings) {
                std::cout << "  ⚠ " << warning << "\n";
            }
        }

        if (result.is_valid && result.warnings.empty()) {
            std::cout << "[OK] " << section << " configuration is valid\n";
        }
    }
};