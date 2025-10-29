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