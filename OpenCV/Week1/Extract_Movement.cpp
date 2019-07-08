#include<opencv2/core.hpp>
#include<opencv2/videoio.hpp>
#include<opencv2/highgui.hpp>
#include<opencv.hpp>
#include<cstdio>
using namespace cv;
using namespace std;
int main(int argc, char ** argv) {

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
	Mat diffImg(vSize, CV_8U);

	Mat bkImg(vSize, CV_8U);
	Mat frame(vSize, CV_8U);
	bkImg = imread("ballBkg.jpg", COLOR_BGR2GRAY);
//	bkImg = imread("carBkg.jpg", COLOR_BGR2GRAY);
//	bkImg = imread("tunnelBkg.jpg", COLOR_BGR2GRAY);
//	bkImg = imread("handBkg.jpg", COLOR_BGR2GRAY);

	if (bkImg.empty()) {
		printf("배경화면 읽기 실패");
		return -1;
	}
	int nFrameCnt = 0;
	int nThreshold = 50;
	for (;;)
	{
		cap.read(frame);
		if (grayImg.empty() || frame.empty()) break;
		
		cvtColor(frame, grayImg, COLOR_BGR2GRAY);
		absdiff(grayImg, bkImg, diffImg);
		threshold(diffImg, diffImg, nThreshold, 255, THRESH_BINARY);
		
		imshow("Live", grayImg);
		imshow("Live", diffImg);

		char k = waitKey(5);
		if (k >= 0) break;
		++nFrameCnt;
	}
	return 0;
}