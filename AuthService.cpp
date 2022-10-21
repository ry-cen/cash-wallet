#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens")
{
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response)
{
    WwwFormEncodedDict args = request->formEncodedBody();
    string username = "";
    string password = "";

    username = args.get("username");
    password = args.get("password");

    if (username == "" || password == "")
    {
        response->setStatus(400);
        throw ClientError::badRequest();
    }

    long unsigned int count = 0;
    for(long unsigned int i = 0; i < username.length(); i++) {
        if (islower(username[i])) {
            count++;
        }
    }

    if(count != username.length()) 
    {
        response->setStatus(400);
        throw ClientError::badRequest();
    }
    

    if (m_db->users.find(username) != m_db->users.end())
    {

        if (m_db->users[username]->password == password)
        {

            string authToken = StringUtils::createAuthToken();
            string userId = m_db->users[username]->user_id;

            m_db->auth_tokens[authToken] = m_db->users[username];

            response->setContentType("application/json");
            response->setBody(postResponseBody(authToken, userId));
        }
        else
        {

            response->setStatus(403);
            throw ClientError::forbidden();
        }
    }
    else
    {

        string authToken = StringUtils::createAuthToken();
        string userId = StringUtils::createUserId();

        User *newUser = new User();
        newUser->username = username;
        newUser->password = password;
        newUser->user_id = userId;
        newUser->balance = 0;

        m_db->users[username] = newUser;
        m_db->auth_tokens[authToken] = newUser;

        response->setContentType("application/json");
        response->setBody(postResponseBody(authToken, userId));
        response->setStatus(201);
    }
}

string AuthService::postResponseBody(string authToken, string userId)
{
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();

    o.AddMember("auth_token", authToken, a);
    o.AddMember("user_id", userId, a);

    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    return (buffer.GetString() + string("\n"));
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response)
{

    string authToken = request->getPathComponents()[1];
    User *currentUser = getAuthenticatedUser(request);

    try
    {

        m_db->auth_tokens[authToken];
    }
    catch (...)
    {

        response->setStatus(404);
        throw ClientError::notFound();
    }

    if (currentUser == nullptr)
    {

        response->setStatus(401);
        throw ClientError::unauthorized();
    }

    if (currentUser == m_db->auth_tokens[authToken])
    {

        m_db->auth_tokens.erase(authToken);
    }
    else
    {

        response->setStatus(403);
        throw ClientError::forbidden();
    }
}
