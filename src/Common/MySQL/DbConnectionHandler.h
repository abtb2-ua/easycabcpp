//
// Created by ab-flies on 15/11/24.
//

#ifndef DB_CONNECTION_Handler_H
#define DB_CONNECTION_Handler_H

#include <mysql/mysql.h>
#include <expected>
#include <vector>
#include <optional>
#include <string>


using namespace std;

class DbConnectionHandler;


class DbResults {
    friend class DbConnectionHandler;

    vector<vector<vector<string> > > results;
    size_t currentResult = 0;
    size_t currentRow = 0;

    DbResults() = default;

    void newResult() {
        results.emplace_back();
    }

    void addRow(vector<string> &row) {
        results.back().emplace_back(row);
    }

public:
    bool nextResult();

    /// @brief Gets the next row of the current result.
    /// @note The internal index is incremented. Consecutive calls may not return the same value.
    /// @return The current row or nullopt if there are no more rows
    [[nodiscard]] optional<vector<string> > getRow();
};

class DbConnectionHandler {
    MYSQL *connection;

public:
    // Canonical form;
    DbConnectionHandler();

    ~DbConnectionHandler();

    DbConnectionHandler(const DbConnectionHandler &other) = delete;

    DbConnectionHandler(DbConnectionHandler &&other) noexcept;

    DbConnectionHandler &operator=(const DbConnectionHandler &other) = delete;

    DbConnectionHandler &operator=(DbConnectionHandler &&other) noexcept;

    // Methods
    bool connect();

    [[nodiscard]] optional<string> checkQueryStatus() const;

    [[nodiscard]] expected<DbResults, string> query(const string &query, bool _checkQueryStatus = false) const;
};


#endif //DB_CONNECTION_Handler_H
