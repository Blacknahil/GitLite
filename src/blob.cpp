#include <sstream>
#include <fstream>
#include <filesystem>

#include "blob.h"
#include "tree.h"
#include "git_object.h"
#include "helper.h"

namespace fs = std::filesystem;

void readBlobObject(std::string& output, const std::string& fileName)
{
    GitObject blobObject(fileName);

    if (blobObject.fileName.length() < 2)
    {
        throw std::runtime_error("git object name must be at least 2 characters long!");
    }
    std::string decompressed;
    zlibDecompression(decompressed, blobObject);
    std::string_view decompressedView(decompressed);

    size_t nullPos = decompressedView.find('\0');
    if (nullPos == std::string::npos)
    {
        throw std::runtime_error("Missing null pointer between header and content");
    }
    blobObject.content = decompressedView.substr(nullPos+1);

    std::string_view header = decompressedView.substr(0,nullPos);
    size_t spacePos = header.find(' ');
    blobObject.type = header.substr(0,spacePos);
    blobObject.size = std::stol(std::string(header.substr(spacePos+1)));

    output = std::string(blobObject.content);
}

void createBlobObject(std::string& hash, std::string inputFile)
{
    // open file 
    std::ifstream file(inputFile, std::ios::binary);
    if(!file)
    {
        throw std::runtime_error("failed to open file " + inputFile);
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string temp(oss.str());

    // produce SHA-1 hash from the content + size + blob
    std::string uncompressed("blob " + std::to_string(temp.size()) + '\0' + temp);
    std::string hashStr;
    getSHA1(hashStr, uncompressed);
    std::string dirNamePart = hashStr.substr(0,2);
    std::string fileNamePart = hashStr.substr(2);

    // compress the  content + size + blob

    // const size_t CHUNK_SIZE = 4096;
    // std::vector<unsigned char> buffer(CHUNK_SIZE);
    uLong uncompressedSize = uncompressed.size();
    uLong compressedSize = compressBound(uncompressedSize);
    std::vector<Bytef> compressed(compressedSize);
    zlibCompression(compressed, compressedSize, uncompressedSize, uncompressed);

    // create the SHA-1 file inside the sha1[:2] folder
    fs::path folderPath(".git/objects/" + dirNamePart);
    if(!fs::create_directories(folderPath))
    {
        std::cerr << "failed to create directory " << folderPath.string();
        return;
    }
    std::string fileName(folderPath.string() + "/" + fileNamePart);
    std::ofstream hashFile(fileName, std::ios::binary);
    if (!hashFile.is_open())
    {
        std::cerr << "failed to create or open file " << fileName; 
        return;
    }
    // write the compressed file to it 
    hashFile.write(reinterpret_cast<const char*>(compressed.data()), 
                  compressedSize);
    hashFile.close();
    // print the file and folder name for verification for the tester
    hash = hashStr;
}