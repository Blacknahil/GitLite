#include <cstddef>
#include <iostream>
struct GitObject{
    std::string dirName;
    std::string fileName;
    std::string content;
    std::string type;
    size_t size;

    GitObject(std::string name)
    {
        dirName = name.substr(0,2);
        fileName = name.substr(2);
    }
};

enum ObjectType {blob, commit, tree};
struct Header {
    ObjectType type;
    size_t size;

    Header();
    Header(ObjectType type, size_t size): type(type), size(size){};
    std::string typeAsString() const
    {
        switch(type)
        {
            case blob: return "blob";
            case commit: return "commit";
            case tree : return "tree";
            default: return "unknown";
        }
    }
};