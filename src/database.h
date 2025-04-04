#pragma once
#include <iostream>
#include <pqxx/pqxx>

void connect_to_db();

void execute_query(std::string query);

void close_connection();
