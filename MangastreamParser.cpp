//
// Created by david on 10-02-2020.
//

#include <regex>
#include <iostream>
#include "MangastreamParser.h"

std::string MangastreamParser::getImageLink(const std::string &htmlCode, const uint16_t &currPage) {
    //std::cout << htmlCode << std::endl;

    // Use regex to get image URL
    std::regex imageLinkRegex("src=.*?\\.jpg");
    std::smatch imageLinkMatch;
    if (not std::regex_search(htmlCode, imageLinkMatch, imageLinkRegex)) {
        return "";
    }

    // Get and save image
    std::string imageLink = imageLinkMatch[0].str().substr(5);


    return ReplaceAll(imageLink,"https","http");
}

bool MangastreamParser::chapterExists(const std::string &htmlCode) {
    return htmlCode.find("is not released yet") == std::string::npos;
}