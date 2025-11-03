#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

#include "commit.h"

User::User(std::string name, std::string email):name(name), email(email)
{
    timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    timezone = getLocalTimeZoneOffSet();
}


// getLocaltimeZone off set function static 

std::string User::getLocalTimeZoneOffSet()
{
    std::time_t now = std::time(nullptr);
    std::tm local_tm = *std::localtime(&now);
    std::tm utc_tm = *std::gmtime(&now);

    long offset_seconds = std::difftime(std::mktime(&local_tm), std::mktime(&utc_tm));
    int hours = offset_seconds / 3600;
    int minutes = std::abs((offset_seconds % 3600)/ 60);

    std::ostringstream oss;
    oss << (hours >=0 ? '+':'-')
        << std::setw(2) << std::setfill('0') << std::abs(hours)
        << std::setw(2) << std::setfill('0') << minutes;

    return oss.str();
}

// serialize User method

void User::serialize(std::string& output, const std::string& role) const
{
    std::ostringstream oss;
    oss << role << ' ' << name << " <" << email << "> "
        << timestamp << ' ' << timezone << '\n';

    output = oss.str();
}

void Commit::serialize(std::string& output) const 
{
    std::ostringstream oss;
    if(!treeHash.empty()) oss << "tree " << treeHash << '\n';
    if(!parent.empty()) oss << "parent " << parent << '\n';
    std::string authorSS;
    std::string committerSS;
    author.serialize(authorSS, "author");
    committer.serialize(committerSS, "commit");
    oss << authorSS << '\n';
    oss << committerSS << '\n';
    oss << message << '\n';

    output = oss.str();
}

void createCommit(std::string& commitSha,
                  std::string treeSha,
                  std::string parentSha,
                  std::string message)
{
   


    User author(std::string("Nahom Garefo"),std::string("nahom23@gmail.com"));
    User committer("Blacknahil", "nahil@gmail.com");
    Commit commit(treeSha, parentSha,
                          author, committer, message);
    commit.author = author;
    commit.committer = committer;

    std::string commitContent;
    commit.serialize(commitContent);
    size_t commitContentSize = commitContent.size();
    Header header(ObjectType::commit, commitContentSize);
    commit.header = header;

    std::string wholeCommitContent(commit.header.typeAsString() + ' ');
    wholeCommitContent += commit.header.size;
    wholeCommitContent += '\0';
    wholeCommitContent += commitContent;

    // generate sha hash 
    getSHA1(commitSha, wholeCommitContent);
    std::string dirNamePart = commitSha.substr(0,2);
    std::string fileNamePart = commitSha.substr(2);

    // decompress  
    uLong uncompressedSize = wholeCommitContent.size();
    uLong compressedSize = compressBound(uncompressedSize);
    std::vector<Bytef> compressed(compressedSize);
    zlibCompression(compressed, compressedSize, 
                        uncompressedSize, 
                        wholeCommitContent);
    

    // and write to file 
    bool status = writeToFile(dirNamePart, 
                              fileNamePart,
                              compressed, 
                              compressedSize);
    if(!status)
    {
        std::cerr << "failed to write compressed commit object";
        return;
    }
}