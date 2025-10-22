#include <cstdlib>
#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <zlib.h>

void readTreeObject(const std::string& treeSha);
struct GitObject{
    std::string dirName;
    std::string fileName;
    std::string content;
    std::string name;
    size_t size;

    GitObject(std::string name)
    {
        dirName = name.substr(0,2);
        fileName = name.substr(2);
    }
};



void readGitObject(std::string& output, const std::string& fileName)
{
    GitObject blobObject(fileName);

    if (blobObject.fileName.length() < 2)
    {
        throw std::runtime_error("git object name must be at least 2 characters long!");
    }

    std::string path("./.git/objects/"+ blobObject.dirName + "/" + blobObject.fileName);
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
    std::vector<unsigned char> decompressed;

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
        decompressed.insert(decompressed.end(), 
                            buffer.begin(),
                            buffer.begin() + (buffer.size() - strm.avail_out)
                            );
    
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);

    /// split the content into headers and contents
    std::string_view decompressedView(
                                reinterpret_cast<const char*>(decompressed.data())
                                , decompressed.size());
    size_t nullPos = decompressedView.find('\0');
    if (nullPos == std::string::npos)
    {
        throw std::runtime_error("Missing null pointer between header and content");
    }
    blobObject.content = decompressedView.substr(nullPos+1);

    std::string_view header = decompressedView.substr(0,nullPos);
    size_t spacePos = header.find(' ');
    blobObject.name = header.substr(0,spacePos);
    blobObject.size = std::stol(std::string(header.substr(spacePos+1)));

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
    unsigned char byteHash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(uncompressed.c_str()),  
        uncompressed.size(),
        byteHash);
    // convert the 20 byte hash to 40 character of hexadecimal 
    std::ostringstream charHash;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
         charHash << std::hex << std::setw(2) <<std::setfill('0')<<(int) byteHash[i];
    }
    std::string hashStr = charHash.str();
    std::string dirNamePart = hashStr.substr(0,2);
    std::string fileNamePart = hashStr.substr(2);

    // compress the  content + size + blob

    // const size_t CHUNK_SIZE = 4096;
    // std::vector<unsigned char> buffer(CHUNK_SIZE);
    uLong uncompressedSize = uncompressed.size();
    uLong compressedSize = compressBound(uncompressedSize);
    std::vector<Bytef> compressed(compressedSize);

    int ret = compress(compressed.data(), 
                    &compressedSize, 
                     reinterpret_cast<const Bytef*>(uncompressed.data()), 
                  uncompressedSize);
    
    if(ret != Z_OK)
    {
        std::cerr << "Unable to compress file using zlib";
        return;
    }
    compressed.resize(compressedSize);

    // create the SHA-1 file inside the sha1[:2] folder 
    std::filesystem::path folderPath(".git/objects/" + dirNamePart);
    if(!std::filesystem::create_directories(folderPath))
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
    hashFile.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    hashFile.close();
    // print the file and folder name for verification for the tester
    hash = hashStr;
}

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage
    //
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }

    std::string command = argv[1];
    
    if (command == "init") {
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");
    
            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
    
            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    } 
    else if (command == "cat-file")
    {
        if (argc != 4)
        {
            std::cerr << "Blob filename error";
            return EXIT_FAILURE;
        }
        std::string output;
        try 
        {
            readGitObject(output, argv[3]);
            std::cout << output;

        } catch (const std::exception& e)
        {
            std::cerr << e.what() << "/n";
            return EXIT_FAILURE;
        }

    }
    else if (command == "hash-object")
    {
        if (argc !=4)
        {
            std::cerr << "incorrect cmdline";
            return EXIT_FAILURE;
        }
        if (std::string(argv[2]) != "-w")
        {
            std::cerr << "missing paramter: -w <fileName>\n";
        }
        std::string hash;
        createBlobObject(hash, argv[3]);
        std::cout << hash;

    }

    else if(command == "ls-tree")
    {
        if (argc !=4 || argv[2] !="--name-only")
        {
            std::cerr << "wrong paramter usage --name-only <tree_sha>";
            return EXIT_FAILURE;
        }
        readTreeObject(argv[3]);

    }
    else{
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


void readTreeObject(const std::string& treeSha)
{
    
}
