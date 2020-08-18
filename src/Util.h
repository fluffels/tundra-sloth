#pragma once

#include <string>
#include <vector>

#include "easylogging++.h"

using std::string;
using std::vector;

const char** stringVectorToC(const vector<string>&);
