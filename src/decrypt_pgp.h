#pragma once
#include <rnp/rnp.h>

#include <string>

void init_rnp();
void deinit_rnp();
bool import_public_key();
bool verify_detached_signature(const std::string& data, const std::string& signature);
