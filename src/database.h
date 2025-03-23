#pragma once
#include <pqxx/pqxx>
#include <iostream>

void connect_to_db();

void execute_query(std::string query);

void close_connection();
