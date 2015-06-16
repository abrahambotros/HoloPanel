#include "com_abrahambotros_holopanel_MainActivity.h"
#include "MainActivity.hpp"

/*
int main() {
	cout << "Begin main()" << endl;

	// createNativeController
	createNativeController();

	// calibrate on first frame
	initializeHelper();

	// play test video
	playTestVideo();

	// wait then destroy all windows
	cv::waitKey(0);
	cv::destroyAllWindows();


	// finish
	cout << "Done." << endl;
	return 0;
}
*/

/*
void initializeHelper() {

	// prepare window to show first frame for dev
	cv::namedWindow(WINDOW_INITIAL_FRAME, CV_WINDOW_AUTOSIZE);

	// read first frame from video
	cv::VideoCapture video(VIDEO_FILENAME);
    cv::Mat frame;
	if (video.isOpened()) {
		cout << "InitializeHelper(): Test video opened successfully." << endl;
		video >> frame;
		if (!frame.empty()) {
			cout << "InitializeHelper(): Showing first frame." << endl;
			cv::imshow(WINDOW_INITIAL_FRAME, frame);
			//cv::waitKey(0);
		} else {
			cout << "InitializeHelper(): Got empty first frame." << endl;
			return;
		}
	}

	// initialize on first frame
	//cout << "InitializeHelper(): Calling initialize()." << endl;
	//initialize(frame);
	handleFrame(frame, true);
}

void playTestVideo() {
	cv::namedWindow(WINDOW_VIDEO, CV_WINDOW_AUTOSIZE);
	cv::VideoCapture video(VIDEO_FILENAME);
	if (video.isOpened()) {
		cout << "Test video opened successfully." << endl;
		bool firstFramePlayed = false;
		while (1) {

			// get frame
			cv::Mat frame;
			video >> frame;
			if (!frame.empty()) {
				assert(frame.rows == _hp::IMAGE_HEIGHT && frame.cols == _hp::IMAGE_WIDTH);
				handleFrame(frame, true, false);
				cv::imshow(WINDOW_VIDEO, frame);
				if (firstFramePlayed == false) {
					cv::waitKey(0);
					firstFramePlayed = true;
				}
				//cv::waitKey(0);
				if (cv::waitKey(VIDEO_FRAME_INTERVAL) >= 0) break;
			} else {
				cout << "frame.empty()" << endl;
				break;
			}

			// pass frame to handler
			//handleFrame(frame, true, true);
		}
	} else {
		cout << "Failed to open test video." << endl;
	}
	cout << "Finished test video." << endl;
}
*/

JNIEXPORT jlong JNICALL Java_com_abrahambotros_holopanel_MainActivity_createNativeController
  (JNIEnv *, jobject)
{
//long createNativeController() {
	// create app instance
	app = new (_hp::HoloPanelApp);

	// return address
	return (long) app;
}


JNIEXPORT void JNICALL Java_com_abrahambotros_holopanel_MainActivity_destroyNativeController
  (JNIEnv *, jobject, jlong addr_native_controller)
{
//void destroyNativeController(long addr_native_controller) {
	delete (_hp::HoloPanelApp*)(addr_native_controller);
}

JNIEXPORT int JNICALL Java_com_abrahambotros_holopanel_MainActivity_handleFrame
  (JNIEnv *, jobject, jlong addr_native_controller, jlong addr_frame, jboolean receivedInitialTouch, jboolean requiresInit)
{
	_hp::HoloPanelApp* app = (_hp::HoloPanelApp*)(addr_native_controller);
	cv::Mat* frame = (cv::Mat*)(addr_frame);
	if (!receivedInitialTouch) {
		return app->showCalibrationWindow(*frame);
	}
	else if (requiresInit) {
		cout << "initialize()" << endl;
		return app->initialize(*frame);
	} else {
		return app->handleFrame(*frame);
	}
}
/*
int handleFrame(cv::Mat& frame, const bool requiresInit) {
	if (requiresInit) {
		cout << "initialize()" << endl;
		return app->initialize(frame);
	} else {
		return app->handleFrame(frame);
	}
}
*/
