#include<opencv2/core.hpp>
#include<opencv2/videoio.hpp>
#include<opencv2/highgui.hpp>
#include<opencv.hpp>
#include<cstdio>
using namespace cv;
using namespace std;
int main(int argc, char ** argv) {

	Mat frame;
	VideoCapture cap;
	cap.open("ball.avi");
//	cap.open("car.wmv");
//	cap.open("tunnel.mpeg");
//	cap.open("hand.avi");
	if (!cap.isOpened()) {
		printf("file 찾을 수 없음");
		return -1;
	}
	int width = cap.get(CAP_PROP_FRAME_WIDTH);
	int height = cap.get(CAP_PROP_FRAME_HEIGHT);
	Size vSize(width, height);
	Mat grayImg(vSize, CV_8U);
	Mat sumImg(vSize, CV_32F);


	int nFrameCnt = 0;
	for (;;)
	{
		cap.read(frame); 
		if (grayImg.empty() || frame.empty() ) break;
		cvtColor(frame, grayImg, COLOR_BGR2GRAY);
		accumulate(grayImg, sumImg);
		if (frame.empty()) {
			cerr << "ERROR! blank frame grabbed\n";
			break;
		}
		imshow("Live", frame);
		char k = waitKey(5);
		if (k >= 0) break;
		++nFrameCnt;
	}
	convertScaleAbs(sumImg, sumImg, 1.0 / nFrameCnt);
	imwrite("ballBkg.jpg", sumImg);
//	imwrite("carBkg.jpg", sumImg);
//	imwrite("tunnelBkg.jpg", sumImg);
//	imwrite("handBkg.jpg", sumImg);
	return 0;
}