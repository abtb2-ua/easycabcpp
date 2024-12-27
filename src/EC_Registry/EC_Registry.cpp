#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <curl/curl.h>
#include <sstream>

using namespace std;

class EC_Registry {
public:
    EC_Registry() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~EC_Registry() {
        curl_global_cleanup();
    }

    bool register_taxi(const string& taxi_id, const string& certificate_path) {
        string certificate_hash = get_certificate_hash(certificate_path);
        if (certificate_hash.empty()) {
            cerr << "Error al obtener el hash del certificado." << endl;
            return false;
        }

        registry_db[taxi_id] = certificate_hash;
        cout << "Taxi registrado: " << taxi_id << endl;
        return true;
    }

    bool verify_certificate(const string& taxi_id, const string& certificate_path) {
        if (registry_db.find(taxi_id) == registry_db.end()) {
            cerr << "Taxi no registrado." << endl;
            return false;
        }

        string certificate_hash = get_certificate_hash(certificate_path);
        if (certificate_hash.empty()) {
            cerr << "Error al obtener el hash del certificado." << endl;
            return false;
        }

        if (registry_db[taxi_id] == certificate_hash) {
            cout << "Certificado válido para el taxi: " << taxi_id << endl;
            return true;
        } else {
            cerr << "Certificado inválido para el taxi: " << taxi_id << endl;
            return false;
        }
    }

private:
    unordered_map<string, string> registry_db;
    CURL* curl;

    string get_certificate_hash(const string& certificate_path) {
        ifstream cert_file(certificate_path, ios::binary);
        if (!cert_file) {
            cerr << "No se pudo abrir el archivo del certificado." << endl;
            return "";
        }

        string cert_data((istreambuf_iterator<char>(cert_file)), istreambuf_iterator<char>());
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256_ctx;
        SHA256_Init(&sha256_ctx);
        SHA256_Update(&sha256_ctx, cert_data.c_str(), cert_data.size());
        SHA256_Final(hash, &sha256_ctx);

        stringstream hash_stream;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            hash_stream << hex << (int)hash[i];
        }
        return hash_stream.str();
    }

    // Función para enviar solicitudes HTTP (por ejemplo, para verificar certificados)
    bool send_http_request(const string& url) {
        CURLcode res;
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                cerr << "Error al realizar la solicitud HTTP: " << curl_easy_strerror(res) << endl;
                curl_easy_cleanup(curl);
                return false;
            }
            curl_easy_cleanup(curl);
        }
        return true;
    }
};

int main() {
    EC_Registry registry;

    string taxi_id = "12345";
    string certificate_path = "taxi_12345_cert.pem";
    if (registry.register_taxi(taxi_id, certificate_path)) {
        cout << "Taxi registrado con éxito." << endl;
    }

    string taxi_id_to_verify = "12345";
    if (registry.verify_certificate(taxi_id_to_verify, certificate_path)) {
        cout << "El taxi fue autenticado correctamente." << endl;
    } else {
        cout << "El taxi no pudo ser autenticado." << endl;
    }

    return 0;
}
