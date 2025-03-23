#include "decrypt_pgp.h"
#include "public_key.h"
#include <iostream>
#include <cstring>

gpgme_ctx_t ctx;

void init_gpgme() {
    if (strcmp(GPGME_VERSION, gpgme_check_version(NULL)) != 0) {
        std::cerr << "GPGME library version mismatch!" << std::endl;
        exit(1);
    }
    gpgme_error_t err = gpgme_new(&ctx);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Failed to initialize GPGME context." << gpgme_strerror(err) << std::endl;
        exit(1);
    }
}

void deinit_gpgme() {
    gpgme_release(ctx);
}

void import_public_key() {
    // Create GPGME data object
    gpgme_data_t data;
    gpgme_error_t err = gpgme_data_new_from_mem(&data, public_key.c_str(), public_key.length(), 0);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Failed to create GPGME data object from key file." << gpgme_strerror(err) << std::endl;
        return;
    }

    // Import the key into the keyring
    err = gpgme_op_import(ctx, data);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Failed to import public key." << gpgme_strerror(err) << std::endl;
        return;
    }

    // Clean up the data object
    gpgme_data_release(data);
    std::cout << "Public key imported successfully." << gpgme_strerror(err) << std::endl;
}

gpgme_key_t get_public_key(const std::string& key_id) {
    gpgme_key_t key;
    gpgme_error_t err = gpgme_get_key(ctx, key_id.c_str(), &key, 0);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Failed to retrieve public key." << gpgme_strerror(err) << std::endl;
        return nullptr;
    }
    return key;
}

bool verify_detached_signature(const std::string& data, const std::string& signature) {
    // Create GPGME data objects for message and signature
    gpgme_data_t in, sig;
    gpgme_error_t err;

    // Create data object from the original message
    err = gpgme_data_new_from_mem(&in, data.c_str(), data.length(), 0);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Failed to create input data object." << gpgme_strerror(err) << std::endl;
        return false;
    }

    // Create data object from the base64-decoded signature
    std::string decoded_signature;
    if (signature.empty()) {
        std::cerr << "Signature is empty." << gpgme_strerror(err) << std::endl;
        return false;
    }

    // Decode base64 signature (if necessary)
    decoded_signature = signature; // Here, use base64 decode if necessary
    err = gpgme_data_new_from_mem(&sig, decoded_signature.c_str(), decoded_signature.length(), 0);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Failed to create signature data object." << gpgme_strerror(err) << std::endl;
        return false;
    }

    // Verify the signature
    err = gpgme_op_verify(ctx, sig, in, nullptr);
    if (err != GPG_ERR_NO_ERROR) {
        std::cerr << "Signature verification failed." << gpgme_strerror(err) << std::endl;
        return false;
    }

    // Clean up data objects
    gpgme_data_release(in);
    gpgme_data_release(sig);

    return true;
}
