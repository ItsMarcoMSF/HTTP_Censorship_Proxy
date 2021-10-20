// cenProxy.h
// CPSC 441 FALL 2021
// Written by: Viet An Truong. UCID: 30090441

#ifndef PROXY_H
#define PROXY_H

#include <string>

char* getIP(const char*);
/*
 *
 */

std::string
forward(const char* host, char* request);
/*
 *
 */

int
sendRequest(const int socket_desc, char* request);
/*
 *
 */

std::string
receiveResponse(const int socket_desc);
/*
 *
 */

void 
catcher(int sig);
/*
 *
 */

char*
censorshipCheck(const char* host);
/*
 *
 */

#endif
