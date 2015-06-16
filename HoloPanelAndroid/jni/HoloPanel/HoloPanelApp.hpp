#ifndef HOLOPANELAPP_HPP_
#define HOLOPANELAPP_HPP_

#include "Common.hpp"

namespace _hp {

	class HoloPanelApp {
	public:
		HoloPanelApp();

		virtual ~HoloPanelApp() = default;

		int showCalibrationWindow(cv::Mat& frame);

		int initialize(const cv::Mat& initFrame);

		int handleFrame(cv::Mat& frame);

		int main();

	private:

		// consts
		const bool IS_MOBILE = true;
		const int ZERO = 0;
		cv::Mat mask, tmp;
		// init
		cv::Mat grayscaleFrame, grayscaleMask;
		const double GRAYSCALE_BLACK_THRESHOLD = 10.0;
		// hsv
		int NUM_HISTOGRAM_BINS = 16;
		cv::Mat hue, hueCenter, hueMask;
		const int HUE_CHANNEL[2] = {0, 0};
		const float MIN_HUE_VALUE = 0.0, MAX_HUE_VALUE = 180.0;
		const float HUE_MINMAX_HELPER[2] = {MIN_HUE_VALUE, MAX_HUE_VALUE}; // see cv::calcHist documentation
		const float* HUE_MINMAX[1] = { HUE_MINMAX_HELPER }; // see cv::calcHist documentation
		const cv::Size HUE_BLUR_KERNEL_SIZE = cv::Size(21, 21);
		//const cv::Scalar HSV_LOWER_BOUND = cv::Scalar(0, 30, 10); // see Bradski98, https://github.com/Itseez/opencv/blob/2.4.8.x-prep/samples/cpp/camshiftdemo.cpp, https://github.com/Itseez/opencv/blob/2.4.8.x-prep/samples/python2/camshift.py
		const cv::Scalar HSV_LOWER_BOUND = cv::Scalar(0, 60, 32); // see Bradski98, https://github.com/Itseez/opencv/blob/2.4.8.x-prep/samples/cpp/camshiftdemo.cpp, https://github.com/Itseez/opencv/blob/2.4.8.x-prep/samples/python2/camshift.py
		const cv::Scalar HSV_UPPER_BOUND = cv::Scalar(180, 256, 256); // see Bradski98, https://github.com/Itseez/opencv/blob/2.4.8.x-prep/samples/cpp/camshiftdemo.cpp, https://github.com/Itseez/opencv/blob/2.4.8.x-prep/samples/python2/camshift.py
		const int HSV_ROUNDING_DIVISOR = 16;
		const float START_CENTER_FRACTION = 0.2,
				END_CENTER_FRACTION = 0.8;
		const int FARTHEST_CORNER_IMAGE_EDGE_THRESHOLD = 30; // TODO: make based on image size!
		// back projections
		cv::Mat referenceHist, newHist;
		cv::Mat backProjectionProbabilities, backProjectionProbabilities_copy;
		const double BACK_PROJECTION_THRESHOLD = 0.50 * MAX_HUE_VALUE;
		const cv::Size BACK_PROJECTION_BLUR_KERNEL_SIZE = cv::Size(21, 21);
		// floodfill
		const int FLOODFILL_CANNY_THRESHOLD1 = 0, // 0
				FLOODFILL_CANNY_THRESHOLD2 = 200, // 400
				FLOODFILL_CANNY_APERTURE_SIZE = 3, // 5
				FLOODFILL_NEWVAL = 255,
				FLOODFILL_LODIFF = 30,
				FLOODFILL_UPDIFF = 30;
		// convex hull, convexity defects
		bool hullFound;
		int hullSize, largestHullSize, largestHullIndex, numFingerDefects;
		const int MIN_HULL_SIZE = (0.15 * IMAGE_WIDTH) * (0.15 * IMAGE_HEIGHT),
				MAX_HULL_SIZE = 0.8 * IMAGE_WIDTH * IMAGE_HEIGHT; // TODO: smaller? or restrict each dimension individually so not full height or width?
		const double CONVEXITY_DEFECT_DEPTH_THRESHOLD = 70; // TODO: make based on image size!
		const int CONVEXITY_IMAGE_EDGE_THRESHOLD = 30; // TODO: make based on image size!
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;
		vector<cv::Vec4i> convexityDefects; // TODO: find struct CvConvexityDefect class?
		vector<vector<cv::Point> > hulls_points;
		vector<vector<int> > hulls_indices;
		cv::Moments convexHullMoments;
		cv::Point2i convexHullCentroid;
		cv::Point2i mainFingerLocation;
		// camshift
		cv::Rect camshiftSearchWindow;
		cv::RotatedRect camshiftFoundWindow;
		cv::TermCriteria CAMSHIFT_TERM_CRITERIA = cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 10, 1);
		// panel
		const int NUM_PANEL_COLUMNS = 3,
				NUM_PANEL_ROWS = 6,
				PANEL_COLUMN_WIDTH = IMAGE_WIDTH / NUM_PANEL_COLUMNS,
				PANEL_ROW_HEIGHT = IMAGE_HEIGHT / NUM_PANEL_ROWS,
				PANEL_COLUMN_PADDING = IMAGE_WIDTH / 10,
				PANEL_OBJECT_THICKNESS = 1,
				PANEL_OBJECT_LINE_TYPE = 8,
				PANEL_OBJECT_SHIFT = 0,
				PANEL_CIRCLE_RADIUS = PANEL_ROW_HEIGHT;
#if IS_MOBILE
		const cv::Scalar PANEL_BORDER_COLOR = cv::Scalar(71, 181, 237),
				PANEL_BORDER_SELECTED_COLOR = cv::Scalar(255, 0, 0);
#else
		const cv::Scalar PANEL_BORDER_COLOR = cv::Scalar(237, 181, 71),
				PANEL_BORDER_SELECTED_COLOR = cv::Scalar(0, 0, 255);
#endif
		/*
		const cv::Rect TOP_LEFT_RECT = cv::Rect(0, 0, PANEL_COLUMN_WIDTH - PANEL_COLUMN_PADDING, PANEL_ROW_HEIGHT),
				TOP_CENTER_RECT = cv::Rect(PANEL_COLUMN_WIDTH + PANEL_COLUMN_PADDING, 0, PANEL_COLUMN_WIDTH - 2*PANEL_COLUMN_PADDING, PANEL_ROW_HEIGHT),
				TOP_RIGHT_RECT = cv::Rect(IMAGE_WIDTH - PANEL_COLUMN_WIDTH + PANEL_COLUMN_PADDING, 0, PANEL_COLUMN_WIDTH - PANEL_COLUMN_PADDING, PANEL_ROW_HEIGHT);
		*/
		const cv::Point2i TOP_LEFT_CIRCLE_CENTER = cv::Point2i(0.5 * PANEL_COLUMN_WIDTH, PANEL_ROW_HEIGHT/2),
				TOP_CENTER_CIRCLE_CENTER = cv::Point2i(IMAGE_WIDTH / 2, PANEL_ROW_HEIGHT/2),
				TOP_RIGHT_CIRCLE_CENTER = cv::Point2i(IMAGE_WIDTH - 0.5 * PANEL_COLUMN_WIDTH, PANEL_ROW_HEIGHT/2),
				BOTTOM_LEFT_CIRCLE_CENTER = cv::Point2i(PANEL_COLUMN_WIDTH - PANEL_COLUMN_PADDING, IMAGE_HEIGHT - PANEL_ROW_HEIGHT/2),
				BOTTOM_RIGHT_CIRCLE_CENTER = cv::Point2i(2*PANEL_COLUMN_WIDTH + PANEL_COLUMN_PADDING, IMAGE_HEIGHT - PANEL_ROW_HEIGHT/2);
		const int PANEL_LABEL_NONE = 0,
				PANEL_LABEL_TOP_LEFT_CIRCLE = 1,
				PANEL_LABEL_TOP_CENTER_CIRCLE = 2,
				PANEL_LABEL_TOP_RIGHT_CIRCLE = 3,
				PANEL_LABEL_BOTTOM_LEFT_CIRCLE = 4,
				PANEL_LABEL_BOTTOM_RIGHT_CIRCLE = 5;
		int panelSelection = PANEL_LABEL_NONE,
				previousPanelSelection = PANEL_LABEL_NONE,
				consecutivePanelSelections = 0;
		bool triggerPanelAction = false;
		const int MIN_CONSECUTIVE_PANEL_SELECTIONS = 5;

		// core functions
		void getHue(const cv::Mat& frame, cv::Mat& hue);
		cv::Mat getCenterRoi(const cv::Mat& input, int* startWidth, int* startHeight, int* centerWidth, int* centerHeight);
		cv::Mat getFloodFillMask(const cv::Mat& input, const string windowName, const bool convertFlag=false, const int convertCode=-1);
		void computeHistogram(const cv::Mat& input, cv::Mat& output, cv::InputArray mask=cv::noArray());
		void computeBackProjectionProbabilities(const cv::Mat& input);
		void addRGBEdgesToBackProjectionProbabilities(const cv::Mat& rgbInput);
		void findConvexHull();
		void getHullCentroid(cv::Mat& frame);
		void findFarthestHullPeak(cv::Mat& frame);
		void displayNumFingers(cv::Mat& frame);
		void updateCamshift();
		// panel functions
		void displayPanelSkeleton(cv::Mat& frame);
		void computePanelSelection(cv::Mat& frame);
		void displayPanelSelection(cv::Mat& frame);
		void processPanelSelection();

		// dev
		const string WINDOW_INITIAL_FRAME = "WINDOW_INITIAL_FRAME",
				WINDOW_INITIAL_GRAYSCALE = "WINDOW_INITIAL_GRAYSCALE",
				WINDOW_INITIAL_HISTOGRAM = "WINDOW_INITIAL_HISTOGRAM",
				WINDOW_INITIAL_CENTER = "WINDOW_INITIAL_CENTER",
				WINDOW_INITIAL_HUE = "WINDOW_INITIAL_HUE",
				WINDOW_INITIAL_FLOODFILL = "WINDOW_INITIAL_FLOODFILL",
				WINDOW_FRAME = "WINDOW_FRAME",
				WINDOW_HUE = "WINDOW_HUE",
				WINDOW_NEW_HISTOGRAM = "WINDOW_NEW_HISTOGRAM",
				WINDOW_BACK_PROJECTIONS = "WINDOW_BACK_PROJECTIONS",
				WINDOW_INITIAL_BACK_PROJECTIONS = "WINDOW_INITIAL_BACK_PROJECTIONS",
				WINDOW_INITIAL_CONTOURS = "WINDOW_INITIAL_CONTOURS",
				WINDOW_INITIAL_CAMSHIFT = "WINDOW_INITIAL_CAMSHIFT",
				WINDOW_GRAYSCALE = "WINDOW_GRAYSCALE",
				WINDOW_CAMSHIFT = "WINDOW_CAMSHIFT",
				WINDOW_CONVEX_HULL = "WINDOW_CONVEX_HULL";
		// histogram
		const int HIST_IMAGE_ROWS = 200, HIST_IMAGE_COLS = 320;
		cv::Mat histImage = cv::Mat::ones(HIST_IMAGE_ROWS, HIST_IMAGE_COLS, CV_8U);
		const cv::Scalar HIST_IMAGE_255_SCALAR = cv::Scalar::all(255);
		const cv::Scalar HIST_IMAGE_ZEROS_SCALAR = cv::Scalar::all(0);
		const int HIST_IMAGE_BIN_WIDTH = cvRound( (double) HIST_IMAGE_COLS / NUM_HISTOGRAM_BINS );
		// convex hull
		cv::Mat convexHullImage;

		// dev: contours
		//cv::RNG CONTOUR_COLOR_RANGE(12345);

		void displayHistogram(const cv::Mat& histogram, const string windowName);
		void displayBackProjectionsImage(const string windowName);
		void displayContours(const cv::Mat& input, const string windowName);
		void resetLargestHullImage();
		void displayLargestHull(cv::Mat& frame);
		void getConvexityDefects(cv::Mat& frame);
		void displayCamshiftWindow(const string windowName);

	};
}


#endif /* HOLOPANELAPP_HPP_ */
