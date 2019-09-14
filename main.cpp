#include <iostream>
#include <fstream>
#include "opencv2/opencv.hpp"
#include <functional>
#include <chrono>
#include "signal.h"
//just use opencv init to avoid boost dependency
//#include <boost/property_tree/ptree.hpp>
//include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <experimental/filesystem>
#include <random>
#include "termios.h"
#include <thread>
#include "unistd.h"
#include "exif.h"
#include <sys/stat.h>
//use to determine if opencv is going to fix for us
enum ImageOrientation
{
    IMAGE_ORIENTATION_NORM = 1,//Dc
    IMAGE_ORIENTATION_MIRROR_HORIZONTAL = 2,//DC
    IMAGE_ORIENTATION_180 = 3,//DC
    IMAGE_ORIENTATION_MIRROR_VERTICAL = 4,//DC
    IMAGE_ORIENTATION_MIRHOR_R270 = 5,//SWITCH
    IMAGE_ORIENTATION_ROT90 = 6,//SWITCH
    IMAGE_ORIENTATION_MIRHOR_R90 = 7,//SWITCH
    IMAGE_ORIENTATION_ROT270 = 8//SWITCH
};
using namespace std::chrono;
using namespace std;
namespace fs=std::experimental::filesystem::v1;

static int key={0};
static std::mutex key_lock;
static bool paused = false;
bool isequals(const string& a, const string& b)
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}
static int nested={0};
//boost example, ignoring skip dirs b/c c++17 with gcc doesn't have no_push()
std::vector<std::string> getFileList(const std::string &dirPath,
                                     const std::vector<std::string> dirSkipList = { })
{

    // Create a vector of string
    std::vector<std::string> listOfFiles;
    try {
        // Check if given path exists and points to a directory
        if (fs::exists(dirPath) && fs::is_directory(dirPath))
        {
            // Create a Recursive Directory Iterator object and points to the starting of directory
            fs::recursive_directory_iterator iter(dirPath);

            // Create a Recursive Directory Iterator object pointing to end.
            fs::recursive_directory_iterator end;

            // Iterate till end
            while (iter != end)
            {
                // Check if current entry is a directory and if exists in skip list
                if (fs::is_directory(iter->path()) &&
                        (std::find(dirSkipList.begin(), dirSkipList.end(), iter->path().filename()) != dirSkipList.end()))
                {
                    // Skip the iteration of current directory pointed by iteraton
                    // Boost Fileystsem  API to skip current directory iteration
                    //iter.no_push();

                }
                else
                {
                    // Add the name in vector
                    std::string filename = iter->path().string();
                    listOfFiles.push_back(filename);
                }
                // Increment the iterator to point to next entry in recursive iteration
                iter++;
            }
        }
    }
    catch (std::system_error & e)
    {
        std::cerr << "Exception :: " << e.what();
    }
   // std::random_shuffle(listOfFiles.begin(),listOfFiles.end());
    return listOfFiles;
}
bool get_next_media(cv::Mat & img,
                    const std::vector<std::string> & images,
                    const std::vector<size_t> & indexes,
                    size_t & curPos,
                    int move)
{
    //only get pictures, one day list in json init
 //Standard mersenne_twister_engine seeded with rd()
    curPos+= move;
    if(curPos >= indexes.size()){
        move < 0? curPos = indexes.size()-1:curPos=0;
    }
    size_t file_index = indexes[curPos];
    while(!
        (isequals(images[file_index].substr(images[file_index].size()-3,3),"jpg") ||
            isequals(images[file_index].substr(images[file_index].size()-3,3),"bmp") ||
            isequals(images[file_index].substr(images[file_index].size()-3,3),"png") ||
            isequals(images[file_index].substr(images[file_index].size()-4,4),"tiff") ||
            isequals(images[file_index].substr(images[file_index].size()-3,3),"tif") ||
            isequals(images[file_index].substr(images[file_index].size()-3,3),"gif") ||
            isequals(images[file_index].substr(images[file_index].size()-4,4),"jpeg"))
           )
     {
        curPos+=move;
        if(curPos >= indexes.size()){
            move < 0? curPos = indexes.size()-1:curPos=0;
        }
        file_index = indexes[curPos];
    }
    img =  cv::imread(images[file_index]);

    return true;
}
bool generate_randoms(std::vector<size_t> & rnds, size_t max){
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
     std::uniform_int_distribution<unsigned int> dis(0, max);
    for(size_t i=0;i< max;i++){
        rnds.push_back(dis(gen));
    }
}
int handle_key(int keypress,size_t & pos,
               std::vector<std::string> & images,
               std::vector<size_t> indexes,
               cv::Mat & img,size_t & lastshown){
    int retval=-1;
    //key codes if keyboard is used
    const int STOP_KEY = 13;
    const int SKIP_FORWARD = 83;
    const int SKIP_BACK  = 81;
    const int QUIT = 27;
    if(keypress >0){
//        std::cout << std::setprecision(5)<< std::setw(5)
//                              <<std::to_string(keypress) << std::endl;
    }
    nested++;
    switch(keypress){

        case SKIP_FORWARD: //skip next images
            get_next_media(img,images,indexes,lastshown,1);
            pos=lastshown;
            //if we are paused, wait for another key
             retval = -1;
            break;
        case SKIP_BACK: //back one
            get_next_media(img,images,indexes,lastshown,-1);
            pos=lastshown;
            //if we are paused, wait for another key
             retval = -1;
            break;
        case STOP_KEY: //toggle pause play
           // paused = !paused;
            //while(paused){
                keypress = cv::waitKey(0);
        //		}
        break;
        case 27:
            exit(0);
        default://load next image, let it be shown on next loop
            get_next_media(img,images,indexes,pos,1);
            retval=0;
            break;
    }
    nested--;
    return retval;
}

static void* callback()
{
    while(true){
        timeval timeout;
        fd_set rdset;

        FD_ZERO(&rdset);
        FD_SET(STDIN_FILENO, &rdset);
        timeout.tv_sec  = 5000;
        timeout.tv_usec = 5000;
        key_lock.lock();
        key =  select(STDIN_FILENO + 1, &rdset, nullptr, nullptr, &timeout);
        key_lock.unlock();
    }
}
long get_file_size(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
int main(int argc, char *argv[])
{
    //std::thread shooter(callback);
    std::vector<size_t> indexes;
    //shooter.detach();
    int i=0;
    i++;
    const std::string keys =
        "{help h usage ? |      | print this message   }"
        "{path p            | /data/Pictures | picture path }"
        "{delay d            | 7000 | inter pic delay in ms }"
        "{debug b            | 0 | debug level }"
        "{hor h     | 1920 | horizontal resolution } "
        "{vert v     | 1200 | vertical resolution } "
        "{init               |  | json config file}"
        ;
    std::cout << (cv::getBuildInformation());
    cv::CommandLineParser parser(argc,argv,keys);
    std::string init = parser.get<std::string>("init");
    auto path=parser.get<std::string>("path");
    auto delay=parser.get<int>("delay");
    const auto screen_width=parser.get<int>("hor");
    const auto screen_height=parser.get<int>("vert");
    //boost::property_tree::ptree pt1;
    //boost::property_tree::read_json(init, pt1);
    //delay in milliseconds
   // auto path = pt1.get<std::string>("path");
   // auto delay = pt1.get<int>("delay");
    cv::namedWindow("win", cv::WINDOW_NORMAL);
    cv::setWindowProperty("win", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    //loop and display
    auto images = getFileList(path);
    std::cout << "Got " << images.size() << "files." << std::endl;
    int keypress = 0;
    cv::Mat img;
    if(!images.empty()){
        //load up and ordered random list
        generate_randoms(indexes,images.size());
         size_t pos=0,lastshown=0;
        get_next_media(img,images,indexes,pos,1);
        lastshown = pos;
       // std::cout << images[0];
        while(true){
            cv::Mat newimg;
            auto org_size = img.size();
            int tops=0,sides=0;
            if(org_size.width > screen_width || org_size.height > screen_height){
                double hor_ratio=1.0,vert_ratio=1.0;
                int newh =screen_height,neww=screen_width;
                double resize_ratio=1;
                easyexif::EXIFInfo result;
                ifstream inf( images[indexes[pos]].c_str() );
                bool swap= false;
                if( inf )
                {
                        unsigned char mDataBuffer[ 400048 ];
                        inf.read( (char*)( &mDataBuffer[0] ), 200048 ) ;
                        if(result.parseFrom(mDataBuffer,
                                            200048 )!= PARSE_EXIF_ERROR_NO_JPEG){
                            if(result.Orientation > 4 ){
                          //      std::swap(neww,newh);
                                swap=~swap;
                                std::cout << "swapping" << std::endl;
                            }
                        }
                        hor_ratio = (double)org_size.width/neww;
                        vert_ratio = (double)org_size.height/newh;
                        if(hor_ratio > vert_ratio){
                            resize_ratio = hor_ratio;
                        }else{
                            resize_ratio= vert_ratio;
                        }
                        cv::resize(img,newimg,cv::Size(org_size.width/resize_ratio,org_size.height/resize_ratio),0,0,cv::INTER_LANCZOS4);
                        sides = (neww - newimg.size().width)/2;
                        tops = (newh - newimg.size().height)/2;
                }
            }else{
                newimg = img;
                sides = (screen_width - newimg.size().width)/2;
                tops = (screen_height - newimg.size().height)/2;

            }
            //lazy bit so we can use high gui and still have a black background
            cv::Mat borderimg;
            cv::Scalar col(0,0,0);
            if(tops<0)tops=0;
            if(sides<0)sides =0;
            cv::copyMakeBorder(newimg,borderimg, tops, tops, sides, sides,cv::BORDER_CONSTANT,col);
            cv::imshow("win", borderimg);
            lastshown = pos;
            keypress = cv::waitKey(1);
             auto start = std::chrono::system_clock::now();
            key_lock.lock();   // auto path = pt1.get<std::string>("path");
            // auto delay = pt1.get<int>("delay");

            keypress = key;
            key=0;
            key_lock.unlock();
             if(handle_key(keypress,pos,images,indexes,img,lastshown)==-1) continue;
            auto finish = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now() - start);
            if(delay-finish.count()> 0){
                keypress = cv::waitKey(delay-finish.count());
            }else{
                key_lock.lock();
                keypress= key;
                key=0;
                key_lock.unlock();
            }
            if(keypress!=-1) handle_key(keypress,pos,images,indexes,img,lastshown);

        }
    }
}
