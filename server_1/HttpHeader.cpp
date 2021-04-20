#include "HttpHeader.h"
#include <ctime>
#include <iostream>

using namespace std;

enum {
    PLAIN = 0,
    HTML = 1,
    JPEG = 2
};

const string plain = "text/plain";
const string html = "text/html";
const string jpeg = "image/jpeg";
const string type_err = "type error";

HttpHeader::HttpHeader(const string& header, const string& value) {
    this->data = header + ": " + value + "\n";
}

string HttpHeader::getData() {
    return string(data);
}

HttpHeader::HttpHeader(const HttpHeader& old)  {
    string tmp(old.getData());
    this->data = tmp;
}

ostream& operator<<(std::ostream& os, const HttpHeader& h)
{
    os << h.data;
    return os;
}


HttpHeader Date() {
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%a, %d %b %G %H:%M:%S GMT", timeinfo);
    std::string str(buffer);

    string header_type = "Date";
    return HttpHeader(header_type, str);
}

HttpHeader Server(string serv) {
    return HttpHeader("Server", serv);
}

HttpHeader ContentType(int type) {
    const string cont;
    switch (type) {
    case PLAIN:
        cont = plain;
        break;
    case HTML:
        cont = html;
        break;
    case JPEG:
        cont = jpeg;
        break;
    default:
        cont = type_error;
        break;
    }
    return HttpHeader("Content-type", cont);
}

HttpHeader ContentLength(int len) {

}

HttpHeader LastModified(string date) {
    return HttpHeader("Last-modified", date);
}

HttpHeader Allow(const string coms[], int numcoms) {
    string comres = "";
    for (int i = 0; i < numcoms; i++) {
        if (i) {
            comres += ", ";
        }
        comres += coms[i];
    }
    return HttpHeader("Allow", comres);
}
