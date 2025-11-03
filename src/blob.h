#pragma once
#include <string>

void createBlobObject(std::string& hash, std::string inputFile);
void readBlobObject(std::string& output, const std::string& fileName);