#include <cstdlib>
#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <zlib.h>

struct GitObject{
    std::string dirName;
    std::string fileName;
    std::string content;
    std::string header;

    GitObject(std::string name)
    {
        dirName = name.substr(0,2);
        fileName = name.substr(2);
    }
};



void readBlobData(std::string& output, const std::string& fileName)
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
    std::vector<unsigned char>::iterator nullPtr = std::find(decompressed.begin(), decompressed.end(), '\0');
    if (nullPtr == decompressed.end())
    {
        throw std::runtime_error("Missing null pointer between header and content");
    }
    blobObject.header.assign(decompressed.begin(), nullPtr);
    blobObject.content.assign(nullPtr+1, decompressed.end());

    output.assign(blobObject.content.begin(),blobObject.content.end());

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
            readBlobData(output, argv[3]);
            std::cout << output;

        } catch (const std::exception& e)
        {
            std::cerr << e.what() << "/n";
            return EXIT_FAILURE;
        }

    }
    else{
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
