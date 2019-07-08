#include<opencv2/core.hpp>
#include<opencv2/videoio.hpp>
#include<opencv2/highgui.hpp>
#include<opencv.hpp>
#include<iostream>
#include<cstdio>
using namespace cv;
using namespace std;
int main(int, char**)
{
	Mat frame, gray, edge;
	//--- INITIALIZE VIDEOCAPTURE
	VideoCapture cap;
	// open the default camera using default API
	// cap.open(0);
	// OR advance usage: select any API backend
	int deviceID = 0;             // 0 = open default camera
	int apiID = cv::CAP_ANY;      // 0 = autodetect default API
	// open selected camera using selected API
	cap.open(deviceID + apiID);
	// check if we succeeded
	if (!cap.isOpened()) {
		cerr << "ERROR! Unable to open camera\n";
		return -1;
	}
	//--- GRAB AND WRITE LOOP
	cout << "Start grabbing" << endl
		<< "Press any key to terminate" << endl;
	for (;;)
	{
		cap.read(frame);
		cvtColor(frame, gray, COLOR_BGR2GRAY);
		Canny(gray, edge, 50, 100, 3);
		// check if we succeeded
		if (frame.empty()) {
			cerr << "ERROR! blank frame grabbed\n";
			break;
		}
		// show live and wait for a key with timeout long enough to show images
		imshow("Live", edge);
		if (waitKey(5) >= 0)
			break;
	}
	
	// the camera will be deinitialized automatically in VideoCapture destructor
	return 0;
}