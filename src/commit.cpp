#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "commit.h"

User::User(std::string& name, std::string& email):name(name), email(email)
{
    timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    timezone = getLocalTimeZoneOffSet();
}


// getLocaltimeZone off set function static 

static std::string getLocalTimeZoneOffSet()
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

void User::serialize(std::string& output, const std::string& role)
{
    std::ostringstream oss;
    oss << role << ' ' << name << " <" << email << "> "
        << timestamp << ' ' << timezone << '\n';

    output = oss.str();
}