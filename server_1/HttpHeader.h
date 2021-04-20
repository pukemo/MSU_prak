#pragma once
#include<string>


using namespace std;

class HttpHeader
{
private:
    string data;
public:
	HttpHeader(const string& header, const string& value);
    
    string getData();
    HttpHeader(const HttpHeader& old);
    friend ostream& operator<<(std::ostream& os, const HttpHeader& h);
};


/*class HttpHeaderFactory {

public:
    HttpHeaderFactory();

    HttpHeader Date();
    //HttpHeader Host();
    //HttpHeader Refer();
    HttpHeader Server(string serv);
    HttpHeader ContentType(int type);
    HttpHeader ContentLength(int len);
    HttpHeader LastModified(string date);
    HttpHeader Allow(const string coms[], int numcoms);

};*/

HttpHeader Date();
HttpHeader Server(string serv);
HttpHeader ContentType(int type);
HttpHeader ContentLength(int len);
HttpHeader LastModified(string date);
HttpHeader Allow(const string coms[], int numcoms);