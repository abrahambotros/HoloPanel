#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
//#include <android/log.h>
using namespace std;

namespace _hp {
	const int IMAGE_WIDTH = 720, IMAGE_HEIGHT = 480;

	// dev
	//template <typename T> string convertNumToString(const T input);
	string convertIntToString(const int input);
}




#endif /* COMMON_HPP_ */
