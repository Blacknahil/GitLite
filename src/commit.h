#include "git_object.h"


struct User {
    std::string name;
    std::string email;
    // timezone and timestamp 
    long timestamp;
    std::string timezone;
};


struct Commit {
    Header header;
    std::string parent;
    User author;
    User committer;
};