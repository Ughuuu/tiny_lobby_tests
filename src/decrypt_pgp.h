#pragma once
#include <gpgme.h>
#include <string>

void init_gpgme();

void deinit_gpgme();

void import_public_key();

gpgme_key_t get_public_key(const std::string& key_id);

bool verify_detached_signature(const std::string& data, const std::string& signature);
