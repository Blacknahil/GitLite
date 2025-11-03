#pragma once
#include <iostream>
#include <unistd.h>
#include <zlib.h>
#include <vector>

#include "git_object.h"

void byteToHexHash(std::string& hexHash, const unsigned char* byteHash, size_t len);
void hexToByteHash(std::string& byteHash, std::string& hexHash);
void getSHA1(std::string& shaHash, std::string& data);
void zlibCompression(std::vector<Bytef>& compressed,
                    uLong compressedSize, 
                    uLong uncompressedSize,
                    std::string uncompressedData);
void zlibDecompression(std::string& decompressedView, GitObject& object);
bool writeToFile(std::string& dirNamePart, 
                std::string& fileNamePart,
                std::vector<Bytef>  compressed,
                uLong compressedSize
                );