#include <iostream>
#include "opencv2/opencv.hpp"
#include "boost/foreach.hpp"
#include "boost/chrono.hpp"
#include "boost/thread/thread.hpp"
#include "boost/atomic.hpp"
#include <functional>
#include <chrono>
#include "signal.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "boost/filesystem.hpp"
#include <chrono>
using namespace std::chrono;
using namespace std;
namespace fs=boost::filesystem;

pthread_t thread;
int key={0};
boost::mutex key_lock;
bool paused = false;
std::vector<std::string> getFileList(const std::string &dirPath, 	const std::vector<std::string> dirSkipList = { })
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
                    iter.no_push();

                }
                else
                {
                    // Add the name in vector
                    std::string filename = iter->path().string();
                    if(filename.compare(filename.size()-3,3,"jpg")==0
                            || filename.compare(filename.size()-3,3,"bmp" )==0
                            || filename.compare(filename.size()-3,3,"png" )==0
                            || filename.compare(filename.size()-3,3,"gif" )==0)
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
    const int STOP_KEY = 32;
    const int SKIP_FORWARD = 83;
    const int SKIP_BACK  = 81;
    if(keypress !=0)std::cout << keypress << std::endl;
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
                if( handle_key(keypress,position,images,img,lastshown)==-1)cv::imshow("window", img);
            }
        break;
        default://load next image, let it be shown on next loop
            position++;
            if(position >= images.size())position=0;
            img = cv::imread((images)[position]);
            retval=0;
            break;
    }
    return retval;
}

static void* callback()
{
    while(true){
        int keypress = cv::waitKey(0);
        key_lock.lock();
        key = keypress;
        key_lock.unlock();
    }
}
int main(int argc, char *argv[])
{
    const std::string keys =
        "{help h usage ? |      | print this message   }"
        "{names   n     |   /home/mark/cfg/obj.names   | files for names   }"
        "{config   c     | /home/mark/cfg/yolo-obj.cfg | config file   }"
        "{weights   w     | /home/mark/cfg/yolo-obj_10000.weights     | weights  }"
        "{path  p         | /srv/ftp/ftp/  | path to save files   }"
        "{image i           |  | image to analyze }"
        "{uid   u         | admin  | admin id for cam}"
        "{pwd            | newpw111 | pwd for cam }"
        "{thresh t            | .5 | threshold }"
        "{disabled d            | 1 | shooter Disabled }"
        "{debug b            | 0 | debug log }"
        "{init               |  | config file}"
        ;
    std::cout << (cv::getBuildInformation());
    cv::CommandLineParser parser(argc,argv,keys);
    std::string init = parser.get<std::string>("init");
    boost::property_tree::ptree pt1;
    boost::property_tree::read_json(init, pt1);
    //delay in milliseconds
    int imageSaveLevel=0,loglevel=0;
    auto path = pt1.get<std::string>("path");
    auto delay = pt1.get<int>("delay");
    cv::namedWindow("window", cv::WND_PROP_FULLSCREEN);
    cv::setWindowProperty("window",cv::WND_PROP_FULLSCREEN,cv::WINDOW_FULLSCREEN);
    //loop and display
    auto images = getFileList(path);
    int keypress = 0;
    cv::Mat img;
    if(!images.empty()){
        img = cv::imread(images[0]);
        size_t pos=0,lastshown=0;
        while(true){
            cv::imshow("window", img);
            lastshown = pos;
            keypress = cv::waitKey(1);
            key_lock.lock();
            key = keypress;
            key_lock.unlock();
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
