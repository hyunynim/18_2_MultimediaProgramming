/**
 * @function findContours_Demo.cpp
 * @brief Demo code to find contours in an image
 * @author OpenCV team
 */

#include<imgcodecs.hpp>
#include<highgui.hpp>
#include<imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;

Mat src_gray;
int thresh = 100;
RNG rng(12345);


void thresh_callback(int, void*)
{
	/// Detect edges using Canny
	Mat canny_output;
	Canny(src_gray, canny_output, thresh, thresh * 2);

	/// Find contours
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

	/// Draw contours
	Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
	for (size_t i = 0; i < contours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));
		drawContours(drawing, contours, (int)i, color, 2, LINE_8, hierarchy, 0);
	}

	/// Show in a window
	imshow("Contours", drawing);
}

int main(int argc, char** argv)
{
	/// Load source image
	Mat src = imread("18_2_Project\\1.jpg");
	if (src.empty())
	{
		cout << "Could not open or find the image!\n" << endl;
		cout << "Usage: " << argv[0] << " <Input image>" << endl;
		return -1;
	}

	/// Convert image to gray and blur it
	cvtColor(src, src_gray, COLOR_BGR2GRAY);
	blur(src_gray, src_gray, Size(3, 3));

	/// Create Window
	const char* source_window = "Source";
	namedWindow(source_window);
	Mat tmp;
	cvtColor(src, tmp, COLOR_BGRA2GRAY);
	Canny(tmp, tmp, 100, 200);
	imshow(source_window, tmp);

	const int max_thresh = 255;
	createTrackbar("Canny thresh:", source_window, &thresh, max_thresh, thresh_callback);
	thresh_callback(0, 0);

	waitKey();
	return 0;
}
