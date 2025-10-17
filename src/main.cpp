#include <cstdlib>
#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

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
    bool inHeader = true;
    char c;

    while (file.get(c))
    {
        if (inHeader)
        {
            if (c == '\0')
            {
                inHeader = false;
                continue;
            }
            blobObject.header.push_back(c);
        }
        else
        {
            blobObject.content.push_back(c);
        }
    }
    if (blobObject.header.empty())
    {
        throw std::runtime_error("no header information found!");
    }

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
        if (argc != 3)
        {
            std::cerr << "Blob filename error";
            return EXIT_FAILURE;
        }
        std::string output;
        try 
        {
            readBlobData(output, argv[0]);
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
