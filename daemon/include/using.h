#ifndef USING_H
#define USING_H

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::istringstream;
using std::ofstream;
using std::perror;
using std::string;
using std::stringstream;
using std::strlen;
using std::to_string;
using std::filesystem::canonical;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
using std::set;
using std::stoi;
using std::unordered_map;
using std::vector;
#endif // USING_H
