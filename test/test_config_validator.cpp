#include "../src/common/config_validator.h"
#include <iostream>
#include <cassert>

/**
 * Unit tests for ConfigValidator utility
 */

void test_webserver_config_validation() {
    // Valid configuration
    auto result = ConfigValidator::validate_webserver_config(8080, 1000, 2048, 30);
    assert(result.is_valid);
    assert(result.errors.empty());
    
    // Invalid port
    result = ConfigValidator::validate_webserver_config(0, 1000, 2048, 30);
    assert(!result.is_valid);
    assert(!result.errors.empty());
    
    // Port requiring admin privileges (warning)
    result = ConfigValidator::validate_webserver_config(80, 1000, 2048, 30);
    assert(result.is_valid);
    assert(!result.warnings.empty());
    
    // Invalid max_users
    result = ConfigValidator::validate_webserver_config(8080, 0, 2048, 30);
    assert(!result.is_valid);
    
    // Invalid payload length
    result = ConfigValidator::validate_webserver_config(8080, 1000, 100, 30);
    assert(!result.is_valid);
    
    // Invalid idle timeout
    result = ConfigValidator::validate_webserver_config(8080, 1000, 2048, 0);
    assert(!result.is_valid);
    
    std::cout << "✓ Webserver config validation tests passed\n";
}

void test_game_config_validation() {
    // Valid configuration
    auto result = ConfigValidator::validate_game_config(60, 30, 1000, 360000);
    assert(result.is_valid);
    
    // Low tickrate warning
    result = ConfigValidator::validate_game_config(10, 30, 1000, 360000);
    assert(result.is_valid);
    assert(!result.warnings.empty());
    
    // High tickrate warning  
    result = ConfigValidator::validate_game_config(2000, 30, 1000, 360000);
    assert(result.is_valid);
    assert(!result.warnings.empty());
    
    // Invalid listing interval
    result = ConfigValidator::validate_game_config(60, 30, 50, 360000);
    assert(!result.is_valid);
    
    std::cout << "✓ Game config validation tests passed\n";
}

void test_database_config_validation() {
    // Valid configuration
    auto result = ConfigValidator::validate_database_config("localhost", 5432, "lobby", "user");
    assert(result.is_valid);
    
    // Empty host
    result = ConfigValidator::validate_database_config("", 5432, "lobby", "user");
    assert(!result.is_valid);
    
    // Invalid port
    result = ConfigValidator::validate_database_config("localhost", 0, "lobby", "user");
    assert(!result.is_valid);
    
    // Empty database name
    result = ConfigValidator::validate_database_config("localhost", 5432, "", "user");
    assert(!result.is_valid);
    
    // Empty username (warning)
    result = ConfigValidator::validate_database_config("localhost", 5432, "lobby", "");
    assert(result.is_valid);
    assert(!result.warnings.empty());
    
    std::cout << "✓ Database config validation tests passed\n";
}

void test_path_validation() {
    // Valid paths
    auto result = ConfigValidator::validate_paths("/scripts", "/logs");
    assert(result.is_valid);
    
    // Empty scripts folder
    result = ConfigValidator::validate_paths("", "/logs");
    assert(!result.is_valid);
    
    // Empty logs folder (warning)
    result = ConfigValidator::validate_paths("/scripts", "");
    assert(result.is_valid);
    assert(!result.warnings.empty());
    
    // Path traversal warning
    result = ConfigValidator::validate_paths("/scripts/../dangerous", "/logs");
    assert(result.is_valid);
    assert(!result.warnings.empty());
    
    std::cout << "✓ Path validation tests passed\n";
}

int main() {
    std::cout << "Running ConfigValidator unit tests...\n\n";
    
    test_webserver_config_validation();
    test_game_config_validation();
    test_database_config_validation();
    test_path_validation();
    
    std::cout << "\n✅ All configuration validation tests passed!\n";
    
    // Demo of validation output formatting
    std::cout << "\n--- Example validation output ---\n";
    auto result = ConfigValidator::validate_webserver_config(80, 50000, 100, 5);
    ConfigValidator::print_validation_results("Webserver", result);
    
    return 0;
}