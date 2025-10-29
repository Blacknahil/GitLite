#include <cstdlib>
#include <exception>
#include <iostream>
#include <iterator>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <git_object.h>
#include <tree.h>
#include <helper.h>

namespace fs = std::filesystem;





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
            fs::create_directory(".git");
            fs::create_directory(".git/objects");
            fs::create_directory(".git/refs");
    
            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
    
            std::cout << "Initialized git directory\n";
        } catch (const fs::filesystem_error& e) {
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
            readBlobObject(output, argv[3]);
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
        if (argc !=4 || std::string(argv[2]) !="--name-only")
        {
            std::cerr << "wrong paramter usage --name-only <tree_sha>";
            std::cerr << argc;
            std::cerr << "argv[2]" << argv[2];
            return EXIT_FAILURE;
        }
        readTreeObject(argv[3]);

    }
    else if (command == "write-tree")
    {
        if (argc != 2)
        {
            std::cerr << "no argument expected";
        }
        std::string treeHash;
        writeTreeObject(treeHash);
        std::cout << treeHash;
    }
    else{
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


void readTreeObject(const std::string& treeSha)
{
    GitObject treeObject(treeSha);
    if (treeObject.fileName.length() < 2)
    {
        throw std::runtime_error("git object name must be at least 2 characters long!");
    }
    std::string decompressed;
    zlibDecompression(decompressed, treeObject);
    std::string_view decompressedView(decompressed);

    size_t nullPos = decompressedView.find('\0');
    if (nullPos == std::string::npos)
    {
        throw std::runtime_error("Missing null pointer between header and content");
    }
    treeObject.content = decompressedView.substr(nullPos+1);
    std::string_view header = decompressedView.substr(0,nullPos);
    Tree tree;
    tree.parseTree(treeObject.content);

    // console log 

    for (TreeEntries& entries:tree.entries)
    {
        std::cout << entries.name << "\n";
    }
}

