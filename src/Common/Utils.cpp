#include "Utils.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

Address::Address() {
    this->ip = "";
    this->port = 0;
}

string Address::getIp() const {
    return this->ip;
}

int Address::getPort() const {
    return this->port;
}

bool Address::setIp(const string &ip) {
    if (ip.empty()) {
        return false;
    }

    int n;
    char c;
    stringstream ss(ip);
    for (int i = 0; i < 4; i++) {
        ss >> n;
        if (ss.fail() || n < 0 || n > 255) {
            return false;
        }

        if (i < 3) {
            ss >> c;
            if (c != '.') {
                return false;
            }
        }
    }

    this->ip = ip;
    return true;
}

bool Address::setPort(const int port) {
    if (port < 0 || port > 65535) {
        return false;
    }

    this->port = port;
    return true;
}

string Address::toString() const {
    return this->ip + ":" + to_string(this->port);
}
