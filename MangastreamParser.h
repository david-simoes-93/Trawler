//
// Created by david on 10-02-2020.
//

#ifndef TRAWLER_MANGASTREAMPARSER_H
#define TRAWLER_MANGASTREAMPARSER_H

#include "SiteParser.h"

class MangastreamParser : public SiteParser {
    virtual std::string getImageLink(const std::string &htmlCode, const uint16_t &currPage) override;

    virtual bool chapterExists(const std::string &htmlCode) override;
};

#endif //TRAWLER_MANGASTREAMPARSER_H
