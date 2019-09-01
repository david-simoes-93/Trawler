#include <iostream>
#include <array>
#include <memory>
#include <ctime>
#include <limits>
#include <boost/filesystem.hpp>
#include <sstream>
#include <regex>
#include <fstream>

#include <zip.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#ifdef gcc8
#include <string_view>
#define literals std::literals
#else

#include <experimental/string_view>

#define std_literals std::experimental::literals
#endif


using namespace std_literals;


int main(int argc, char *argv[]) {
    // process args: compress(yes/no), start ch, end ch, savedir
    if (argc < 2) {
        std::cout << "Please provide a MangaReader address:" << std::endl;
        std::cout << "\thttps://www.mangareader.net/soul-eater" << std::endl;
        return 0;
    }

    std::string mangaLink = std::string(argv[1]);
    bool compress = true;
    uint32_t startChapter = 1;
    uint32_t endChapter = 9999; // std::numeric_limits<uint32_t>::max();
    std::string saveDir = boost::filesystem::current_path().string();

    for (uint8_t argIndex = 2; argIndex < argc; ++argIndex) {
        if (argv[argIndex] == "--no-compress"sv) {
            compress = false;
        } else if (argv[argIndex] == "--start"sv) {
            argIndex++;
            startChapter = std::atoi(argv[argIndex]);
        } else if (argv[argIndex] == "--end"sv) {
            argIndex++;
            endChapter = std::min(std::atoi(argv[argIndex]), 9999);
        } else if (argv[argIndex] == "--save"sv) {
            argIndex++;
            std::string saveDir = std::string(argv[argIndex]);
        } else {
            std::cout << "Trawler https://www.mangareader.net/soul-eater"
                      << "[--no-compress] [--start 1] [--end 100] [--save /tmp]" << std::endl;
        }
    }

    // Create save directory
    boost::filesystem::create_directory(saveDir);

    // Ready the HTTP request and HTTP request for JPGs
    curlpp::Easy myRequest;
    curlpp::Easy myRequestJpg;

    // crawl each chapter
    for (uint32_t currChapter = startChapter; currChapter <= endChapter; ++currChapter) {
        // format chapter numbers
        char *currChapterStr = new char[5];
        sprintf(currChapterStr, "%04d", currChapter);

        //https://www.mangareader.net/soul-eater/45/
        std::string chapterDir = saveDir + "/" + std::string(currChapterStr) + "/";
        boost::filesystem::create_directory(chapterDir);

        // TODO handle timeout crash

        // crawl each page, up to one thousand
        uint16_t currPage = 1;
        while(currPage<1000){
            std::string currLink = mangaLink+"/"+std::to_string(currChapter) + "/"+std::to_string(currPage);
            std::cout << currLink << std::endl;

            try {
                // Set the URL, send request, and get a result
                myRequest.setOpt<curlpp::options::Url>(currLink);
                std::ostringstream htmlStream;
                curlpp::options::WriteStream ws(&htmlStream);
                myRequest.setOpt(ws);
                myRequest.perform();
                std::string htmlCode = htmlStream.str();
                //std::cout << htmlCode << std::endl;

                // check if chapter exists
                std::regex chapterExistsRegex ("is not released yet.");
                std::smatch chapterExistsMatch;
                if (std::regex_search (htmlCode,chapterExistsMatch,chapterExistsRegex)) {
                    // delete created folder
                    boost::filesystem::remove(chapterDir);
                    endChapter = currChapter;
                    break;
                }

                // Use regex to get image URL
                std::regex imageLinkRegex ("src=.*?\\.jpg");
                std::smatch imageLinkMatch;
                if (not std::regex_search (htmlCode,imageLinkMatch,imageLinkRegex)) {
                    break;
                }

                // format page numbers
                char* currPageStr = new char[4];
                sprintf(currPageStr, "%03d", currPage);

                // Get and save image
                std::string imageLink = imageLinkMatch[0].str().substr(5);
                //std::cout << imageLink << std::endl;
                myRequestJpg.setOpt<curlpp::options::Url>(imageLink);
                std::ofstream outputFile(chapterDir+std::string(currPageStr)+".jpg");
                curlpp::options::WriteStream wsJpg(&outputFile);
                myRequestJpg.setOpt(wsJpg);
                myRequestJpg.perform();
                outputFile.close();
            } catch(curlpp::RuntimeError &e) {
                std::cerr << e.what() << std::endl;
            } catch(curlpp::LogicError &e) {
                std::cerr << e.what() << std::endl;
            }

            currPage++;
        }

        if (compress) {
            std::cout << "Compressing..." << std::endl;

            // create cbz archive
            int errorp;
            std::string chapterZipName = saveDir + "/" + std::string(currChapterStr) + ".cbz";
            zip_t *archive = zip_open(chapterZipName.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &errorp);
            if (archive == nullptr) {
                zip_error_t ziperror;
                zip_error_init_with_code(&ziperror, errorp);
                std::cout << "Failed to open output file " << chapterZipName << ": " <<
                          zip_error_strerror(&ziperror) << std::endl;
                continue;
            }

            bool crashed = false;
            for (uint16_t zCurrPage = 1; zCurrPage < currPage; zCurrPage++) {
                char *currPageStr = new char[8];
                sprintf(currPageStr, "%03d.jpg", zCurrPage);
                std::string currPageSrc = chapterDir + std::string(currPageStr);

                // get source of page to zip
                zip_source_t *zsource = zip_source_file(archive, currPageSrc.c_str(), 0, 0);
                if (zsource == nullptr) {
                    std::cout << "Zip source error: " << zip_strerror(archive) << " with " << currPageSrc << std::endl;
                    zip_source_free(zsource);
                    crashed = true;
                    break;
                }

                // add page to archive
                if (zip_file_add(archive, currPageStr, zsource, ZIP_FL_ENC_UTF_8) < 0) {
                    zip_source_free(zsource);
                    std::cout << "Zipping error: " << zip_strerror(archive) << std::endl;
                    crashed = true;
                    break;
                }
            }

            // Write and close archive
            if (not crashed) {
                int errorClose = zip_close(archive);
                if (errorClose != 0) {
                    std::cout << "Zip closing error: " << errorClose << std::endl;
                    zip_discard(archive);
                    continue;
                }
            } else {
                zip_discard(archive);
                continue;
            }

            // delete folder
            boost::filesystem::remove_all(chapterDir);
        }
    }


    return 0;
}


void zip_directory() {
    int errorp;
    zip_t *zipper = zip_open("myzip.zip", ZIP_CREATE | ZIP_EXCL, &errorp);
    if (zipper == nullptr) {
        zip_error_t ziperror;
        zip_error_init_with_code(&ziperror, errorp);
        std::cout << "Failed to open output file myzip.zip: " << zip_error_strerror(&ziperror) << std::endl;
    }

    std::string fullname = std::string("/home/david/a");
    zip_source_t *source = zip_source_file(zipper, fullname.c_str(), 0, 0);
    if (source == nullptr) {
        throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zipper)));
    }
    if (zip_file_add(zipper, std::string("a").c_str(), source, ZIP_FL_ENC_UTF_8) < 0) {
        zip_source_free(source);
        throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zipper)));
    }

    zip_close(zipper);
}
