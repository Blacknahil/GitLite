#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <openssl/sha.h>

#include "helper.h"

namespace fs = std::filesystem;

void byteToHexHash(std::string& hexHash, const unsigned char* byteHash, size_t len){
    std::ostringstream charHash;
    for (int i = 0; i < len; i++)
    {
         charHash << std::hex << std::setw(2) <<std::setfill('0')<<(int) byteHash[i];
    }
    hexHash = charHash.str();
}
void hexToByteHash(std::string& byteHash, std::string& hexHash)
{
    if (hexHash.size() % 2 !=0)
    {
        throw std::runtime_error("hex hash can not be converted to 20 byte binary hash");
    }
    for (size_t i = 0; i < hexHash.size() ; i+=2)
    {
        std::string binarySubString = hexHash.substr(i,2);
        unsigned char byte = static_cast<unsigned char>(std::stoul(binarySubString, nullptr, 16));
        byteHash.push_back(byte); 

    }
}

void getSHA1(std::string& shaHash, std::string& data)
{
    unsigned char byteHash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()),  
        data.size(),
        byteHash);
     // convert the 20 byte hash to 40 character of hexadecimal 
    byteToHexHash(shaHash, byteHash, SHA_DIGEST_LENGTH);
}

void zlibCompression(std::vector<Bytef>& compressed,
                    uLong compressedSize, 
                    uLong uncompressedSize,
                    std::string uncompressedData)
{

    int ret = compress(compressed.data(), 
                    &compressedSize, 
                     reinterpret_cast<const Bytef*>(uncompressedData.data()), 
                  uncompressedSize);
    
    if(ret != Z_OK)
    {
        std::cerr << "Unable to compress file using zlib";
        return;
    }
    compressed.resize(compressedSize);
}

void zlibDecompression(std::string& decompressed, GitObject& object)
{
    std::string path("./.git/objects/"+ object.dirName + "/" + object.fileName);
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("failed to open file " + path);
    }
    // save file in memory for decompression 
    std::vector<unsigned char> compressed(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // decompress the file using zlib
    z_stream strm{};
    strm.next_in = compressed.data();
    strm.avail_in = compressed.size();

    if(inflateInit(&strm) != Z_OK)
    {
        throw std::runtime_error("inflateInit failed");
    }

    const size_t CHUNK_SIZE = 4096;
    std::vector<unsigned char> buffer(CHUNK_SIZE);
    std::vector<unsigned char> decompressedVector;

    int ret;
    do{
        strm.next_out = buffer.data();
        strm.avail_out = buffer.size();

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR)
        {
            inflateEnd(&strm);
            throw std::runtime_error("Zlib inflate failed!");
        }
        decompressedVector.insert(decompressedVector.end(), 
                            buffer.begin(),
                            buffer.begin() + (buffer.size() - strm.avail_out)
                            );
    
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);

    
    decompressed = std::string(decompressedVector.begin(),decompressedVector.end());
}

bool writeToFile(std::string& dirNamePart, 
                std::string& fileNamePart,
                std::vector<Bytef>  compressed,
                uLong compressedSize
                )
{
    fs::path folderPath(".git/objects/" + dirNamePart);
    if (!fs::create_directories(folderPath))
    {
        std::cerr << "failed to create directory " << folderPath.string();
        return false;
    }
    std::string fileName(folderPath.string() + "/" + fileNamePart);
    std::ofstream hashFile(fileName, std::ios::binary);

    if (!hashFile.is_open())
    {
        std::cerr << "failed to create or open file " << fileName; 
        return false;
    }

    hashFile.write(reinterpret_cast<const char*>(compressed.data()), 
                  compressedSize);
    hashFile.close();
    return true;

}