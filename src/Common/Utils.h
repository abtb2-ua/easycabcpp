#ifndef UTILS_H
#define UTILS_H

#include <iostream>

using namespace std;

class Address {
private:
    string ip;
    int port;

public:
    Address();

    Address(string ip, int port);

    [[nodiscard]] string getIp() const;

    [[nodiscard]] int getPort() const;

    bool setIp(const string &ip);

    bool setPort(int port);

    string toString() const;
};


#endif //UTILS_H
