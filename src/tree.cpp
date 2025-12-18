#include <filesystem>
#include <fstream>
#include <algorithm>

#include "blob.h"
#include "tree.h"
#include "helper.h"

namespace fs = std::filesystem;

void Tree::parseTree(const std::string& content)
{
    size_t l = content.size();
    size_t pos = 0;
    // std::cout << "content " << content;
    while (pos < l)
    {
        // mode 
        size_t spacePos = content.find(' ',pos);
        if(spacePos == std::string::npos) 
        {
            std::cout << "space not found";
            break;
        }
        std::string mode = content.substr(pos,spacePos-pos);
        pos= spacePos + 1;

        // name  up to null byte
        size_t nullPos = content.find('\0', pos);
        if(nullPos == std::string::npos) 
        {
            std::cout << "null byte not found"; 
            break;
        }
        std::string name = content.substr(pos,nullPos-pos);
        pos= nullPos + 1;

        // 20 bit SHA-1 binary hash
        std::string byteHash = content.substr(pos,20);
        pos += 20;
        std::string hexHash;
        std::vector<unsigned char> data_vect(byteHash.begin(), byteHash.end());
        byteToHexHash(hexHash, data_vect.data(),data_vect.size());

        // std::cout << "mode: " << mode << " name: " << name << "hex hash: " << hexHash;

        TreeEntries entry{mode,name,hexHash};
        if (mode == "40000")
        {
            entry.type = "tree";
        }
        else if (mode == "100644")
        {
            entry.type = "blob";
        }
        else
        {
            entry.type = "blob";
        }
        entries.push_back(entry);

    }
}

void createTreeHash(std::string& treeHash, fs::path dir_path)
{
    if (!fs::exists(dir_path))
    {
        std::cerr << "directory not found!";
        return;
    }

    if (!fs::is_directory(dir_path))
    {
        std::cerr << "path is not a directory " << dir_path;
        return;
    }
    Tree tree;
    for (fs::directory_entry dir_entry: fs::directory_iterator(dir_path))
    {
        fs::path entry_path = dir_entry.path();
        std::string entry_pathStr = entry_path.filename().string();
        std::string outputHash;
        TreeEntries treeEntry;
        if (dir_entry.is_directory() && entry_pathStr == ".git") continue;

        if (fs::is_regular_file(dir_entry))
        {
            createBlobObject(outputHash, entry_path.string());
            std::string binaryHash;
            hexToByteHash(binaryHash, outputHash);

            if (access(entry_path.c_str(), X_OK) == 0)
            {
                // this is an excutable use mode = 100755
                treeEntry.mode = "100755";
            }
            else 
            {
                // it is a regular file use mode = 100644
                treeEntry.mode = "100644";
            }

            treeEntry.name = entry_pathStr;
            treeEntry.hash = binaryHash;
        }
        else if (fs::is_symlink(dir_entry))
        {
            createBlobObject(outputHash, entry_path.string());
            std::string binaryHash;
            hexToByteHash(binaryHash, outputHash);
            // use mode = 120000
            treeEntry.mode = "120000";
            treeEntry.name = entry_pathStr;
            treeEntry.hash = binaryHash;

        }
        else if (fs::is_directory(dir_entry))
        {
            // go recursively here
            std::string subdirHash;
            createTreeHash(subdirHash, entry_path.string());
            std::string binaryHash;
            hexToByteHash(binaryHash, subdirHash);

            treeEntry.mode = "40000";
            treeEntry.name = entry_pathStr;
            treeEntry.hash = binaryHash;

        }
        else 
        {
            std::cerr << "skipping undetected object at : " << dir_entry;
            continue;
        }

        tree.entries.push_back(treeEntry);
    }
    // save the tree entries in a tree object with a header

    // sort the tree entries 
    std::sort(tree.entries.begin(),
              tree.entries.end(),
            [](const TreeEntries& a, const TreeEntries& b) 
        {
            // if (a.name == b.name) return a.mode < b.mode;
            return a.name < b.name;
        });
    
    std::string treeContent;
    for (TreeEntries treeEntry:tree.entries)
    {
        treeContent.append(treeEntry.mode);
        treeContent.push_back(' ');
        treeContent.append(treeEntry.name);
        treeContent.push_back('\0');
        treeContent.insert(treeContent.end(), treeEntry.hash.begin(), treeEntry.hash.end());
    }
    std::string treeHeader = "tree " + std::to_string(treeContent.size());
    treeHeader.push_back('\0');
    std::string treeData;
    treeData.reserve(treeHeader.size() + treeContent.size());
    treeData.append(treeHeader);
    treeData.append(treeContent);

    // produce the SHA-1 hash from the content of the tree object 
    getSHA1(treeHash, treeData);

    // compress the data 
    uLong uncompressedSize = treeData.size();
    uLong compressedSize = compressBound(uncompressedSize);
    std::vector<Bytef> compressed(compressedSize);

    zlibCompression(compressed,compressedSize,uncompressedSize, treeData);

    // create files and folders and write the content 
    std::string dirNamePart = treeHash.substr(0,2);
    std::string fileNamePart = treeHash.substr(2);
    fs::path folderPath(".git/objects/" + dirNamePart);
    if (!fs::create_directories(folderPath))
    {
        std::cerr << "failed to create directory" << folderPath.string();
        return;
    }
    std::string fileName(folderPath.string() + "/" + fileNamePart);
    std::ofstream file(fileName, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "failed to create or open file " << fileName;
        return;
    }

    file.write(reinterpret_cast < const char*>(compressed.data()),  compressed.size());
    file.close();
    // std::cout << "size " << compressed.size() << " ";

}

void writeTreeObject(std::string& treeHash)
{
    fs::path current_path(".");
    createTreeHash(treeHash, current_path);
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