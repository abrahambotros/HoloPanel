#include "Common.hpp"

namespace _hp {

	//template <typename T> string convertNumToString(const T input) {
	string convertIntToString(const int input) {
		ostringstream ss;
		ss << input;
		return ss.str();
	}

}
