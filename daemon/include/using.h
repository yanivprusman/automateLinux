#ifndef USIND_STD_H
#define USIND_STD_H

#include <string>
#include <iostream>
#include <filesystem>
#include <string>
#include <cstdio>
#include <cstring>
#include <nlohmann/json.hpp>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::istringstream;
using std::filesystem::canonical;
using std::to_string;
using std::strlen; 
using std::perror;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
#endif // USIND_STD_H
