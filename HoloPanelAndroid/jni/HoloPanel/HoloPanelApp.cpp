#include "HoloPanelApp.hpp"
//#include "HoloPanel/HoloPanelApp.hpp"

/*
 * TODO:
 * - draw 6 boxes on handleFrame
 * - color image segmentation using mean shift http://stackoverflow.com/questions/4831813/image-segmentation-using-mean-shift-explained/4835340#4835340
 * - watershed segmentation? https://opencv-python-tutroals.readthedocs.org/en/latest/py_tutorials/py_imgproc/py_watershed/py_watershed.html#watershed
 * - try using 2-dim histogram (hue + saturation)
 */

/*
 * Overview:
 * - Initialization assumes black background, takes center box hues
 */

namespace _hp {

	HoloPanelApp::HoloPanelApp() {

	}

	// TODO: precompute calibration ROI dimensions, move out of camera loop
	int HoloPanelApp::showCalibrationWindow(cv::Mat& frame) {
		// draw rectangle around center roi
		int startWidth, startHeight, centerWidth, centerHeight;
		getCenterRoi(frame, &startWidth, &startHeight, &centerWidth, &centerHeight);
		cv::rectangle(frame, cv::Rect(startWidth, startHeight, centerWidth, centerHeight), PANEL_BORDER_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);

		// return PANEL_LABEL_NONE since no panel selection
		return PANEL_LABEL_NONE;
	}

	// - frames are of type CV_8UC3
	// - input frames are RGBA for initialize and handleFrame
	int HoloPanelApp::initialize(const cv::Mat& initFrame) {

		// assert
		assert(initFrame.type() == CV_8UC3 || initFrame.type() == CV_8UC4);
		assert(initFrame.rows == IMAGE_HEIGHT && initFrame.cols == IMAGE_WIDTH);

		// reset
		hue.release();
		hueCenter.release();
		referenceHist.release();
		backProjectionProbabilities.release();

		// get hue and hueMask
		getHue(initFrame, hue);
		if (!IS_MOBILE) {
			cv::namedWindow(WINDOW_INITIAL_HUE);
			cv::imshow(WINDOW_INITIAL_HUE, hue);
		}

		// get center ROI
		int startWidth, startHeight, centerWidth, centerHeight;
		hueCenter = getCenterRoi(hue, &startWidth, &startHeight, &centerWidth, &centerHeight);
		if (!IS_MOBILE) {
			cv::namedWindow(WINDOW_INITIAL_CENTER);
			cv::imshow(WINDOW_INITIAL_CENTER, hueCenter);
		}

		// convert initFrame to grayscale, mask black colors out, and get center ROI of grayscaleMask
		cv::cvtColor(initFrame, grayscaleFrame, CV_RGB2GRAY);
		cv::threshold(grayscaleFrame, grayscaleMask, GRAYSCALE_BLACK_THRESHOLD, 255.0, CV_THRESH_BINARY);
		cv::erode(grayscaleMask, grayscaleMask, cv::Mat());
		cv::erode(grayscaleMask, grayscaleMask, cv::Mat());
		cv::erode(grayscaleMask, grayscaleMask, cv::Mat());
		cv::Mat grayscaleMaskCenter(grayscaleMask, cv::Rect(startWidth, startHeight, centerWidth, centerHeight));
		if (!IS_MOBILE) {
			cv::namedWindow(WINDOW_INITIAL_GRAYSCALE);
			cv::imshow(WINDOW_INITIAL_GRAYSCALE, grayscaleMask);
		}

		// compute histogram using grayscaleMask to mask out black background
		computeHistogram(hueCenter, referenceHist, grayscaleMaskCenter);

		// display histogram
		if (!IS_MOBILE) {
			cv::namedWindow(WINDOW_INITIAL_HISTOGRAM);
			displayHistogram(referenceHist, WINDOW_INITIAL_HISTOGRAM);
		}

		// display initial back-projection
		computeBackProjectionProbabilities(hue);
		if (!IS_MOBILE) {
			cv::namedWindow(WINDOW_INITIAL_BACK_PROJECTIONS);
			displayBackProjectionsImage(WINDOW_INITIAL_BACK_PROJECTIONS);
		}

		// initialize camshift
		if (!IS_MOBILE) {
			camshiftSearchWindow = cv::Rect(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
			camshiftFoundWindow = cv::CamShift(backProjectionProbabilities, camshiftSearchWindow, CAMSHIFT_TERM_CRITERIA);
			cv::namedWindow(WINDOW_INITIAL_CAMSHIFT);
			displayCamshiftWindow(WINDOW_INITIAL_CAMSHIFT);
		}

		if (!IS_MOBILE) {
			// draw contours
			displayContours(initFrame, WINDOW_INITIAL_CONTOURS);
		}

		// also create window for upcoming frames
		if (!IS_MOBILE) {
			cv::namedWindow(WINDOW_FRAME);
			cv::namedWindow(WINDOW_HUE);
			cv::namedWindow(WINDOW_NEW_HISTOGRAM);
			cv::namedWindow(WINDOW_BACK_PROJECTIONS);
			cv::namedWindow(WINDOW_CAMSHIFT);
			cv::namedWindow(WINDOW_GRAYSCALE);
		}

		// return PANEL_LABEL_NONE since no action
		return PANEL_LABEL_NONE;

	}


	// handle frames after initialization
	int HoloPanelApp::handleFrame(cv::Mat &frame) {

		// show initial frame
		if (!IS_MOBILE) {
			cv::imshow(WINDOW_FRAME, frame);
		}

		// get hue
		getHue(frame, hue);

		// show hue
		if (!IS_MOBILE) {
			cv::imshow(WINDOW_HUE, hue);
		}

		// show grayscale
		if (!IS_MOBILE) {
			cv::Mat grayscaleTemp;
			cv::cvtColor(frame, grayscaleTemp, CV_RGB2GRAY);
			//cv::threshold(grayscaleTemp, grayscaleTemp, GRAYSCALE_BLACK_THRESHOLD, 255.0, CV_THRESH_BINARY);
			cv::imshow(WINDOW_GRAYSCALE, grayscaleTemp);
		}

		// compute and display histogram
		if (!IS_MOBILE) {
			computeHistogram(hue, newHist, hueMask);
			displayHistogram(newHist, WINDOW_NEW_HISTOGRAM);
		}

		// compute back-projection of hueCenter onto referenceHist histogram, store in backProjectionProbabilities
		computeBackProjectionProbabilities(hue);
		// draw RGB edges on top of backProjectionProbabilities
		//addRGBEdgesToBackProjectionProbabilities(frame); // TODO: COMMENT OUT

		// display backprojections
		if (!IS_MOBILE) {
			displayBackProjectionsImage(WINDOW_BACK_PROJECTIONS);
		}

		// find convex hulls using contours on image-like backProjectionProbabilities
		findConvexHull();

		// if hull found
		if (hullFound) {

			// reset largestHullImage
			resetLargestHullImage();

			// draw largest hull
			displayLargestHull(frame);

			// get centroid of largest hull using moments
			getHullCentroid(frame);

			// isolate farthest convex hull corner/peak from convexHullCentroid as finger to track - must not be along edge of image
			findFarthestHullPeak(frame);

			// get and draw convexity defects and number of fingers
			getConvexityDefects(frame);

			// show hull image
			if (!IS_MOBILE) {
				cv::namedWindow(WINDOW_CONVEX_HULL);
				cv::imshow(WINDOW_CONVEX_HULL, convexHullImage);
			}
		}

		// display number of fingers, based on hullFound and numFingerDefects
		displayNumFingers(frame);

		// run camshift update - first move foundWindow to searchWindow, then perform camshift
		if (!IS_MOBILE) {
			updateCamshift();
		}


		// PANEL

		// processing
		if (!IS_MOBILE) { displayPanelSkeleton(frame); }
		computePanelSelection(frame);
		if (!IS_MOBILE) { displayPanelSelection(frame); }
		processPanelSelection();

		// set panel selection for next loop
		previousPanelSelection = panelSelection;

		// if processPanelSelection sets triggerPanelAction=true, then return panelSelection to Android
		if (triggerPanelAction) {
			return panelSelection;
		// otherwise, return PANEL_LABEL_NONE
		} else {
			return PANEL_LABEL_NONE;
		}

	}

	void HoloPanelApp::getHue(const cv::Mat& input, cv::Mat& output) {

		// for HSV/hue
		//input.copyTo(tmp);
		// convert to HSV
		//cv::cvtColor(input, tmp, CV_RGBA2RGB);
		//cv::cvtColor(tmp, tmp, CV_RGB2HSV);
		//cv::cvtColor(input, tmp, CV_RGBA2BGR);
		//cv::cvtColor(tmp, tmp, CV_BGR2HSV);
		cv::cvtColor(input, tmp, CV_RGB2HSV);
		//cv::cvtColor(input, tmp, CV_RGBA2GRAY);
		// mask on HSV thresholds
		cv::inRange(tmp, HSV_LOWER_BOUND, HSV_UPPER_BOUND, hueMask); // TODO: UNCOMMENT
		// extract only hue
		output.release();
		//output.create(tmp.size(), tmp.depth());
		output.create(tmp.size(), tmp.depth()); //CV_8UC1);
		cv::mixChannels(&tmp, 1, &output, 1, HUE_CHANNEL, 1);
		/*
		vector<cv::Mat> channels;
		cv::split(tmp, channels);
		output = channels[0];
		*/
		// bitwise-and mask with hue
		//cv::bitwise_and(output, mask, output); // TODO: UNCOMMENT
		//output &= mask;

		/*
		// print min and max
		double min, max;
		cv::minMaxLoc(output, &min, &max);
		cout << min << ", " << max << endl;
		*/

		/*
		// for GRAYSCALE
		cv::cvtColor(input, output, CV_RGB2GRAY);
		cv::threshold(output, output, 0, 255, cv::THRESH_TOZERO | cv::THRESH_OTSU);
		*/

		// smooth some more
		cv::GaussianBlur(output, output, HUE_BLUR_KERNEL_SIZE, 0);

		/*
		// convert to HSV
		cv::cvtColor(input, input, CV_RGB2HSV);

		// extract single hue channel
		output.create(input.size(), input.depth());
		cv::mixChannels(&input, 1, &output, 1, HUE_CHANNEL, 1);

		// smooth hue channel
		cv::GaussianBlur(output, output, BLUR_KERNEL_SIZE, 0);
		*/

		// mod by HSV_ROUNDING_DIVISOR: divide by HSV_ROUNDING_DIVISOR, then multiply back (int mat)
		//output = (output / HSV_ROUNDING_DIVISOR) * HSV_ROUNDING_DIVISOR;
		/*
		int x = (int) output.at<uchar>(IMAGE_HEIGHT/2, IMAGE_WIDTH/2);
		output /= HSV_ROUNDING_DIVISOR;
		output *= HSV_ROUNDING_DIVISOR;
		int y = (int) output.at<uchar>(IMAGE_HEIGHT/2, IMAGE_WIDTH/2);//(x/HSV_ROUNDING_DIVISOR) * HSV_ROUNDING_DIVISOR;
		cout << x << " -> " << y << endl;
		//cout << (int) output.at<uchar>(IMAGE_HEIGHT/2, IMAGE_WIDTH/2) << endl;
		 */
		/*
		output /= HSV_ROUNDING_DIVISOR;
		output.convertTo(output, CV_8U);
		output *= HSV_ROUNDING_DIVISOR;
		cout << output.at<double>(5, 5) << endl;
		*/

	}

	cv::Mat HoloPanelApp::getCenterRoi(const cv::Mat& input, int* startWidth, int* startHeight, int* centerWidth, int* centerHeight) {

		// get input dimensions
		cv::Size size = input.size();
		int height = size.height, width = size.width;
		//cout << "input.size = (" << height << ", " << width << ")" << endl;

		// select center region - center 50% height/width
		*startHeight = START_CENTER_FRACTION * height;
		int endHeight = END_CENTER_FRACTION * height;
		*startWidth = START_CENTER_FRACTION * width;
		int endWidth = END_CENTER_FRACTION * width;
		*centerHeight = endHeight - *startHeight;
		*centerWidth = endWidth - *startWidth;

		// print
		cout << "Got startHeight=" << *startHeight << ", endHeight=" << endHeight
				<< ", startWidth=" << *startWidth << ", endWidth=" << endWidth << endl;

		// extract center ROI
		//Mat hue_hsv_center(cv::Size(centerWidth, centerHeight), hue.type());
		cv::Mat center(input, cv::Rect(*startWidth, *startHeight, *centerWidth, *centerHeight));

		// return
		return center;

	}

	// given input image, detect edges, and do floodFill and return mask based on floodfill
	cv::Mat HoloPanelApp::getFloodFillMask(const cv::Mat& input, const string windowName, const bool convertFlag, const int convertCode) {
		// copy input to output
		cv::Mat output;
		input.copyTo(output);
		// if convertFlag on, then convert using conversion
		if (convertFlag) {
			cv::cvtColor(output, output, convertCode);
		}
		// get edges of output; need +1 on each edge
		cv::Mat edges;
		edges.create(input.rows+2, input.cols+2, output.type());
		cv::Mat edgesRoi(edges, cv::Rect(1, 1, input.cols, input.rows));
		cv::Canny(output, edgesRoi, FLOODFILL_CANNY_THRESHOLD1, FLOODFILL_CANNY_THRESHOLD2, FLOODFILL_CANNY_APERTURE_SIZE);//100, 500, 5);
		// perform floodfill
		cv::Rect floodFillRect;
		int floodFillArea = cv::floodFill(output, edges, cv::Point(input.rows/2, input.cols/2), FLOODFILL_NEWVAL, &floodFillRect, FLOODFILL_LODIFF, FLOODFILL_UPDIFF);
		// threshold to 0-1 mask
		//cv::threshold(output, output, FLOODFILL_NEWVAL-1, 255, CV_THRESH_BINARY);
		// show window
		if (!IS_MOBILE) {
			cv::namedWindow(windowName);
			cv::imshow(windowName, output);
		}
		// return
		return output;
	}

	// compute normalized histogram on single-channel Mat
	// - if reference frame, then
	// TODO: maybe never call without mask anymore?
	void HoloPanelApp::computeHistogram(const cv::Mat& input, cv::Mat& output, cv::InputArray mask) {

		// compute
		if (mask.empty()) {
			cv::calcHist(&input, 1, 0, cv::noArray(), output, 1, &NUM_HISTOGRAM_BINS, HUE_MINMAX);
		} else {
			cv::calcHist(&input, 1, 0, mask.getMat(), output, 1, &NUM_HISTOGRAM_BINS, HUE_MINMAX);
		}
		//int channels[2] = {0, 1};
		//cv::calcHist(&input, 1, channels, cv::Mat(), output, 2, &NUM_HISTOGRAM_BINS, HUE_MINMAX);

		//normalize
		cv::normalize(output, output, 0, 255, cv::NORM_MINMAX);

	}

	// display histogram of single-channel Mat; display in windowName window
	void HoloPanelApp::displayHistogram(const cv::Mat& histogram, const string windowName) {
		// reset histImage for this round
		histImage.setTo(HIST_IMAGE_255_SCALAR);
		// set each bin
		for (int i=0; i<NUM_HISTOGRAM_BINS; i++) {
			cv::rectangle(histImage, cv::Point(i*HIST_IMAGE_BIN_WIDTH, histImage.rows),
					cv::Point((i+1)*HIST_IMAGE_BIN_WIDTH, histImage.rows - cvRound(histogram.at<float>(i))),
					HIST_IMAGE_ZEROS_SCALAR, -1, 8, 0);
		}
		// show
		cv::imshow(windowName, histImage);
	}

	// TODO: JUMP: Draw black edges (value 0) on backProjectionProbabilities where we have strong edges!!!
	// compute back-projection of hueCenter onto referenceHist histogram, store in backProjectionProbabilities
	void HoloPanelApp::computeBackProjectionProbabilities(const cv::Mat& input) {
		//__android_log_print(ANDROID_LOG_INFO, "HoloPanelApp.cpp", "computeBackProjectionProbabilities: dims:%d", referenceHist.dims);
		//cv::calcBackProject(&input, 1, &ZERO, referenceHist, backProjectionProbabilities, HUE_MINMAX);
		cv::calcBackProject(&input, 1, 0, referenceHist, backProjectionProbabilities, HUE_MINMAX);
		//// print min and max of backProjectionProbabilities
		//double minVal, maxVal;
		//cv::minMaxLoc(backProjectionProbabilities, &minVal, &maxVal);
		//cout << "computeBackProjectionProbabilities: min=" << minVal << ", max=" << maxVal << endl;

		// mask backProjectionProbabilities with mask computed from hue values
		backProjectionProbabilities &= hueMask;

		// threshold backProjectionProbabilities
		// TODO: Experiment more with THRESH_OTSU - did poor job of separating out from background in initial attempt, but maybe could be better with tweaking input?
		cv::threshold(backProjectionProbabilities, backProjectionProbabilities, BACK_PROJECTION_THRESHOLD, MAX_HUE_VALUE, CV_THRESH_BINARY);

		// smooth the backProjectionProbabilities
		cv::GaussianBlur(backProjectionProbabilities, backProjectionProbabilities, BACK_PROJECTION_BLUR_KERNEL_SIZE, 0);

		// erode
		cv::erode(backProjectionProbabilities, backProjectionProbabilities, cv::Mat());
		//cv::erode(backProjectionProbabilities, backProjectionProbabilities, cv::Mat());
		//cv::erode(backProjectionProbabilities, backProjectionProbabilities, cv::Mat());
	}

	void HoloPanelApp::displayBackProjectionsImage(const string windowName) {
		cv::imshow(windowName, backProjectionProbabilities);
	}

	// draw RGB edges on top of backProjectionProbabilities
	void HoloPanelApp::addRGBEdgesToBackProjectionProbabilities(const cv::Mat& rgbInput) {
		// detect edges
		cv::Mat edges;
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;
		cv::Canny(rgbInput, edges, 200, 0, 3);
		//cv::Mat rgbInput_dilated;
		//cv::erode(rgbInput, rgbInput_dilated, cv::Mat());
		//cv::dilate(rgbInput, rgbInput_dilated, cv::Mat());
		//cv::dilate(rgbInput, rgbInput_dilated, cv::Mat());
		//cv::dilate(rgbInput, rgbInput_dilated, cv::Mat());
		//cv::Canny(rgbInput_dilated, edges, 200, 0, 3);
		// find contours
		cv::findContours(edges, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		// draw contours on top of backProjectionProbabilities as 0's
		const cv::Scalar COLOR_BLACK = cv::Scalar(0, 0, 0);
		cv::Mat output = cv::Mat::zeros(edges.size(), CV_8UC3);
		for (int i=0; i<contours.size(); i++) {
			//cv::Scalar color = cv::Scalar(rand()&255, rand()&255, rand()&255);//CONTOUR_COLOR_RANGE.uniform(0, 255), CONTOUR_COLOR_RANGE.uniform(0, 255), CONTOUR_COLOR_RANGE.uniform(0, 255));
			//cv::drawContours(output, contours, i, color, CV_FILLED, 8, hierarchy, 0, cv::Point());
			cv::drawContours(backProjectionProbabilities, contours, i, COLOR_BLACK, 3, 8, hierarchy, 0, cv::Point());
		}
		// dilate backProjectionProbabilities
		//cv::erode(backProjectionProbabilities, backProjectionProbabilities, cv::Mat());
		//cv::erode(backProjectionProbabilities, backProjectionProbabilities, cv::Mat());
		//cv::erode(backProjectionProbabilities, backProjectionProbabilities, cv::Mat());
	}

	void HoloPanelApp::displayContours(const cv::Mat& input, const string windowName) {
		// detect edges
		cv::Mat edges;
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;
		cv::Canny(input, edges, 200, 100, 3);
		// find contours
		cv::findContours(edges, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
		// draw contours
		cv::Mat output = cv::Mat::zeros(edges.size(), CV_8UC3);
		for (int i=0; i<contours.size(); i++) {
			cv::Scalar color = cv::Scalar(rand()&255, rand()&255, rand()&255);//CONTOUR_COLOR_RANGE.uniform(0, 255), CONTOUR_COLOR_RANGE.uniform(0, 255), CONTOUR_COLOR_RANGE.uniform(0, 255));
			//cv::drawContours(output, contours, i, color, CV_FILLED, 8, hierarchy, 0, cv::Point());
		}
		// draw approximate contours using approxPolyDP
		vector<vector<cv::Point> > contoursPoly(contours.size());
		for (int i=0; i<contours.size(); i++) {
			cv::approxPolyDP(cv::Mat(contours[i]), contoursPoly[i], 5, true);
			cv::polylines(output, contoursPoly[i], false, cv::Scalar(rand()&255, rand()&255, rand()&255), 1, 8, 0);
		}
		// show
		cv::imshow(windowName, output);
	}

	// find convex hulls using contours on image-like backProjectionProbabilities
	// TODO: pick largest hull that contains center of camshift search window
	void HoloPanelApp::findConvexHull() {

		// init
		hullFound = false;
		largestHullSize = MIN_HULL_SIZE;//-1;
		largestHullIndex = -1;

		// first find contours on image-like backProjectionProbabilities - use copy of backProjectionProbabilities since findContours modifies source
		backProjectionProbabilities.copyTo(backProjectionProbabilities_copy);
		//cv::GaussianBlur(backProjectionProbabilities_copy, backProjectionProbabilities_copy, cv::Size(31, 31), 0);
		cv::findContours(backProjectionProbabilities_copy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		//cv::findContours(edges, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

		// find convex hull for each contour, track largest contour
		hulls_points.clear();
		hulls_indices.clear();
		hulls_points.resize(contours.size());
		hulls_indices.resize(contours.size());
		for (int i=0; i<contours.size(); i++) {
			cv::convexHull(cv::Mat(contours[i]), hulls_points[i], false, true);
			cv::convexHull(cv::Mat(contours[i]), hulls_indices[i], false, false);
			hullSize = (int) cv::contourArea(hulls_points[i]);
			if (hullSize >= largestHullSize && hullSize <= MAX_HULL_SIZE) {
				largestHullSize = hullSize;
				largestHullIndex = i;
				hullFound = true;
			}
		}
	}

	// reset largestHullImage
	void HoloPanelApp::resetLargestHullImage() {
		if (!IS_MOBILE) {
			convexHullImage = cv::Mat::zeros(backProjectionProbabilities.size(), CV_8UC3); // TODO: hpp
		}
	}

	// draw largest hull
	void HoloPanelApp::displayLargestHull(cv::Mat& frame) {
		if (!IS_MOBILE) {
			cv::Scalar largestColor = cv::Scalar(0, 0, 255);//rand()&255, rand()&255, rand()&255); // TODO: hpp
			cv::drawContours(convexHullImage, contours, largestHullIndex, largestColor, 1, 8, vector<cv::Vec4i>(), 0, cv::Point());
			cv::drawContours(convexHullImage, hulls_points, largestHullIndex, largestColor, 1, 8, vector<cv::Vec4i>(), 0, cv::Point());
			cv::drawContours(frame, contours, largestHullIndex, largestColor, 1, 8, vector<cv::Vec4i>(), 0, cv::Point());
			cv::drawContours(frame, hulls_points, largestHullIndex, largestColor, 1, 8, vector<cv::Vec4i>(), 0, cv::Point());
		}
	}

	// get centroid of largest hull using moments
	void HoloPanelApp::getHullCentroid(cv::Mat& frame) {
		// get centroid of convex hull using moments
		convexHullMoments = cv::moments(hulls_points[largestHullIndex]);
		convexHullCentroid = cv::Point2i(convexHullMoments.m10 / convexHullMoments.m00, convexHullMoments.m01/convexHullMoments.m00);

		// draw centroid
		if (!IS_MOBILE) {
			cv::circle(convexHullImage, convexHullCentroid, 5, cv::Scalar(255, 0, 0), 2, 8, 0);
			cv::circle(frame, convexHullCentroid, 5, cv::Scalar(255, 0, 0), 2, 8, 0);
		} else {
			cv::circle(frame, convexHullCentroid, 5, cv::Scalar(255, 0, 0), 2, 8, 0);
		}
	}

	// isolate farthest convex hull corner/peak from convexHullCentroid as finger to track - must not be along edge of image
	void HoloPanelApp::findFarthestHullPeak(cv::Mat& frame) {
		// TODO: If maxDistance for farthest point is below some threshold, then no finger to track (closed fist, for example)
		double maxDistance = -1.0;
		int maxIndex = -1;
		for (int i=0; i<hulls_points[largestHullIndex].size(); i++) {
			double distanceToCentroid = cv::norm(hulls_points[largestHullIndex][i] - convexHullCentroid);
			if ( (distanceToCentroid >= maxDistance)
					&& (FARTHEST_CORNER_IMAGE_EDGE_THRESHOLD <= hulls_points[largestHullIndex][i].x)
					&& (hulls_points[largestHullIndex][i].x <= IMAGE_WIDTH - FARTHEST_CORNER_IMAGE_EDGE_THRESHOLD)
					&& (FARTHEST_CORNER_IMAGE_EDGE_THRESHOLD <= hulls_points[largestHullIndex][i].y)
					&& (hulls_points[largestHullIndex][i].y <= IMAGE_HEIGHT - FARTHEST_CORNER_IMAGE_EDGE_THRESHOLD) ) {
				maxDistance = distanceToCentroid;
				maxIndex = i;
			}
		}

		// save location of farthest hull peak
		mainFingerLocation = hulls_points[largestHullIndex][maxIndex];

		// draw circle around this point on image
		if (!IS_MOBILE) {
			cv::circle(convexHullImage, mainFingerLocation, 5, cv::Scalar(0, 255, 0), 2, 8, 0);
			cv::circle(frame, mainFingerLocation, 5, cv::Scalar(0, 255, 0), 2, 8, 0);
		} else {
			cv::circle(frame, mainFingerLocation, 5, cv::Scalar(0, 255, 0), 2, 8, 0);
		}
	}

	// get and display convexity defects
	void HoloPanelApp::getConvexityDefects(cv::Mat& frame) {

		// get defects
		cv::convexityDefects(contours[largestHullIndex], hulls_indices[largestHullIndex], convexityDefects);

		// display defects
		// TODO: only select defects where start/end point not on edge of image, and when convexity depth greater than threshold
		numFingerDefects = 0;
		for (int i=0; i<convexityDefects.size(); i++) {
			// parse defect
			cv::Point contour_defectStart = contours[largestHullIndex][convexityDefects[i][0]];
			cv::Point contour_defectEnd = contours[largestHullIndex][convexityDefects[i][1]];
			cv::Point defectPoint = contours[largestHullIndex][convexityDefects[i][2]];
			float convexityDepth = convexityDefects[i][3] / 256.0; // compute convexity depth with conversion to float
			// evaluate against image-edges and defect-threshold
			if ( convexityDepth >= CONVEXITY_DEFECT_DEPTH_THRESHOLD
					&& ( CONVEXITY_IMAGE_EDGE_THRESHOLD <= contour_defectStart.x )
					&& ( contour_defectStart.x <= (IMAGE_WIDTH - CONVEXITY_IMAGE_EDGE_THRESHOLD) )
					&& ( CONVEXITY_IMAGE_EDGE_THRESHOLD <= contour_defectStart.y )
					&& ( contour_defectStart.y <= (IMAGE_HEIGHT - CONVEXITY_IMAGE_EDGE_THRESHOLD) )
					&& ( CONVEXITY_IMAGE_EDGE_THRESHOLD <= contour_defectEnd.x )
					&& ( contour_defectEnd.x <= (IMAGE_WIDTH - CONVEXITY_IMAGE_EDGE_THRESHOLD) )
					&& ( CONVEXITY_IMAGE_EDGE_THRESHOLD <= contour_defectEnd.y )
					&& ( contour_defectEnd.y <= (IMAGE_HEIGHT - CONVEXITY_IMAGE_EDGE_THRESHOLD) ) ) {
				// plot from start -> defectPoint -> end
				if (!IS_MOBILE) {
					cv::line(convexHullImage, contour_defectStart, defectPoint, cv::Scalar(0, 255, 0), 1, 8, 0);
					cv::line(convexHullImage, defectPoint, contour_defectEnd, cv::Scalar(0, 255, 0), 1, 8, 0);
					cv::line(frame, contour_defectStart, defectPoint, cv::Scalar(0, 255, 0), 1, 8, 0);
					cv::line(frame, defectPoint, contour_defectEnd, cv::Scalar(0, 255, 0), 1, 8, 0);
				}
				numFingerDefects++;
			}
		}
	}

	// if hullFound, then write number of fingers (numFingerDefects + 1) - truncated down to 5 if greater
	// if no hull found, then write 0 fingers
	void HoloPanelApp::displayNumFingers(cv::Mat& frame) {
		if (hullFound) {
			if (!IS_MOBILE) {
				cv::putText(convexHullImage, _hp::convertIntToString(min(5, numFingerDefects + 1)), cv::Point(0, IMAGE_HEIGHT), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255));
				cv::putText(frame, _hp::convertIntToString(min(5, numFingerDefects + 1)), cv::Point(0, IMAGE_HEIGHT), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255));
			} else {
				cv::putText(frame, _hp::convertIntToString(min(5, numFingerDefects + 1)), cv::Point(0, IMAGE_HEIGHT), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255));
			}
		} else {
			if (!IS_MOBILE) {
				cv::putText(convexHullImage, _hp::convertIntToString(0), cv::Point(0, IMAGE_HEIGHT), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255));
			} else {
				cv::putText(frame, _hp::convertIntToString(0), cv::Point(0, IMAGE_HEIGHT), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255));
			}
		}
	}

	// run camshift update - first move foundWindow to searchWindow, then perform camshift
	void HoloPanelApp::updateCamshift() {
		camshiftSearchWindow = camshiftFoundWindow.boundingRect();
		camshiftFoundWindow = cv::CamShift(backProjectionProbabilities, camshiftSearchWindow, CAMSHIFT_TERM_CRITERIA);
		// if lost tracking, then reset to full image
		if (camshiftFoundWindow.boundingRect().area() <= 1) {
			camshiftFoundWindow = cv::RotatedRect(cv::Point2f( (float) IMAGE_WIDTH / 2, (float) IMAGE_HEIGHT / 2 ), cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT), 0);
		}
		if (!IS_MOBILE) {
			displayCamshiftWindow(WINDOW_CAMSHIFT);
		}
	}

	void HoloPanelApp::displayCamshiftWindow(const string windowName) {
		// copy backProjectionProbabilities mat to new image
		cv::Mat image;
		backProjectionProbabilities.copyTo(image);
		// draw camshiftWindow
		cv::rectangle(image, camshiftSearchWindow, cv::Scalar(255, 0, 0), 2, 8, 0);
		cv::rectangle(image, camshiftFoundWindow.boundingRect(), cv::Scalar(180, 0, 0), 2, 8, 0);
		// display image
		cv::imshow(windowName, image);
	}

	// display panel skeleton
	void HoloPanelApp::displayPanelSkeleton(cv::Mat& frame) {

		// draw top-left circle
		cv::circle(frame, TOP_LEFT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);

		// draw top-center circle
		cv::circle(frame, TOP_CENTER_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);

		// draw top-right circle
		cv::circle(frame, TOP_RIGHT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);

		// draw bottom-left circle
		cv::circle(frame, BOTTOM_LEFT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);

		// draw bottom-right circle
		cv::circle(frame, BOTTOM_RIGHT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);

	}

	// compute panel skeleton location of finger
	// TODO: Ignore if finger at edge (such at (0,0) uninitialized)
	void HoloPanelApp::computePanelSelection(cv::Mat& frame) {

		// if no hull found, then return none
		if (!hullFound) {
			panelSelection = PANEL_LABEL_NONE;
			return;
		}

		// if in top-left circle
		if (cv::norm(mainFingerLocation - TOP_LEFT_CIRCLE_CENTER) <= PANEL_CIRCLE_RADIUS) {
			cv::circle(frame, TOP_LEFT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_SELECTED_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);
			panelSelection = PANEL_LABEL_TOP_LEFT_CIRCLE;
		}
		// if in top-center circle
		else if (cv::norm(mainFingerLocation - TOP_CENTER_CIRCLE_CENTER) <= PANEL_CIRCLE_RADIUS) {
			cv::circle(frame, TOP_CENTER_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_SELECTED_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);
			panelSelection = PANEL_LABEL_TOP_CENTER_CIRCLE;
		}
		// if in top-right circle
		else if (cv::norm(mainFingerLocation - TOP_RIGHT_CIRCLE_CENTER) <= PANEL_CIRCLE_RADIUS) {
			cv::circle(frame, TOP_RIGHT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_SELECTED_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);
			panelSelection = PANEL_LABEL_TOP_RIGHT_CIRCLE;
		}
		// if in bottom-left circle
		else if (cv::norm(mainFingerLocation - BOTTOM_LEFT_CIRCLE_CENTER) <= PANEL_CIRCLE_RADIUS) {
			cv::circle(frame, BOTTOM_LEFT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_SELECTED_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);
			panelSelection = PANEL_LABEL_BOTTOM_LEFT_CIRCLE;
		}
		// if in bottom-right circle
		else if (cv::norm(mainFingerLocation - BOTTOM_RIGHT_CIRCLE_CENTER) <= PANEL_CIRCLE_RADIUS) {
			cv::circle(frame, BOTTOM_RIGHT_CIRCLE_CENTER, PANEL_CIRCLE_RADIUS, PANEL_BORDER_SELECTED_COLOR, PANEL_OBJECT_THICKNESS, PANEL_OBJECT_LINE_TYPE, PANEL_OBJECT_SHIFT);
			panelSelection = PANEL_LABEL_BOTTOM_RIGHT_CIRCLE;
		}
		// otherwise, return none
		else {
			panelSelection = PANEL_LABEL_NONE;
		}
	}

	// draw panel selection number on bottom-right of screen
	void HoloPanelApp::displayPanelSelection(cv::Mat& frame) {
		cv::putText(frame, _hp::convertIntToString(panelSelection), cv::Point(IMAGE_WIDTH - 20, IMAGE_HEIGHT - 5), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255));
	}

	// process panel selection vs previousPanelSelection and consecutivePanelSelections
	void HoloPanelApp::processPanelSelection() {

		// reset triggerPanelAction first
		triggerPanelAction = false;

		// if consecutive non-none selection
		if (panelSelection == previousPanelSelection && panelSelection != PANEL_LABEL_NONE) {

			// increment consecutive count
			consecutivePanelSelections++;

			// if greater than numConsecutive threshold
			if (consecutivePanelSelections >= MIN_CONSECUTIVE_PANEL_SELECTIONS) {

				// set action trigger
				triggerPanelAction = true;

			}

		// otherwise, reset consecutivePanelSelections
		} else {
			consecutivePanelSelections = 0;
		}
	}
}
