#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") {}

void TransferService::post(HTTPRequest *request, HTTPResponse *response)
{
    WwwFormEncodedDict args = request->formEncodedBody();
    string amountString = args.get("amount");
    string to = args.get("to");
    User *currentUser = getAuthenticatedUser(request);
    User *toUser = m_db->users[to];

    if (currentUser == nullptr)
    {

        response->setStatus(401);
        throw ClientError::unauthorized();
    }

    if (toUser == nullptr)
    {

        response->setStatus(404);
        throw ClientError::notFound();
    }

    if (amountString == "" || to == "")
    {

        response->setStatus(400);
        throw ClientError::badRequest();
    }

    int amount;
    stringstream sstream;
    sstream << amountString;
    sstream >> amount;

    if (currentUser->balance >= amount)
    {

        currentUser->balance -= amount;
        toUser->balance += amount;

        Transfer *transfer = new Transfer();

        transfer->to = toUser;
        transfer->from = currentUser;
        transfer->amount = amount;

        m_db->transfers.push_back(transfer);

        vector<Transfer> userTransfers;

        for (long unsigned int i = 0; i < m_db->transfers.size(); i++)
        {
            if (m_db->transfers[i]->from == currentUser || m_db->transfers[i]->to == currentUser)
            {
                userTransfers.push_back(*m_db->transfers[i]);
            }
        }

        response->setContentType("application/json");
        response->setBody(postResponseBody(currentUser->balance, userTransfers));
    }
    else
    {

        response->setBody("");
        response->setStatus(400);
        ClientError::badRequest();
    }
}

string TransferService::postResponseBody(int balance, vector<Transfer> transfers)
{

    Document document;
    Document::AllocatorType &a = document.GetAllocator();

    Value o;
    o.SetObject();

    o.AddMember("balance", balance, a);

    Value array;
    array.SetArray();

    int arraySize = transfers.size();
    for (int i = 0; i < arraySize; i++)
    {

        string from = transfers[i].from->username;
        string to = transfers[i].to->username;
        int amount = transfers[i].amount;

        Value transfer;
        transfer.SetObject();

        transfer.AddMember("from", from, a);
        transfer.AddMember("to", to, a);
        transfer.AddMember("amount", amount, a);

        array.PushBack(transfer, a);
    }

    o.AddMember("transfers", array, a);

    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    return (buffer.GetString() + string("\n"));
}
