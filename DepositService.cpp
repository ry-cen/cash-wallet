#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") {}

void DepositService::post(HTTPRequest *request, HTTPResponse *response)
{
    WwwFormEncodedDict args = request->formEncodedBody();
    string amountString = args.get("amount");
    string stripeToken = args.get("stripe_token");
    User *currentUser = getAuthenticatedUser(request);

    if (currentUser == nullptr)
    {

        response->setStatus(401);
        throw ClientError::unauthorized();
    }

    if (amountString == "" || stripeToken == "")
    {

        response->setStatus(400);
        throw ClientError::badRequest();
    }

    int amount;
    stringstream sstream;
    sstream << amountString;
    sstream >> amount;

    if(amount < 50) {

        response->setStatus(400);
        throw ClientError::badRequest();
    }

    HttpClient client("api.stripe.com", 443, true);
    client.set_basic_auth(m_db->stripe_secret_key, "");
    WwwFormEncodedDict body;
    body.set("amount", amount);
    body.set("currency", "usd");
    body.set("source", stripeToken);
    HTTPClientResponse *client_response = client.post("/v1/charges", body.encode());

    Document *d = client_response->jsonBody();
    string status;
    string chargeId;

    if (client_response->success())
    {
        status = (*d)["status"].GetString();
        chargeId = (*d)["id"].GetString();
    }
    else
    {
        response->setBody("");
        response->setStatus(400);
        ClientError::badRequest();
    }

    delete d;
    delete client_response;

    if (status == "succeeded")
    {
        currentUser->balance += amount;

        Deposit *deposit = new Deposit();
        deposit->to = currentUser;
        deposit->amount = amount;
        deposit->stripe_charge_id = chargeId;
        m_db->deposits.push_back(deposit);

        vector<Deposit> userDeposits;

        for (long unsigned int i = 0; i < m_db->deposits.size(); i++)
        {
            if (m_db->deposits[i]->to == currentUser)
            {
                userDeposits.push_back(*m_db->deposits[i]);
            }
        }

        response->setContentType("application/json");
        response->setBody(postResponseBody(currentUser->balance, userDeposits));
    }
    else
    {

        response->setBody("");
        response->setStatus(400);
        return;
    }
}

string DepositService::postResponseBody(int balance, vector<Deposit> deposits)
{

    Document document;
    Document::AllocatorType &a = document.GetAllocator();

    Value o;
    o.SetObject();

    o.AddMember("balance", balance, a);

    Value array;
    array.SetArray();

    int arraySize = deposits.size();
    for (int i = 0; i < arraySize; i++)
    {

        string to = deposits[i].to->username;
        int amount = deposits[i].amount;
        string stripe_charge_id = deposits[i].stripe_charge_id;

        Value deposit;
        deposit.SetObject();
        deposit.AddMember("to", to, a);
        deposit.AddMember("amount", amount, a);
        deposit.AddMember("stripe_charge_id", stripe_charge_id, a);

        array.PushBack(deposit, a);
    }

    o.AddMember("deposits", array, a);

    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    return (buffer.GetString() + string("\n"));
}
