# Cash Wallet
Assignment for ECS 150 Operating Systems

Back end for a peer to peer payment system that allows for simultaneous transactions. 
Basic single threaded http server was provided as template code.

# Gunrock Web Server
This web server is a simple server used in ECS 150 for teaching about multi-threaded programming and operating systems. This version of the server can only handle one client at a time and simply serves static files. Also, it will close each connection after reading the request and responding, but generally is still HTTP 1.1 compliant.

This server was written by Sam King from UC Davis and is actively maintained by Sam as well. The `http_parse.c` file was written by [Ryan Dahl](https://github.com/ry) and is licensed under the BSD license by Ryan. This programming assignment is from the [OSTEP](http://ostep.org) textbook (tip of the hat to the authors for writing an amazing textbook).
