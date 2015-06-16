#ifndef MAIN_HPP_
#define MAIN_HPP_

#include <opencv2/opencv.hpp>
#include "HoloPanel/HoloPanelApp.hpp"

/*
int main();
long createNativeController();
void destroyNativeController(long addr_native_controller);
//void initialize(cv::Mat& initFrame);
void handleFrame(cv::Mat& frame, const bool receivedInitialTouch, const bool requiresInit);
*/

// possibly for both real and dev
_hp::HoloPanelApp* app;

// just for dev
//const string VIDEO_FILENAME = "assets/hand_test_wood_floor_background.mp4";
//const string VIDEO_FILENAME = "assets/hand_test_black_chair_background.mp4";
const string VIDEO_FILENAME = "assets/finger_test_black_chair_background.mp4";
const string WINDOW_VIDEO = "WINDOW_VIDEO";
const string WINDOW_INITIAL_FRAME = "WINDOW_INITIAL_FRAME";
const int VIDEO_FRAME_INTERVAL = 30;
void playTestVideo();
void initializeHelper();


#endif /* MAIN_HPP_ */
