A manga crawler from MangaReader and ReadMng.

You might need to install dependencies:

    sudo apt-get install libboost-system-dev libboost-filesystem-dev libzip-dev libcurlpp-dev libcurl4-nss-dev

To build

    mkdir build
    cd build
    cmake ..
    make

And run with something like

    ./Trawler http://www.mangareader.net/soul-eater 


Or use options

    ./Trawler http://www.mangareader.net/soul-eater --no-compress --start 1 --end 100 --save /tmp

Or use options

    ./Trawler https://www.readmng.com/dragon-ball/ --no-compress --start 277


