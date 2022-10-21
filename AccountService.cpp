#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users")
{
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response)
{
    vector<string> pathComponents = request->getPathComponents();

    if (pathComponents.size() < 2)
    {

        response->setStatus(400);
        throw ClientError::badRequest();
    }

    string userId = request->getPathComponents()[1];
    User *currentUser = getAuthenticatedUser(request);

    if (currentUser == nullptr)
    {

        response->setStatus(401);
        throw ClientError::unauthorized();
    }

    if (userId == currentUser->user_id)
    {

        int balance = currentUser->balance;
        string email = currentUser->email;

        response->setContentType("application/json");
        response->setBody(responseBody(email, balance));
    }
    else
    {

        response->setStatus(403);
        throw ClientError::forbidden();
    }
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response)
{
    string userId = request->getPathComponents()[1];
    WwwFormEncodedDict args = request->formEncodedBody();
    User *currentUser = getAuthenticatedUser(request);

    if (currentUser == nullptr)
    {

        response->setStatus(401);
        throw ClientError::unauthorized();
    }

    if (args.get("email") == "")
    {

        response->setStatus(400);
        throw ClientError::badRequest();
    }

    if (userId == currentUser->user_id)
    {

        int balance = currentUser->balance;
        string email = args.get("email");

        currentUser->email = email;
        response->setContentType("application/json");
        response->setBody(responseBody(email, balance));
    }
    else
    {

        response->setStatus(403);
        throw ClientError::forbidden();
    }
}

string AccountService::responseBody(string email, int balance)
{
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();

    o.AddMember("balance", balance, a);
    o.AddMember("email", email, a);

    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    return (buffer.GetString() + string("\n"));
}