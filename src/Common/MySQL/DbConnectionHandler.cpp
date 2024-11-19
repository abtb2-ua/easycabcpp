//
// Created by ab-flies on 15/11/24.
//

#include "Common.h"
#include "DbConnectionHandler.h"
#include <dotenv.h>
#include <vector>
#include <optional>
#include <expected>

bool DbResults::nextResult() {
    if (currentResult + 1 >= results.size()) {
        return false;
    }

    currentResult++;
    currentRow = 0;
    return true;
}

optional<vector<string> > DbResults::getRow() {
    if (currentRow >= results[currentResult].size()) {
        return nullopt;
    }

    return results[currentResult][currentRow++];
}

DbConnectionHandler::DbConnectionHandler(): connection(nullptr) {
    mysql_library_init(0, nullptr, nullptr);
}

DbConnectionHandler::~DbConnectionHandler() {
    if (connection) {
        mysql_close(connection);
        mysql_library_end();
    }
}

DbConnectionHandler::DbConnectionHandler(DbConnectionHandler &&other) noexcept: connection(other.connection) {
    other.connection = nullptr;
}

DbConnectionHandler &DbConnectionHandler::operator=(DbConnectionHandler &&other) noexcept {
    if (this == &other) { return *this; }

    if (connection) {
        mysql_close(connection);
    }

    connection = other.connection;
    other.connection = nullptr;

    return *this;
}

bool DbConnectionHandler::connect() {
    connection = mysql_init(nullptr);
    if (!connection) {
        return false;
    }

    const Address database(dotenv::getenv("MYSQL_SERVER", "127.0.0.1:3306"));
    const string DB_NAME = dotenv::getenv("MYSQL_DB_NAME", "db");
    const string DB_PASSWORD = dotenv::getenv("MYSQL_PASSWORD", "1234");
    const string DB_USER = dotenv::getenv("MYSQL_USER", "root");

    if (!mysql_real_connect(this->connection, database.getIp().c_str(), DB_USER.c_str(), DB_PASSWORD.c_str(),
                            DB_NAME.c_str(), database.getPort(), nullptr, CLIENT_MULTI_STATEMENTS)) {
        mysql_close(connection);
        connection = nullptr;
        return false;
    }

    return true;
}

optional<string> DbConnectionHandler::checkQueryStatus() const {
    MYSQL_RES *err = mysql_store_result(connection);

    if (!err) {
        return {};
    }

    const auto row = mysql_fetch_row(err);
    mysql_next_result(connection);
    mysql_free_result(err);

    if (row[0] != nullptr) {
        return row[0];
    }

    return {};
}

expected<DbResults, string> DbConnectionHandler::query(const string &query, const bool _checkQueryStatus) const {
    if (!connection) {
        return unexpected("Connection not initialized");
    }

    if (mysql_query(connection, query.c_str())) {
        return unexpected(mysql_error(connection));
    }

    if (_checkQueryStatus) {
        auto err = checkQueryStatus();
        if (err.has_value()) {
            return unexpected(err.value());
        }
    }

    // Check for results
    DbResults results;

    do {
        MYSQL_RES *mysql_res = mysql_store_result(connection);

        if (!mysql_res) {
            continue;
        }

        results.newResult();

        while (const auto mysql_row = mysql_fetch_row(mysql_res)) {
            vector<string> row;
            for (unsigned int i = 0; i < mysql_num_fields(mysql_res); i++) {
                row.emplace_back(mysql_row[i]);
            }
            results.addRow(row);
        }

        mysql_free_result(mysql_res);
    } while (!mysql_next_result(connection));

    return results;
}