/// @file test2.cpp
/// @brief Demonstrate reading and writing a reflected business object with SQLite.
/// Compile with: cc -c 3rdparty/sqlite3.c -o 3rdparty/sqlite3.o && g++ -std=c++17 -I../include -I3rdparty test2.cpp 3rdparty/sqlite3.o -o test2
/// g++ -std=c++17 -I../include -I3rdparty test2.cpp 3rdparty/sqlite3.o -o bin/test2

#include "../include/pfr.hpp"
#include "3rdparty/sqlite3.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace db {

sqlite3* connection = nullptr;


void check(int result, const char* operation)
{
    if (result != SQLITE_OK && result != SQLITE_DONE && result != SQLITE_ROW) {
        throw std::runtime_error(std::string(operation) + ": " + sqlite3_errmsg(connection));
    }
}

void execute(std::string_view sql)
{
    char* error_message = nullptr;
    const int result = sqlite3_exec(connection, std::string(sql).c_str(), nullptr, nullptr, &error_message);
    if (result != SQLITE_OK) {
        const std::string message = error_message != nullptr ? error_message : "unknown SQLite error";
        sqlite3_free(error_message);
        throw std::runtime_error(std::string("SQLite execute failed: ") + message);
    }
}

template <typename Value>
void bind(sqlite3_stmt* statement, int index, const Value& value);

template <>
void bind(sqlite3_stmt* statement, int index, const int& value)
{
    check(sqlite3_bind_int(statement, index, value), "bind int");
}

template <>
void bind(sqlite3_stmt* statement, int index, const unsigned int& value)
{
    check(sqlite3_bind_int64(statement, index, static_cast<sqlite3_int64>(value)), "bind unsigned int");
}

template <>
void bind(sqlite3_stmt* statement, int index, const std::string& value)
{
    check(sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT), "bind string");
}

template <>
void bind(sqlite3_stmt* statement, int index, const std::string_view& value)
{
    check(sqlite3_bind_text(statement, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT), "bind string view");
}

template <typename... Values>
void insert(std::string_view sql, const Values&... values)
{
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(connection, std::string(sql).c_str(), -1, &statement, nullptr), "prepare insert");
    int index = 1;
    (bind(statement, index++, values), ...);
    check(sqlite3_step(statement), "execute insert");
    sqlite3_finalize(statement);
}

template <typename Record>
void insert(std::string_view sql, const Record& record)
{
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(connection, std::string(sql).c_str(), -1, &statement, nullptr), "prepare record insert");
    int index = 1;
    pfr::for_each_field(record, [&](const auto& field) {
        bind(statement, index++, field);
    });
    check(sqlite3_step(statement), "execute record insert");
    sqlite3_finalize(statement);
}

template <typename Type>
Type read_column(sqlite3_stmt* statement, int column)
{
    if constexpr (std::is_same_v<Type, std::string>) {
        const auto* text = sqlite3_column_text(statement, column);
        return text == nullptr ? std::string{} : reinterpret_cast<const char*>(text);
    } else if constexpr (std::is_same_v<Type, unsigned int>) {
        return static_cast<unsigned int>(sqlite3_column_int64(statement, column));
    } else {
        return static_cast<Type>(sqlite3_column_int64(statement, column));
    }
}

template <typename... Types, std::size_t... Indices>
std::tuple<Types...> read_row(sqlite3_stmt* statement, std::index_sequence<Indices...>)
{
    return std::tuple<Types...>{read_column<Types>(statement, static_cast<int>(Indices))...};
}

template <typename... Types, typename... Parameters>
std::tuple<Types...> one_row_as_tuple(std::string_view sql, const Parameters&... parameters)
{
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(connection, std::string(sql).c_str(), -1, &statement, nullptr), "prepare select");
    int index = 1;
    (bind(statement, index++, parameters), ...);
    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        throw std::runtime_error("select returned no rows");
    }

    auto result = read_row<Types...>(statement, std::index_sequence_for<Types...>{});
    sqlite3_finalize(statement);
    return result;
}

template <typename Record, typename... Parameters>
Record one_row_as(std::string_view sql, const Parameters&... parameters)
{
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(connection, std::string(sql).c_str(), -1, &statement, nullptr), "prepare record select");
    int index = 1;
    (bind(statement, index++, parameters), ...);
    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        throw std::runtime_error("select returned no rows");
    }

    Record record{};
    int column = 0;
    pfr::for_each_field(record, [&](auto& field) {
        using field_type = std::remove_cv_t<std::remove_reference_t<decltype(field)>>;
        field = read_column<field_type>(statement, column++);
    });
    sqlite3_finalize(statement);
    return record;
}

} // namespace db

typedef struct user_info_s
{
    std::string     name;
    unsigned int    birth_year;
    std::string     email;
    std::string     gender;
    std::string     address;
    int             user_id;
    std::string     login_token;
} user_info_t;



user_info_t retrieve_user_info_by_id(int id)
{
    std::tuple user_info_tuple
        = db::one_row_as_tuple<std::string, unsigned int, std::string, std::string, std::string, int, std::string>(
            "SELECT name, birth_year, email, gender, address, user_id, login_token FROM user_infos WHERE user_id=$0",
            id
    );

    ////////////////////////////////////////////////////////////////////////////////
    user_info_t user_info {
        std::move(std::get<0>(user_info_tuple)),
        std::move(std::get<1>(user_info_tuple)),
        std::move(std::get<2>(user_info_tuple)),
        std::move(std::get<3>(user_info_tuple)),
        std::move(std::get<4>(user_info_tuple)),
        std::move(std::get<5>(user_info_tuple)),
        std::move(std::get<6>(user_info_tuple))
    };
    ////////////////////////////////////////////////////////////////////////////////**
    
    return user_info;
}

user_info_t retrieve_user_info_by_id_pfr(int id)
{
    return db::one_row_as<user_info_t>(
        "SELECT name, birth_year, email, gender, address, user_id, login_token FROM user_infos WHERE user_id=$1",
        id
    );
}

user_info_t return_user_info_unchanged(user_info_t user_info)
{
    return user_info;
}

int main()
{
    // Initialize SQLite in-memory database
    if (sqlite3_open(":memory:", &db::connection) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db::connection) << '\n';
        return 1;
    }

    try {
        // Create a table for user information
        db::execute("CREATE TABLE user_infos ("
                    "user_id INTEGER PRIMARY KEY, name TEXT NOT NULL, birth_year INTEGER NOT NULL, "
                    "email TEXT NOT NULL, gender TEXT NOT NULL, address TEXT NOT NULL, login_token TEXT NOT NULL)");
        db::execute("CREATE TABLE user_infos_pfr AS SELECT * FROM user_infos WHERE 0");

        const std::array<user_info_t, 15> users{{
            {"hesphoros", 2008, "hesphoros@example.com", "male", "123 Main St", 1, "token001"},
            {"alice", 1995, "alice@example.com", "female", "45 Oak Ave", 2, "token002"},
            {"bob", 1992, "bob@example.com", "male", "78 Pine Road", 3, "token003"},
            {"carol", 1988, "carol@example.com", "female", "9 River Lane", 4, "token004"},
            {"david", 2001, "david@example.com", "male", "16 Hill Street", 5, "token005"},
            {"eva", 1999, "eva@example.com", "female", "27 Lake Drive", 6, "token006"},
            {"frank", 1985, "frank@example.com", "male", "34 Cedar Way", 7, "token007"},
            {"grace", 1997, "grace@example.com", "female", "52 Maple Court", 8, "token008"},
            {"henry", 1990, "henry@example.com", "male", "63 Forest Road", 9, "token009"},
            {"iris", 2003, "iris@example.com", "female", "71 Garden Street", 10, "token010"},
            {"jack", 1982, "jack@example.com", "male", "84 Sunset Blvd", 11, "token011"},
            {"kate", 1994, "kate@example.com", "female", "96 Spring Avenue", 12, "token012"},
            {"leo", 2000, "leo@example.com", "male", "105 Harbor Road", 13, "token013"},
            {"mia", 1998, "mia@example.com", "female", "118 Willow Lane", 14, "token014"},
            {"nick", 1987, "nick@example.com", "male", "129 Elm Street", 15, "token015"}
        }};

        for (const auto& user_info : users) {
            db::insert("INSERT INTO user_infos(user_id, name, birth_year, email, gender, address, login_token) "
                       "VALUES ($1, $2, $3, $4, $5, $6, $7)",
                       user_info.user_id, user_info.name, user_info.birth_year, user_info.email,
                       user_info.gender, user_info.address, user_info.login_token);
        }

        const auto& user_info = users.front();

        const auto normal_result = retrieve_user_info_by_id(user_info.user_id);
        const auto pfr_result = return_user_info_unchanged(retrieve_user_info_by_id_pfr(user_info.user_id));
        db::insert("INSERT INTO user_infos_pfr(user_id, name, birth_year, email, gender, address, login_token) "
               "VALUES ($1, $2, $3, $4, $5, $6, $7)", pfr_result);
        std::cout << "normal: " << pfr::io(normal_result) << '\n';
        std::cout << "pfr: " << pfr::io(pfr_result) << '\n';
    } catch (const std::exception& error) 
    {
        std::cerr << error.what() << '\n';
        sqlite3_close(db::connection);
        return 1;
    }

    sqlite3_close(db::connection);
    return 0;
}