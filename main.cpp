#include <iostream>
#include "opencv2/opencv.hpp"
#include <functional>
#include <chrono>
#include "signal.h"
//just use opencv init to avoid boost dependency
//#include <boost/property_tree/ptree.hpp>
//include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <experimental/filesystem>
#include "termios.h"
#include <thread>
#include "unistd.h"
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
                    //only get pictures, one day list in json init
                    if(    isequals(filename.substr(filename.size()-3,3),"jpg")==0 ||
                            isequals(filename.substr(filename.size()-3,3),"bmp")==0 ||
                            isequals(filename.substr(filename.size()-3,3),"png")==0 ||
                            isequals(filename.substr(filename.size()-4,4),"tiff")==0 ||
                            isequals(filename.substr(filename.size()-3,3),"tif")==0 ||
                            isequals(filename.substr(filename.size()-3,3),"gif")==0 ||
                            isequals(filename.substr(filename.size()-4,4),"jpeg")==0
                           )
                     {
                    listOfFiles.push_back(filename);
                    }
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
    std::random_shuffle(listOfFiles.begin(),listOfFiles.end());
    return listOfFiles;
}

int handle_key(int keypress,size_t & position,std::vector<std::string> & images,
               cv::Mat & img,size_t lastshown){
    int retval=-1;
    //key codes if keyboard is used
    const int STOP_KEY = 13;
    const int SKIP_FORWARD = 83;
    const int SKIP_BACK  = 81;
    const int QUIT = 27;
    if(keypress >0){
        std::cout << std::setprecision(5)<< std::setw(5)
                              <<std::to_string(keypress) << std::endl;
    }
    switch(keypress){

        case SKIP_FORWARD: //skip next images
            if((position)<=images.size()) (position) = lastshown+1;
            else position=0;
            img = cv::imread((images)[position]);
            //if we are paused, wait for another key
            if(!paused) retval = -1;
            break;
        case SKIP_BACK: //back one
            position = lastshown-1;
            if((position) >= images.size())position = images.size()-1;
            img = cv::imread((images)[(position)]);
            //if we are paused, wait for another key
            if(paused) retval = -1;
            break;
        case STOP_KEY: //toggle pause play
            paused = !paused;
            while(paused){
                keypress = cv::waitKey(0);
                key_lock.lock();
                keypress = key;
                key=0;
                key_lock.unlock();
                if( handle_key(keypress,position,images,img,lastshown)==-1)cv::imshow("window", img);
            }
        break;
        case 27:
            exit(0);
        default://load next image, let it be shown on next loop
            position++;
            if(position >= images.size())position=0;
       //     std::cout << images[position] << endl;
            img = cv::imread((images)[position]);
            retval=0;
            break;
    }
    return retval;
}

static void* callback()
{
    while(true){
        timeval timeout;
        fd_set rdset;

        FD_ZERO(&rdset);
        FD_SET(STDIN_FILENO, &rdset);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;
        key_lock.lock();
        key =  select(STDIN_FILENO + 1, &rdset, NULL, NULL, &timeout);
        key_lock.unlock();
    }
}
int main(int argc, char *argv[])
{
    std::thread shooter(callback);
    shooter.detach();
    int i=0;
    i++;
    const std::string keys =
        "{help h usage ? |      | print this message   }"
        "{path p            | /data/Pictures | picture path }"
        "{delay d            | 5000 | inter pic delay in ms }"
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
    auto hor=parser.get<int>("hor");
    auto vert=parser.get<int>("vert");
    //boost::property_tree::ptree pt1;
    //boost::property_tree::read_json(init, pt1);
    //delay in milliseconds
   // auto path = pt1.get<std::string>("path");
   // auto delay = pt1.get<int>("delay");
//    cv::namedWindow("win", cv::WINDOW_NORMAL);
//    cv::setWindowProperty("win", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    //loop and display
    auto images = getFileList(path);
    int keypress = 0;
    cv::Mat img;
    if(!images.empty()){
        img = cv::imread(images[0]);
       // std::cout << images[0];
        size_t pos=0,lastshown=0;
        while(true){
            cv::Mat newimg;
            auto org_size = img.size();
            if(org_size.width > hor && org_size.height > vert){
                auto hor_ratio = (float)org_size.width/hor;
                auto vert_ratio = (float)org_size.height/vert;
                float resize_ratio=0;
                hor_ratio > vert_ratio?resize_ratio = hor_ratio:resize_ratio=vert_ratio;
                cv::resize(img,newimg,cv::Size(org_size.width/resize_ratio,org_size.height/resize_ratio),0,0,cv::INTER_LANCZOS4);
            }
 //           cv::imshow("win", newimg);
            lastshown = pos;
            keypress = cv::waitKey(1);
            auto start = std::chrono::system_clock::now();
            key_lock.lock();
            keypress = key;
            key=0;
            key_lock.unlock();
             if(handle_key(keypress,pos,images,img,lastshown)==-1) continue;
            auto finish = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now() - start);
            if(delay-finish.count()> 0){
                keypress = cv::waitKey(delay-finish.count());
            }else{
                key_lock.lock();
                keypress= key;
                key=0;
                key_lock.unlock();
            }
            if(keypress!=0) handle_key(keypress,pos,images,img,lastshown);

        }
    }
}
