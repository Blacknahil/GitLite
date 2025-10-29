#include <string>
#include <vector>

struct TreeEntries{
    std::string mode;
    std::string name;
    std::string hash;
    std::string type;
};
class Tree{
    public:
        std::vector<TreeEntries> entries;

    public:
        void parseTree(const std::string& content);
};

void readTreeObject(const std::string& treeSha);
void writeTreeObject(std::string& treehash);