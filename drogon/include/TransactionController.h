#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class TransactionController : public drogon::HttpController<TransactionController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TransactionController::getBalance, "/api/balance", Get, Options, "JwtFilter");
    ADD_METHOD_TO(TransactionController::getTransactions, "/api/transactions", Get, Options, "JwtFilter");
    ADD_METHOD_TO(TransactionController::transfer, "/api/transfer", Post, Options, "JwtFilter");
    ADD_METHOD_TO(TransactionController::demoFunds, "/api/demo-funds", Post, Options, "JwtFilter");
    METHOD_LIST_END

    void getBalance(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void getTransactions(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void transfer(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void demoFunds(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
};
