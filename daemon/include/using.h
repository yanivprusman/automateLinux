#ifndef USIND_H
#define USIND_H

#include <string>
#include <iostream>
#include <filesystem>
#include <string>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::istringstream;
using std::ofstream;
using std::ifstream;
using std::stringstream;
using std::filesystem::canonical;
using std::to_string;
using std::strlen; 
using std::perror;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
using std::set;
using std::vector;
using std::stoi;   
using std::unordered_map;
; //keep this in case of not adding ; at the last using
#endif // USIND_H
