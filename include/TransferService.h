#ifndef _TRANSFERSERVICE_H_
#define _TRANSFERSERVICE_H_

#include "HttpService.h"
#include "Database.h"

#include <string>

class TransferService : public HttpService {
 public:
  TransferService();
  virtual void post(HTTPRequest *request, HTTPResponse *response);

  std::string postResponseBody(int balance, std::vector<Transfer> transfers);
};

#endif
