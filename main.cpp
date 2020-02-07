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

#include "Trip.h"

using namespace std_literals;


std::string ReplaceAll(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::string mangastream_image(std::string &htmlCode) {
    // Use regex to get image URL
    std::regex imageLinkRegex("src=.*?\\.jpg");
    std::smatch imageLinkMatch;
    if (not std::regex_search(htmlCode, imageLinkMatch, imageLinkRegex)) {
        return "";
    }

    // Get and save image
    std::string imageLink = imageLinkMatch[0].str().substr(5);

    return imageLink;
}

std::string readmanga_image(std::string &htmlCode, uint16_t &currPage) {
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


void showHelp() {
    std::cout << "Please provide a MangaReader or ReadManga address:" << std::endl;
    std::cout << "\thttps://www.mangareader.net/soul-eater" << std::endl;
    std::cout << "\thttps://www.readmng.com/dragon-ball" << std::endl;
    std::cout << "Example usage: Trawler https://www.mangareader.net/soul-eater "
              << "[--no-compress] [--start 1] [--end 100] [--save /tmp]" << std::endl;
}


int main(int argc, char *argv[]) {
    // process args: compress(yes/no), start ch, end ch, savedir
    if (argc < 2) {
        showHelp();
        return 0;
    }

    std::string mangaLink = std::string(argv[1]);
    bool mangareader_link = mangaLink.find("mangareader") < mangaLink.size();
    if (!mangareader_link) {
        bool readmng_link = mangaLink.find("readmng") < mangaLink.size();
        if (!readmng_link) {
            showHelp();
            return 0;
        }
    }

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
            showHelp();
            return 0;
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
        while (currPage < 1000) {
            std::string currLink = mangaLink + "/" + std::to_string(currChapter) + "/" + std::to_string(currPage);
            std::cout << currLink << std::endl;

            try {
                // Set the URL, send request, and get a result
                myRequest.setOpt<curlpp::options::Url>(currLink);

                myRequest.setOpt(curlpp::options::UserAgent(std::string(
                        "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:72.0) Gecko/20100101 Firefox/72.0")));
                myRequest.setOpt(curlpp::options::Cookie(std::string(
                        "Cookie: __cfduid=dce7d571085ddb5906dd36bb8bfcff9551580944641; ci_session=iRJr2RsPUVUcrzgqyXq6MFYlGuRCY1n8TNBWS8JWB%2BCNiQb752rGMUFI4bj4SmbQh333iG3%2BGcx4zMZa1SLMt0uatOIgBjWgdcXbIrK8GCu5robNWUH1LcY3QyYo0DM43OHa5%2BD%2BkYVTjaB2YfnYlRzBCGkX7B1o4H7jFmKNqpSsyXeL7XfKAg9UzzhHjzGsPt28rQc%2BJacuZj3s30pRKMv9KK9A5HV5VtPqKe955ZEyyvnmVVeTocNy8DZMDIbGScoo0PWD7E0YvyJV5AUxX6P23s%2B83GuBPi0cTcIG2LB%2FBf1YViP8GIUbAtq4luMFYJLLsQOQN03UX3QwCKv6loTj%2FIFwUN%2BBB0zbtxfGtj8VScKycVUgKThQapKR8EhxC%2Fq12QEkl2bXsqWszfW0pXyqEn7sOqZQzjYRy9oDc2w%3D; _ga=GA1.2.257006594.1580944644; _gid=GA1.2.1622360435.1580944644; OB-USER-TOKEN=3c134b0f-d2dd-4220-83e6-430ba7ce9d37; _awl=2.1580945930.0.4-6a365285-a98b5de77a6c926b649cf7751b68ee68-6763652d6575726f70652d7765737431-5e3b520a-1; cf_clearance=38d853a3621b470a742635b1698773e1c3ec0b3b-1580946598-0-150")));
                //myRequest.setOpt(curlpp::options::Referer(std::string("https://www.readmng.com/dragon-ball/277/1")));

                //myRequest.setOpt(new curlpp::options::FileTime(true));
                //myRequest.setOpt(new curlpp::options::Verbose(true));


                std::ostringstream htmlStream;
                curlpp::options::WriteStream ws(&htmlStream);
                myRequest.setOpt(ws);
                myRequest.perform();
                std::string htmlCode = htmlStream.str();

                //std::cout << htmlCode << std::endl;
                while (htmlCode.find("Checking your browser before accessing") != std::string::npos) {
                    std::cout << "Handling DDOS protection..." << std::endl;
                    std::cout << "Manually access the website and copy the contents of the cf_clearance cookie"
                              << std::endl;
                    std::cout << "  On FireFox, F12 > Network. Find the GET request for the page, Right Click > Copy > Copy as cURL. Copy the cookies parameter." << std::endl;

                    // This is not enough to bypass cloudflare... TODO
                    exit(1);
                    // wait
                    //sleep(1);

                    //myRequest.perform();
                    //std::string htmlCode = htmlStream.str();

                }
                //std::cout << htmlCode << std::endl;

                // check if chapter exists
                std::string chapterExists;
                if (mangareader_link) {
                    chapterExists = std::string("is not released yet.");
                } else {
                    chapterExists = std::string("is not available yet.");
                }
                if (htmlCode.find(chapterExists) != std::string::npos) {
                    // delete created folder
                    boost::filesystem::remove(chapterDir);
                    endChapter = currChapter;
                    //std::cout << "Chapter unavailable" << std::endl;
                    //break;
                }


                // format page numbers
                char *currPageStr = new char[4];
                sprintf(currPageStr, "%03d", currPage);

                // Get and save image
                std::string imageLink;
                if (mangareader_link)
                    imageLink = mangastream_image(htmlCode);
                else
                    imageLink = readmanga_image(htmlCode, currPage);
                if (imageLink.empty()){
                    std::cout << "Empty image" << std::endl;
                    break;
                }

                std::cout << imageLink << std::endl;
                myRequestJpg.setOpt<curlpp::options::Url>(imageLink);

                myRequestJpg.setOpt(curlpp::options::UserAgent(std::string(
                        "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:72.0) Gecko/20100101 Firefox/72.0")));
                myRequestJpg.setOpt(curlpp::options::Cookie(std::string(
                        "Cookie: __cfduid=dce7d571085ddb5906dd36bb8bfcff9551580944641; ci_session=iRJr2RsPUVUcrzgqyXq6MFYlGuRCY1n8TNBWS8JWB%2BCNiQb752rGMUFI4bj4SmbQh333iG3%2BGcx4zMZa1SLMt0uatOIgBjWgdcXbIrK8GCu5robNWUH1LcY3QyYo0DM43OHa5%2BD%2BkYVTjaB2YfnYlRzBCGkX7B1o4H7jFmKNqpSsyXeL7XfKAg9UzzhHjzGsPt28rQc%2BJacuZj3s30pRKMv9KK9A5HV5VtPqKe955ZEyyvnmVVeTocNy8DZMDIbGScoo0PWD7E0YvyJV5AUxX6P23s%2B83GuBPi0cTcIG2LB%2FBf1YViP8GIUbAtq4luMFYJLLsQOQN03UX3QwCKv6loTj%2FIFwUN%2BBB0zbtxfGtj8VScKycVUgKThQapKR8EhxC%2Fq12QEkl2bXsqWszfW0pXyqEn7sOqZQzjYRy9oDc2w%3D; _ga=GA1.2.257006594.1580944644; _gid=GA1.2.1622360435.1580944644; OB-USER-TOKEN=3c134b0f-d2dd-4220-83e6-430ba7ce9d37; _awl=2.1580945930.0.4-6a365285-a98b5de77a6c926b649cf7751b68ee68-6763652d6575726f70652d7765737431-5e3b520a-1; cf_clearance=38d853a3621b470a742635b1698773e1c3ec0b3b-1580946598-0-150")));


                std::ofstream outputFile(chapterDir + std::string(currPageStr) + ".jpg");
                curlpp::options::WriteStream wsJpg(&outputFile);
                myRequestJpg.setOpt(wsJpg);
                myRequestJpg.perform();
                outputFile.close();
            } catch (curlpp::RuntimeError &e) {
                std::cerr << e.what() << std::endl;
            } catch (curlpp::LogicError &e) {
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

void test2() {

    float value = 3.14;
    //std::cin >> value;
    std::array<int, 3> arr = {9, 8, 7};
    arr[1] = static_cast<int>(value);
    std::cout << "Array size = " << arr.size() << std::endl;
    std::cout << "2nd element = " << arr[1] << std::endl;

    for (int i = 0; i < 5; ++i) {
        std::cout << "This is silly." << std::endl;
    }

    int *myIntegerPointer = nullptr;
    if (!myIntegerPointer) {
        std::cout << "Empty pointer" << std::endl;
    }
    myIntegerPointer = new int;
    delete myIntegerPointer;
    myIntegerPointer = nullptr;

    auto myIntegerSmartPointer = std::make_unique<int>(1);
    std::cout << "Smart pointer " << *myIntegerSmartPointer << std::endl;

    int arraySize = 8;
    int *myVariableSizedArray = new int[arraySize];
    delete[] myVariableSizedArray;
    myVariableSizedArray = nullptr;

    try {
        Trip myTrip("OPO", "LIS", 2);
    } catch (const std::invalid_argument &exception) {
        std::cout << "Error making trip" << std::endl;
    }

    std::time_t result = std::time(nullptr);
    std::cout << std::asctime(std::localtime(&result));
    std::cout << "wat" << std::endl;

}

/*
void test(){
    URI uri("http://pocoproject.org/images/front_banner.jpg");
    std::string path(uri.getPathAndQuery());
    if (path.empty()) path = "/";
    HTTPClientSession session(uri.getHost(), uri.getPort());
    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    if (!doRequest(session, request, response))
    {
        std::cerr << "Invalid username or password" << std::endl;
        return 1;
    }
}

bool doRequest(Poco::Net::HTTPClientSession& session, Poco::Net::HTTPRequest& request,              Poco::Net::HTTPResponse& response)
{
    session.sendRequest(request);
    std::istream& rs = session.receiveResponse(response);
    std::cout << response.getStatus() << " " << response.getReason() << std::endl;
    if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
        std::ofstream ofs("Poco_banner.jpg",std::fstream::binary);
        StreamCopier::copyStream(rs, ofs);
        return true;
    }
    else
    {
        //it went wrong ?
        return false;
    }
}*/

void mysteryFunction(const int *ptrToImmutableVal, int &referenceToVal, const int &referenceToImmutableVal) {
    referenceToVal = 1;
    throw std::invalid_argument("Denominator cannot be 0.");
}