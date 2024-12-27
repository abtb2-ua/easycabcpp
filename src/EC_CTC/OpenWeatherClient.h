//
// Created by ab-flies on 24/12/24.
//

#ifndef OPENWEATHERCLIENT_H
#define OPENWEATHERCLIENT_H

#include <string>
#include <httplib.h>

using namespace std;

struct Info {
    double temperature = 0;
    double min_temp = 0;
    double max_temp = 0;
    double windChill = 0;
    double windSpeed = 0;
    double humidity = 0;
    double visibility = 0; // in meters
    double clouds = 0;
};

struct City {
    string name;
    double lat = 0;
    double lon = 0;
    string country;
    string state;
    int resultsOmitted = -1;
    Info info;

    bool empty() const {
        return name.empty();
    }
};

class OpenWeatherClient {
    string api_key;
    httplib::Client client;

    City getCity(const string &query);
    void getCityInfo(City& city);

    public:
        OpenWeatherClient();

        City getCityByName(string cityName);
        City getCityByZipCode(string zipCode);
};



#endif //OPENWEATHERCLIENT_H
