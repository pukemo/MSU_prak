#pragma once
#include "HttpHeader.h"
#include <iostream>
#include <string.h>

class Code {
public:
	int code;
	string comment;
	HttpHeader** headers;
	int num_headers;

	Code(int c);
	Code(Code& c);
	friend void AddHeader(Code code, HttpHeader newh);
	friend ostream& operator<<(std::ostream& os, const Code& c);//code: comment\nheaders
	~Code();
};

class Param {
public:
	string name;
	string value;
	Param(string name, string value);
};

//Code CodeSearch(Code** codes, int code);

class HttpRequest {
public:
	//HttpHeader** header;
	int method;// !=0 -> 501, =0 -> no problem
	string path;
	int num_params;
	Param** params;
	int is_home_page;//0 -> not home page, !=0 -> home page
	int is_correct;// !=0 -> 400, =0 -> no problem

	HttpRequest(const char& request[]);
	//friend ostream& operator<<(std::ostream& os, const HttpRequest& r);
	~HttpRequest();
};

class HttpResponse {
	//int code;
	//string comment;
	Code code;
	int fd;
public:
	HttpResponse(int flag, HttpRequest& req);
	friend ostream& operator<<(std::ostream& os, const HttpResponse& r);
};