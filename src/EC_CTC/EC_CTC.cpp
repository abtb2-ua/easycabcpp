#include <Common.h>
#include <Ncurses/Common.h>
#include <Ncurses/Window.h>
#include <dotenv.h>
#include <httplib.h>
#include <iostream>
#include <ncurses.h>
#include <nlohmann/json.hpp>
#include "OpenWeatherClient.h"

using namespace httplib;

constexpr int width = 50;
constexpr int height = 20;
constexpr int hMargin = 4;
constexpr int vMargin = 2;
constexpr int headerWidth = 16;
constexpr int restWidth = width - hMargin * 2 - 5 - headerWidth;

bool isAlphaNumeric(int ch);
string toCapital(const string &str);

void showCityInfo(const Window &window, const City &city);

int main() {
    dotenv::init(dotenv::Preserve);
    OpenWeatherClient client;
    Server server;
    City city = client.getCityByName("sapporo");
    mutex mut;

    server.Get("/temp", [&mut, &city](const Request &, Response &res) {
        lock_guard lock(mut);
        res.set_content(to_string(city.info.temperature), "text/plain");
    });
    thread serverThread([&server]() { server.listen(dotenv::getenv("EC_CTC_IP", "localhost"), envInt("EC_CTC_PORT")); });


    const string formatStr = "{:_<" + to_string(width - hMargin * 2 - 5) + "}";
    const array<string, 2> options = {"Name:", "Zip code:"};
    array<string, 2> contents = {"", ""};

    initCurses();
    const int maxX = getmaxx(stdscr);
    const int maxY = getmaxy(stdscr);
    const Window window(height, width, maxY / 2 - height / 2, maxX / 2 - width / 2, vMargin, hMargin);
    window.setScroll(false);

    Menu menu;
    menu.setPrefix("[$] ");
    menu.addOption(fmt::format("{: <{}}{:_<{}}", options[0], headerWidth - 4, contents[0], restWidth));
    menu.addOption(fmt::format("{: <{}}{:_<{}}", options[1], headerWidth - 4, contents[1], restWidth));

    const auto refreshHoveredOption = [&]() {
        const int option = menu.getHovered();
        menu.setOption(option,
                       fmt::format("{: <{}}{:_<{}}", options[option], headerWidth - 4, contents[option], restWidth));
    };

    window.showBox();

    while (true) {
        window.clear();
        window.addMenu(menu);
        window.addLine("(Press enter to change the city)", A_DIM);
        window.addLine();
        window.addLine();
        showCityInfo(window, city);
        window.addLine();
        window.addLine("(Press Ctrl+C to exit)", A_DIM);
        window.show();
        const int ch = getch();

        if (ch == ctrl('c') || ch == ctrl('z')) {
            break;
        } else if (ch == KEY_DOWN) {
            menu.hoverNext();
        } else if (ch == KEY_UP) {
            menu.hoverPrevious();
        } else if (isAlphaNumeric(ch)) {
            contents[menu.getHovered()] += static_cast<char>(ch);
            refreshHoveredOption();
        } else if (ch == KEY_BACKSPACE) {
            if (contents[menu.getHovered()].empty())
                continue;
            contents[menu.getHovered()].pop_back();
            refreshHoveredOption();
        } else if (ch == '\n') {
            lock_guard lock(mut);
            if (menu.getHovered() == 0) {
                city = client.getCityByName(contents[0]);
            } else {
                city = client.getCityByZipCode(contents[1]);
            }
        }
    }

    endCurses();
}

bool isAlphaNumeric(const int ch) { return isalnum(ch) || ch == ',' || ch == ' '; }

void showCityInfo(const Window &window, const City &city) {
    window.addLine(format("{: <{}}{}", "City: ", headerWidth, toCapital(city.name)));
    window.addLine(format("{: <{}}{}", "Country: ", headerWidth, city.country));

    window.addString(format("{: <{}}{:3.2f} ", "Temperature: ", headerWidth, city.info.temperature), A_BOLD);
    window.addChar(ACS_DEGREE, A_BOLD);
    window.addLine("C", A_BOLD);

    window.addString(format("{: <{}}{:3.2f} ", "Min temp: ", headerWidth, city.info.min_temp));
    window.addChar(ACS_DEGREE);
    window.addLine("C");

    window.addString(format("{: <{}}{:3.2f} ", "Max temp: ", headerWidth, city.info.max_temp));
    window.addChar(ACS_DEGREE);
    window.addLine("C");

    window.addString(format("{: <{}}{:3.2f} ", "Wind chill: ", headerWidth, city.info.windChill));
    window.addChar(ACS_DEGREE);
    window.addLine("C");

    window.addLine(format("{: <{}}{:3.2f} m/s", "Wind speed: ", headerWidth, city.info.windSpeed));

    window.addLine(format("{: <{}}{:3.2f} %", "Humidity: ", headerWidth, city.info.humidity));

    window.addLine(format("{: <{}}{:3.2f} km", "Visibility: ", headerWidth, city.info.visibility / 1000));

    window.addLine(format("{: <{}}{:3.2f} %", "Clouds: ", headerWidth, city.info.clouds));
}

string toCapital(const string &str) {
    string _str = str;
    ranges::for_each(_str, [](char &c) { c = tolower(c); });
    _str[0] = toupper(_str[0]);
    return _str;
}
