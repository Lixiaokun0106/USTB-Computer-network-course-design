#pragma once
#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>

//base64±àÂë
std::string base64_encode(const std::string& in);

//base64½âÂë
std::string base64_decode(const std::string& in);

#endif //BASE64_H
