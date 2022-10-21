#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"
#include "StringUtils.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;

void takeInput();
void runCommand(string input);

void auth(vector<string> parsedArgs);
void balance();
void deposit(vector<string> parsedArgs);
void send(vector<string> parsedArgs);
void logout();

void printBalance(HTTPClientResponse *client_response);
int convertAmount(string amountString);
void remove_carriage_return(std::string &line);
vector<string> parseInput(const string input, const char parseAt);

int main(int argc, char *argv[])
{
    stringstream config;
    int fd = open("config.json", O_RDONLY);
    if (fd < 0)
    {
        cout << "could not open config.json" << endl;
        exit(1);
    }
    int ret;
    char buffer[4096];
    while ((ret = read(fd, buffer, sizeof(buffer))) > 0)
    {
        config << string(buffer, ret);
    }
    Document d;
    d.Parse(config.str());
    API_SERVER_PORT = d["api_server_port"].GetInt();
    API_SERVER_HOST = d["api_server_host"].GetString();
    PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

    if (argc == 1)
    {
        while (true)
        {
            takeInput();
        }
    }
    else if (argc == 2)
    {
        string input;
        fstream file;
        file.open(argv[1]);

        if (!file)
        {
            cout << "Error";
            exit(1);
        }

        while (getline(file, input))
        {
            remove_carriage_return(input);
            runCommand(input);
        }

        file.close();
        exit(0);
    }
    else
    {
        cout << "Error";
        exit(1);
    }

    return 0;
}

void takeInput()
{
    string input;
    cout << "D$> ";
    getline(cin, input);
    runCommand(input);
}

void runCommand(string input)
{
    vector<string> parsedArgs = StringUtils::split(input, ' ');

    if (parsedArgs.size() == 0)
    {
        cout << "Error" << endl;
        return;
    }
    string command = parsedArgs[0];
    if (command.compare("auth") == 0 && (parsedArgs.size() == 4 || parsedArgs.size() == 3))
    {
        auth(parsedArgs);
    }
    else if (command.compare("balance") == 0 && parsedArgs.size() == 1)
    {
        balance();
    }
    else if (command.compare("deposit") == 0 && parsedArgs.size() == 6)
    {
        deposit(parsedArgs);
    }
    else if (command.compare("send") == 0 && parsedArgs.size() == 3)
    {
        send(parsedArgs);
    }
    else if (command.compare("logout") == 0 && parsedArgs.size() == 1)
    {
        logout();
    }
    else
    {
        cout << "Error" << endl;
    }
}

void auth(vector<string> parsedArgs)
{
    HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
    WwwFormEncodedDict body1;
    body1.set("username", parsedArgs[1]);
    body1.set("password", parsedArgs[2]);
    HTTPClientResponse *client_response = client.post("/auth-tokens", body1.encode());

    string new_auth_token;
    string new_user_id;

    if (client_response->success())
    {
        Document *d = client_response->jsonBody();
        new_auth_token = (*d)["auth_token"].GetString();
        new_user_id = (*d)["user_id"].GetString();
        delete d;
    }
    else
    {
        cout << "Error" << endl;
        return;
    }

    if (!auth_token.empty())
    {
        HttpClient client2(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
        client2.set_header("x-auth-token", auth_token);
        client2.del("/auth-tokens/" + auth_token);
    }

    auth_token = new_auth_token;
    user_id = new_user_id;

    if (parsedArgs.size() == 3)
    {

        HttpClient client3(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
        client3.set_header("x-auth-token", auth_token);
        client_response = client3.get("/users/" + user_id);
    }
    else
    {

        HttpClient client3(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
        WwwFormEncodedDict body2;
        body2.set("email", parsedArgs[3]);
        client3.set_header("x-auth-token", auth_token);
        client_response = client3.put("/users/" + user_id, body2.encode());
    }

    printBalance(client_response);
    delete client_response;
}

void balance()
{
    HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
    client.set_header("x-auth-token", auth_token);
    HTTPClientResponse *client_response = client.get("/users/" + user_id);

    printBalance(client_response);
    delete client_response;
}

void deposit(vector<string> parsedArgs)
{
    HttpClient stripeClient("api.stripe.com", 443, true);
    stripeClient.set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);

    WwwFormEncodedDict body1;
    body1.set("card[number]", parsedArgs[2]);
    body1.set("card[exp_month]", parsedArgs[4]);
    body1.set("card[exp_year]", parsedArgs[3]);
    body1.set("card[cvc]", parsedArgs[5]);
    HTTPClientResponse *client_response = stripeClient.post("/v1/tokens", body1.encode());
    string token;

    if (client_response->success())
    {
        Document *d = client_response->jsonBody();
        token = (*d)["id"].GetString();
        delete d;
    }
    else
    {
        cout << "Error" << endl;
        return;
    }
    delete client_response;

    HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

    WwwFormEncodedDict body2;
    body2.set("amount", convertAmount(parsedArgs[1]));
    body2.set("stripe_token", token);
    client.set_header("x-auth-token", auth_token);
    client_response = client.post("/deposits", body2.encode());

    printBalance(client_response);
    delete client_response;
}

void send(vector<string> parsedArgs)
{
    HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

    WwwFormEncodedDict body;
    body.set("to", parsedArgs[1]);
    body.set("amount", convertAmount(parsedArgs[2]));

    client.set_header("x-auth-token", auth_token);
    HTTPClientResponse *client_response = client.post("/transfers", body.encode());

    printBalance(client_response);
    delete client_response;
}

void logout()
{
    HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);
    client.set_header("x-auth-token", auth_token);
    client.del("/auth-tokens/" + auth_token);
    exit(0);
}

void printBalance(HTTPClientResponse *client_response)
{

    if (client_response->success())
    {
        Document *d = client_response->jsonBody();
        double balance = (*d)["balance"].GetDouble();
        cout << "Balance: $" << setprecision(2) << fixed << balance / 100 << endl;
        delete d;
    }
    else
    {
        cout << "Error" << endl;
        return;
    }
}

void remove_carriage_return(std::string &line)
{
    if (*line.rbegin() == '\r')
    {
        line.erase(line.length() - 1);
    }
}

int convertAmount(string amountString)
{
    double amountDouble;
    stringstream ss;
    ss << amountString;
    ss >> amountDouble;
    int amount = amountDouble * 100;

    return amount;
}
