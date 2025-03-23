#include "database.h"
#include <iostream>
#include "INIReader.h"

pqxx::connection *connection;

bool ensure_connection() {
    if (!connection->is_open()) {
        std::cerr << "Connection lost, attempting to reconnect..." << std::endl;
        connect_to_db();
        return false;
    }
    return true;
}

void connect_to_db() {
    try {
        INIReader config_reader("config.ini");
        std::string database = config_reader.Get("database", "database", "");
        if (database.empty()) {
            std::cerr << "Database name not found in config.ini" << std::endl;
            exit(1);
        }
        std::string user = config_reader.Get("database", "user", "");
        if (user.empty()) {
            std::cerr << "Database user not found in config.ini" << std::endl;
            exit(1);
        }
        std::string password = config_reader.Get("database", "password", "");
        if (password.empty()) {
            std::cerr << "Database password not found in config.ini" << std::endl;
            exit(1);
        }
        std::string host = config_reader.Get("database", "host", "");
        if (host.empty()) {
            std::cerr << "Database host not found in config.ini" << std::endl;
            exit(1);
        }
        std::string port = config_reader.Get("database", "port", "");
        if (port.empty()) {
            std::cerr << "Database port not found in config.ini" << std::endl;
            exit(1);
        }
        std::string conn_str = "dbname=" + database + " user=" + user + " password=" + password + " host=" + host + " port=" + port;
        connection = new pqxx::connection(conn_str);
        if (connection->is_open()) {
            std::cout << "Connected to database: " << connection->dbname() << std::endl;
        } else {
            std::cerr << "Can't open database" << std::endl;
            exit(1);
        }

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}

void execute_query(std::string query) {
    ensure_connection();
    pqxx::work work(*connection);
    pqxx::result result = work.exec("SELECT id, name FROM employees;");
    for (auto row : result) {
        std::cout << "ID: " << row[0].as<int>() << " Name: " << row[1].as<std::string>() << std::endl;
    }
    work.commit();
}

void close_connection() {
    connection->close();
    delete connection;
}
