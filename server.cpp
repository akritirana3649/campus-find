
#include "crow.h"
#include <mysql.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>

using namespace std;

MYSQL* db = nullptr;

string escapeSQL(const string& value) {
    char* buffer = new char[value.length() * 2 + 1];
    mysql_real_escape_string(db, buffer, value.c_str(),
                             static_cast<unsigned long>(value.length()));
    string result(buffer);
    delete[] buffer;
    return result;
}

string getValue(const crow::json::rvalue& data, const string& key) {
    if (!data.has(key)) return "";
    return data[key].s();
}

crow::response serveFile(const string& filename, const string& contentType) {
    ifstream file(filename, ios::binary);
    if (!file) return crow::response(404, "File not found");

    stringstream buffer;
    buffer << file.rdbuf();

    crow::response response(buffer.str());
    response.set_header("Content-Type", contentType);
    return response;
}

int main() {
    const char* password = getenv("CAMPUS_DB_PASSWORD");

    if (!password) {
        cerr << "Please set CAMPUS_DB_PASSWORD in the terminal." << endl;
        return 1;
    }

    db = mysql_init(nullptr);

    if (!mysql_real_connect(db, "127.0.0.1", "root", password,
                            "campusfind", 3306, nullptr, 0)) {
        cerr << "MySQL connection failed: "
             << mysql_error(db) << endl;
        mysql_close(db);
        return 1;
    }

    cout << "Connected to CampusFind database!" << endl;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([] {
        return serveFile("index.html", "text/html");
    });

    CROW_ROUTE(app, "/style.css")([] {
        return serveFile("style.css", "text/css");
    });

    CROW_ROUTE(app, "/script.js")([] {
        return serveFile("script.js", "application/javascript");
    });

    CROW_ROUTE(app, "/api/items").methods(crow::HTTPMethod::Get)([] {
        if (mysql_query(db,
            "SELECT item_id, item_name, item_type, category, location, "
            "item_date, reporter, description, status FROM Items ORDER BY item_id DESC")) {
            return crow::response(500, mysql_error(db));
        }

        MYSQL_RES* result = mysql_store_result(db);
        crow::json::wvalue output;
        output["items"] = crow::json::wvalue::list();

        MYSQL_ROW row;
        int index = 0;

        while ((row = mysql_fetch_row(result))) {
            auto& item = output["items"][index++];
            item["id"] = stoi(row[0]);
            item["name"] = row[1] ? row[1] : "";
            item["type"] = row[2] ? row[2] : "";
            item["category"] = row[3] ? row[3] : "";
            item["location"] = row[4] ? row[4] : "";
            item["date"] = row[5] ? row[5] : "";
            item["reporter"] = row[6] ? row[6] : "";
            item["description"] = row[7] ? row[7] : "";
            item["status"] = row[8] ? row[8] : "";
        }

        mysql_free_result(result);
        return crow::response(output);
    });

    CROW_ROUTE(app, "/api/items").methods("POST"_method)
    ([](const crow::request& req) {
        auto data = crow::json::load(req.body);
        if (!data) return crow::response(400, "Invalid JSON");

        string name = escapeSQL(getValue(data, "name"));
        string type = escapeSQL(getValue(data, "type"));
        string category = escapeSQL(getValue(data, "category"));
        string location = escapeSQL(getValue(data, "location"));
        string date = escapeSQL(getValue(data, "date"));
        string reporter = escapeSQL(getValue(data, "reporter"));
        string description = escapeSQL(getValue(data, "description"));

        if (name.empty() || location.empty() || reporter.empty() ||
            description.empty() || date.empty()) {
            return crow::response(400, "Please complete all required fields");
        }

        string query =
            "INSERT INTO Items (item_name, item_type, category, location, "
            "item_date, reporter, description, status) VALUES ('" +
            name + "','" + type + "','" + category + "','" + location +
            "','" + date + "','" + reporter + "','" + description +
            "','" + type + "')";

        if (mysql_query(db, query.c_str()))
            return crow::response(500, mysql_error(db));

        crow::json::wvalue response;
        response["message"] = "Report added successfully";
        response["id"] = static_cast<int>(mysql_insert_id(db));
        return crow::response(201, response);
    });

    CROW_ROUTE(app, "/api/items/<int>/returned").methods(crow::HTTPMethod::Post)
    ([](int id) {
        string query = "UPDATE Items SET status='Returned' WHERE item_id=" +
                       to_string(id);

        if (mysql_query(db, query.c_str()))
            return crow::response(500, mysql_error(db));

        return crow::response(200, "Item marked as returned");
    });

    CROW_ROUTE(app, "/api/items/<int>").methods(crow::HTTPMethod::Delete)
    ([](int id) {
        string query = "DELETE FROM Items WHERE item_id=" + to_string(id);

        if (mysql_query(db, query.c_str()))
            return crow::response(500, mysql_error(db));

        return crow::response(200, "Item deleted");
    });

    CROW_ROUTE(app, "/api/contact").methods("POST"_method)
    ([](const crow::request& req) {
        auto data = crow::json::load(req.body);
        if (!data) return crow::response(400, "Invalid JSON");

        string name = escapeSQL(getValue(data, "name"));
        string email = escapeSQL(getValue(data, "email"));
        string subject = escapeSQL(getValue(data, "subject"));
        string message = escapeSQL(getValue(data, "message"));

        if (name.empty() || email.empty() ||
            subject.empty() || message.empty()) {
            return crow::response(400, "Please complete all fields");
        }

        string query =
            "INSERT INTO ContactMessages (name, email, subject, message) "
            "VALUES ('" + name + "','" + email + "','" +
            subject + "','" + message + "')";

        if (mysql_query(db, query.c_str()))
            return crow::response(500, mysql_error(db));

        return crow::response(201, "Message saved successfully");
    });

    app.port(18080).multithreaded().run();

    mysql_close(db);
    return 0;
}
