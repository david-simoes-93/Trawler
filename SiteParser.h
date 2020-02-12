//
// Created by david on 10-02-2020.
//

#ifndef TRAWLER_SITEPARSER_H
#define TRAWLER_SITEPARSER_H

#include <string>

class SiteParser {
public:
    virtual std::string getImageLink(const std::string &htmlCode, const uint16_t &currPage) = 0;

    virtual bool chapterExists(const std::string &htmlCode) = 0;

    static std::string ReplaceAll(std::string str, const std::string &from, const std::string &to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }
};

#endif //TRAWLER_SITEPARSER_H
