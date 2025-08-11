#include "../src/common/input_validator.h"
#include <iostream>
#include <cassert>

/**
 * Simple unit tests for the InputValidator class.
 * This demonstrates testing the improvement made to input validation.
 */

void test_lobby_name_validation() {
    // Valid names
    assert(InputValidator::is_valid_lobby_name("Test Lobby"));
    assert(InputValidator::is_valid_lobby_name("My Game"));
    assert(InputValidator::is_valid_lobby_name("a"));
    
    // Invalid names
    assert(!InputValidator::is_valid_lobby_name(""));  // empty
    assert(!InputValidator::is_valid_lobby_name("Test\nLobby"));  // newline
    assert(!InputValidator::is_valid_lobby_name("Test\tLobby"));  // tab
    assert(!InputValidator::is_valid_lobby_name(std::string(65, 'a')));  // too long
    
    std::cout << "✓ Lobby name validation tests passed\n";
}

void test_max_players_validation() {
    // Valid values
    assert(InputValidator::is_valid_max_players(1));
    assert(InputValidator::is_valid_max_players(8));
    assert(InputValidator::is_valid_max_players(64));
    
    // Invalid values  
    assert(!InputValidator::is_valid_max_players(0));
    assert(!InputValidator::is_valid_max_players(-1));
    assert(!InputValidator::is_valid_max_players(65));
    assert(!InputValidator::is_valid_max_players(1000));
    
    std::cout << "✓ Max players validation tests passed\n";
}

void test_command_id_validation() {
    // Valid commands
    assert(InputValidator::is_valid_command_id(0));   // COMMAND_LOBBY_DATA
    assert(InputValidator::is_valid_command_id(22));  // COMMAND_STOP_LISTING
    
    // Invalid commands
    assert(!InputValidator::is_valid_command_id(-1));
    assert(!InputValidator::is_valid_command_id(23));
    assert(!InputValidator::is_valid_command_id(100));
    
    std::cout << "✓ Command ID validation tests passed\n";
}

void test_string_sanitization() {
    assert(InputValidator::sanitize_string("Hello World") == "Hello World");
    assert(InputValidator::sanitize_string("Test\x01\x02\x03") == "Test");  // control chars removed
    assert(InputValidator::sanitize_string("Test\nNew\tLine") == "TestNewLine");  // newline/tab removed
    assert(InputValidator::sanitize_string("") == "");  // empty string
    
    std::cout << "✓ String sanitization tests passed\n";
}

void test_chat_message_validation() {
    assert(InputValidator::is_valid_chat_message("Hello!"));
    assert(InputValidator::is_valid_chat_message("a"));
    
    assert(!InputValidator::is_valid_chat_message(""));  // empty
    assert(!InputValidator::is_valid_chat_message(std::string(1025, 'a')));  // too long
    
    std::cout << "✓ Chat message validation tests passed\n";
}

int main() {
    std::cout << "Running InputValidator unit tests...\n\n";
    
    test_lobby_name_validation();
    test_max_players_validation(); 
    test_command_id_validation();
    test_string_sanitization();
    test_chat_message_validation();
    
    std::cout << "\n✅ All tests passed! Input validation is working correctly.\n";
    return 0;
}