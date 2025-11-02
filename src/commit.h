#include "git_object.h"


struct User {
    std::string name;
    std::string email;
    // timezone and timestamp 
    time_t timestamp;
    std::string timezone;

    User(std::string& name, std::string& email);

    void serialize(std::string& output, const std::string& role)const;

    static std::string getLocalTimeZoneOffSet();
};


struct Commit {
    Header header;
    std::string parent;
    User author;
    User committer;

    void serialize(std::string& output) const;

};