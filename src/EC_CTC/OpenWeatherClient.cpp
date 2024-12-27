//
// Created by ab-flies on 24/12/24.
//

#include <dotenv.h>
#include <nlohmann/json.hpp>
#include <format>

#include "OpenWeatherClient.h"

using json = nlohmann::json;

OpenWeatherClient::OpenWeatherClient():
    api_key(dotenv::getenv("OPENWEATHER_API_KEY", "")),
    client(httplib::Client("http://api.openweathermap.org")) {}

void OpenWeatherClient::getCityInfo(City &city) {
    const string query = format("/data/2.5/weather?lat={}&lon={}&appid={}&lang=ES&units=metric", city.lat, city.lon, api_key);
    auto res = client.Get(query);

    auto j = json::parse(res->body);
    if (j.empty()) return;

    city.info.temperature = j["main"]["temp"];
    city.info.min_temp = j["main"]["temp_min"];
    city.info.max_temp = j["main"]["temp_max"];
    city.info.humidity = j["main"]["humidity"];
    city.info.windSpeed = j["wind"]["speed"];
    city.info.windChill = j["wind"]["deg"];
    city.info.clouds = j["clouds"]["all"];
}


City OpenWeatherClient::getCity(const string &query) {
    auto res = client.Get(query);

    City city;

    auto j = json::parse(res->body);
    if (j.empty()) return city;

    nlohmann::basic_json<>* first;
    if (j.is_array()) {
        first = &j[0];
        ranges::for_each(j, [&city](auto&) { city.resultsOmitted++; });
    } else if (j.contains("name")) {
        first = &j;
        city.resultsOmitted = 0;
    } else {
        return city;
    }

    city.name = (*first)["name"];
    city.lat = (*first)["lat"];
    city.lon = (*first)["lon"];
    city.country = (*first)["country"];
    city.state = first->contains("state") ? (*first)["state"] : " - ";

    getCityInfo(city);
    return city;
}

City OpenWeatherClient::getCityByName(string cityName) {
    const string params = std::format("q={}&appid={}", cityName, api_key);
    const string query = std::format("/geo/1.0/direct?{}", params);

    return getCity(query);
}

City OpenWeatherClient::getCityByZipCode(string zipCode) {
    const string params = format("zip={}&appid={}", zipCode, api_key);
    const string query = format("/geo/1.0/zip?{}", params);

    return getCity(query);
}