#ifndef _AUTHSERVICE_H_
#define _AUTHSERVICE_H_

#include "HttpService.h"

#include <string>

class AuthService : public HttpService {
 public:
  AuthService();

  virtual void post(HTTPRequest *request, HTTPResponse *response);
  virtual void del(HTTPRequest *request, HTTPResponse *response);

  std::string postResponseBody(std::string authToken, std::string userId);
};

#endif
