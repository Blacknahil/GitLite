#pragma once
#include "git_object.h"
#include "helper.h"


struct User {
    std::string name;
    std::string email;
    // timezone and timestamp 
    time_t timestamp;
    std::string timezone;

    User();
    User(const std::string name, const std::string email);

    void serialize(std::string& output, const std::string& role)const;

    static std::string getLocalTimeZoneOffSet();
};


struct Commit {
    Header header;
    std::string treeHash;
    std::string parent;
    User author;
    User committer;
    std::string message;

    Commit(std::string& treeHash, std::string& parent, User& author, User& committer, std::string& message):
    header(ObjectType::commit, 0),
    author(author),
    committer(committer),
    treeHash(treeHash), parent(parent), message(message)
    {

    };

    void serialize(std::string& output) const;

};

void createCommit(std::string& commitSha,
                  std::string treeSha,
                  std::string parentSha,
                  std::string message);