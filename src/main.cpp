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
#include <type_traits>
#include <vector>
#include <zlib.h>
#include <unistd.h>

namespace fs = std::filesystem;
struct GitObject{
    std::string dirName;
    std::string fileName;
    std::string_view content;
    std::string type;
    size_t size;

    GitObject(std::string name)
    {
        dirName = name.substr(0,2);
        fileName = name.substr(2);
    }
};

struct TreeEntries{
    std::string_view mode;
    std::string_view name;
    std::string_view hash;
    std::string_view type;
};
class Tree{
    public:
        std::vector<TreeEntries> entries;

    public:
        void parseTree(const std::string_view& content);
};

void readTreeObject(const std::string& treeSha);
void readBlobObject(std::string& output, const std::string& fileName);
void zlibDecompression(std::string& decompressedView, GitObject& object);
void createBlobObject(std::string& hash, std::string inputFile);
void byteToHexHash(std::string& hexHash, const unsigned char* byteHash, size_t len);
void writeTreeObject(std::string& treehash);
void hexToByteHash(std::string& byteHash, std::string& hexHash);
void getSHA1(std::string& shaHash, std::string& data);
void zlibCompression(std::vector<Bytef>& compressed,
                    uLong compressedSize, 
                    uLong uncompressedSize,
                    std::string uncompressedData);





void Tree::parseTree(const std::string_view& content)
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
        std::string_view mode = content.substr(pos,spacePos-pos);
        pos= spacePos + 1;

        // name  up to null byte
        size_t nullPos = content.find('\0', pos);
        if(nullPos == std::string::npos) 
        {
            std::cout << "null byte not found"; 
            break;
        }
        std::string_view name = content.substr(pos,nullPos-pos);
        pos= nullPos + 1;

        // 20 bit SHA-1 binary hash
        std::string_view byteHash = content.substr(pos,20);
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
            if (a.name == b.name) return a.mode < b.mode;
            return a.name < b.name;
        });
    
    std::string treeContent;
    for (TreeEntries treeEntry:tree.entries)
    {
        treeContent.append(treeEntry.mode);
        treeContent.push_back(' ');
        treeContent.append(treeEntry.name);
        treeContent.push_back('\0');
        treeContent.append(treeEntry.hash);
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

    file.write(reinterpret_cast < const char*>(compressed.data()),  compressedSize);
    file.close();

}

void writeTreeObject(std::string& treeHash)
{
    fs::path current_path = fs::current_path();
    createTreeHash(treeHash, current_path);
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