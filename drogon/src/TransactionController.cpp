#include "TransactionController.h"
#include <drogon/drogon.h>

void TransactionController::getBalance(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string user_id = req->getAttributes()->get<std::string>("user_id");

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT "
        "  COALESCE(SUM(CASE WHEN receiver_id = $1::uuid THEN amount ELSE 0 END), 0) - "
        "  COALESCE(SUM(CASE WHEN sender_id = $1::uuid THEN amount ELSE 0 END), 0) "
        "  AS balance "
        "FROM transactions WHERE receiver_id = $1::uuid OR sender_id = $1::uuid;",
        [callback](const drogon::orm::Result &result) {
            Json::Value ret;
            ret["status"] = "success";
            ret["balance"] = result[0]["balance"].as<std::string>();
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

void TransactionController::getTransactions(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string user_id = req->getAttributes()->get<std::string>("user_id");

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT t.id, t.amount, t.created_at, "
        "  CASE WHEN t.sender_id = $1::uuid THEN 'sent' ELSE 'received' END as type, "
        "  CASE WHEN t.sender_id = $1::uuid THEN u_recv.email ELSE u_send.email END as counterparty "
        "FROM transactions t "
        "LEFT JOIN users u_send ON t.sender_id = u_send.id "
        "LEFT JOIN users u_recv ON t.receiver_id = u_recv.id "
        "WHERE t.sender_id = $1::uuid OR t.receiver_id = $1::uuid "
        "ORDER BY t.created_at DESC LIMIT 50;",
        [callback](const drogon::orm::Result &result) {
            Json::Value ret;
            ret["status"] = "success";
            Json::Value transactions(Json::arrayValue);
            for (auto row : result) {
                Json::Value t;
                t["id"] = row["id"].as<std::string>();
                t["amount"] = row["amount"].as<std::string>();
                t["created_at"] = row["created_at"].as<std::string>();
                t["type"] = row["type"].as<std::string>();
                t["counterparty"] = row["counterparty"].isNull() ? "System" : row["counterparty"].as<std::string>();
                transactions.append(t);
            }
            ret["transactions"] = transactions;
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

void TransactionController::transfer(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string sender_id = req->getAttributes()->get<std::string>("user_id");

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("email") || !jsonPtr->isMember("amount")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing email or amount");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string receiver_email = (*jsonPtr)["email"].asString();
    double amount = 0;
    try {
        amount = std::stod((*jsonPtr)["amount"].asString());
    } catch (...) {
        if ((*jsonPtr)["amount"].isNumeric()) {
             amount = (*jsonPtr)["amount"].asDouble();
        } else {
             auto resp = HttpResponse::newHttpResponse();
             resp->setStatusCode(k400BadRequest);
             resp->setBody("Invalid amount format");
             resp->addHeader("Access-Control-Allow-Origin", "*");
             callback(resp);
             return;
        }
    }

    if (amount <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Amount must be greater than 0");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    
    // Check balance and execute transfer in a transaction block
    // Since Drogon Async doesn't support complex transactions easily in raw callbacks, 
    // we use a CTE or a PL/pgSQL block to ensure atomic execution.
    std::string sql = R"(
        WITH lock AS (
            SELECT pg_advisory_xact_lock(hashtext($1::text))
        ),
        sender_balance AS (
            SELECT COALESCE(SUM(CASE WHEN receiver_id = $1::uuid THEN amount ELSE 0 END), 0) -
                   COALESCE(SUM(CASE WHEN sender_id = $1::uuid THEN amount ELSE 0 END), 0) as balance
            FROM transactions WHERE receiver_id = $1::uuid OR sender_id = $1::uuid
        ),
        receiver AS (
            SELECT id FROM users WHERE email = $2
        ),
        inserted_tx AS (
            INSERT INTO transactions (sender_id, receiver_id, amount)
            SELECT $1::uuid, (SELECT id FROM receiver), $3
            WHERE (SELECT balance FROM sender_balance) >= $3 AND EXISTS (SELECT id FROM receiver)
            RETURNING id
        )
        SELECT 
            (SELECT balance FROM sender_balance) as old_balance,
            (SELECT id FROM receiver) as recv_id,
            (SELECT id FROM inserted_tx) as tx_id,
            (SELECT * FROM lock) as lock_dummy;
    )";

    dbClient->execSqlAsync(
        sql,
        [callback, amount, receiver_email, sender_id](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }
            
            if (result[0]["recv_id"].isNull()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Receiver email not found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            if (result[0]["tx_id"].isNull()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Insufficient funds");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            Json::Value ret;
            ret["status"] = "success";
            ret["tx_id"] = result[0]["tx_id"].as<std::string>();
            
            LOG_INFO << "Transaction executed: $" << amount << " sent to " << receiver_email << " from user_id: " << sender_id;

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error during transfer.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        sender_id, receiver_email, amount
    );
}

void TransactionController::demoFunds(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string receiver_id = req->getAttributes()->get<std::string>("user_id");
    
    // Inject 100 Demo Funds (null sender)
    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "INSERT INTO transactions (sender_id, receiver_id, amount) VALUES (NULL, $1, 100.00) RETURNING id;",
        [callback, receiver_id](const drogon::orm::Result &result) {
            Json::Value ret;
            ret["status"] = "success";
            ret["tx_id"] = result[0]["id"].as<std::string>();
            
            LOG_INFO << "Demo funds ($100) minted for user ID: " << receiver_id;

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error adding funds.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        receiver_id
    );
}
