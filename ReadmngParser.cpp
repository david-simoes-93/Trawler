//
// Created by david on 10-02-2020.
//

#include <regex>
#include <iostream>
#include "ReadmngParser.h"

std::string ReadmngParser::getImageLink(const std::string &htmlCode, const uint16_t &currPage) {
    //std::cout << htmlCode << std::endl;

    char *currPageStr = new char[25];
    sprintf(currPageStr, "%d,\"url\":\".*%05d\\....", currPage - 1, currPage);
    //std::cout << currPageStr << std::endl;

    // Use regex to get image URL
    std::regex imageLinkRegex(currPageStr);
    std::smatch imageLinkMatch;
    if (not std::regex_search(htmlCode, imageLinkMatch, imageLinkRegex)) {
        return "";
    }

    // Get and save image
    std::string imageLink = imageLinkMatch[0].str();
    imageLink = imageLink.substr(imageLink.find("h"));
    imageLink = ReplaceAll(imageLink, "\\", "");
    std::cout << imageLink << std::endl;

    return imageLink;
}

bool ReadmngParser::chapterExists(const std::string &htmlCode) {
    return htmlCode.find("is not available yet.") == std::string::npos;
}