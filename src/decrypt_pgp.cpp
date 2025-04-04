#include "decrypt_pgp.h"

#include <iostream>
#include <vector>

#include "public_key.h"

rnp_ffi_t ffi = nullptr;

void init_rnp() {
    std::cout << "RNP backend: " << rnp_backend_string() << " version: " << rnp_backend_version()
              << std::endl;
    if (rnp_ffi_create(&ffi, "GPG", "GPG")) {
        std::cerr << "Failed to initialize RNP." << std::endl;
        exit(0);
    }
}

void deinit_rnp() {
    if (ffi) {
        rnp_ffi_destroy(ffi);
        ffi = nullptr;
        std::cout << "RNP is not initialized." << std::endl;
        exit(1);
    }
}
bool import_public_key() {
    if (!ffi) {
        std::cerr << "RNP is not initialized." << std::endl;
        return false;
    }

    rnp_input_t input = nullptr;
    if (rnp_input_from_memory(&input, reinterpret_cast<const uint8_t*>(public_key.data()),
                              public_key.size(), false)) {
        std::cerr << "Failed to create input stream." << std::endl;
        return false;
    }
    int result = rnp_import_keys(ffi, input, RNP_LOAD_SAVE_PUBLIC_KEYS, nullptr);
    rnp_input_destroy(input);

    if (result != 0) {
        std::cerr << "Failed to import public key. Error code: " << rnp_result_to_string(result)
                  << std::endl;
        return false;
    }

    std::cout << "Public key imported successfully." << std::endl;
    return true;
}

bool verify_detached_signature(const std::string& data, const std::string& signature) {
    if (!ffi) {
        std::cerr << "RNP is not initialized." << std::endl;
        return false;
    }

    rnp_input_t data_input = nullptr, sig_input = nullptr;
    rnp_op_verify_t verify = nullptr;
    // Create input streams for data and signature
    if (rnp_input_from_memory(&data_input, reinterpret_cast<const uint8_t*>(data.data()),
                              data.size(), false) ||
        rnp_input_from_memory(&sig_input, reinterpret_cast<const uint8_t*>(signature.data()),
                              signature.size(), false)) {
        std::cerr << "Failed to create input streams." << std::endl;
        return false;
    }

    // Create verification operation
    if (rnp_op_verify_detached_create(&verify, ffi, data_input, sig_input)) {
        std::cerr << "Failed to create verification operation." << std::endl;
        rnp_input_destroy(data_input);
        rnp_input_destroy(sig_input);
        return false;
    }

    // Execute verification
    if (rnp_op_verify_execute(verify)) {
        std::cerr << "Signature verification failed." << std::endl;
        rnp_op_verify_destroy(verify);
        rnp_input_destroy(data_input);
        rnp_input_destroy(sig_input);
        return false;
    }

    // Retrieve verification results
    size_t sig_count = 0;
    rnp_op_verify_get_signature_count(verify, &sig_count);
    if (sig_count == 0) {
        std::cerr << "No signatures found." << std::endl;
        rnp_op_verify_destroy(verify);
        rnp_input_destroy(data_input);
        rnp_input_destroy(sig_input);
        return false;
    }

    // Iterate over each signature and check if it's valid
    for (size_t i = 0; i < sig_count; i++) {
        rnp_op_verify_signature_t sig = nullptr;
        if (rnp_op_verify_get_signature_at(verify, i, &sig) == 0) {
            std::cerr << "Failed to retrieve signature." << std::endl;
            rnp_op_verify_destroy(verify);
            rnp_input_destroy(data_input);
            rnp_input_destroy(sig_input);
            return false;
        }

        if (rnp_op_verify_signature_get_status(sig) != 0) {
            std::cerr << "Failed to retrieve signature status." << std::endl;
            rnp_op_verify_destroy(verify);
            rnp_input_destroy(data_input);
            rnp_input_destroy(sig_input);
            return false;
        }
    }

    // Cleanup
    rnp_op_verify_destroy(verify);
    rnp_input_destroy(data_input);
    rnp_input_destroy(sig_input);

    std::cout << "Signature verification successful." << std::endl;
    return true;
}
