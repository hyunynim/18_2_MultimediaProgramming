
#include<core.hpp>
#include<imgproc.hpp>
#include<highgui.hpp>
#include<iostream>
#include<cmath>
#include<windows.h>
#include<mmsystem.h>
#pragma comment(lib, "winmm.lib")


#include <videoio.hpp>
#include <video/background_segm.hpp>
using namespace cv;
using namespace std;

static void refineSegments(const Mat& img, Mat& mask, Mat& dst)
{
	int niters = 3;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat temp;
	dilate(mask, temp, Mat(), Point(-1, -1), niters);
	erode(temp, temp, Mat(), Point(-1, -1), niters * 2);
	dilate(temp, temp, Mat(), Point(-1, -1), niters);
	findContours(temp, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
	dst = Mat::zeros(img.size(), CV_8UC3);
	if (contours.size() == 0)
		return;

	int idx = 0, largestComp = 0;
	double maxArea = 0;
	for (; idx >= 0; idx = hierarchy[idx][0])
	{
		const vector<Point>& c = contours[idx];
		double area = fabs(contourArea(Mat(c)));
		if (area > maxArea)
		{
			maxArea = area;
			largestComp = idx;
		}
	}
	Scalar color(0, 0, 255);
	drawContours(dst, contours, largestComp, color, FILLED, LINE_8, hierarchy);
}
void bark() {
	printf("침입자 감지\n");
	PlaySound(TEXT("18_2_Project\\bark.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}
void shutup() {
	PlaySound(NULL, 0, 0);
	system("cls");
}
int main(int argc, char** argv)
{
	VideoCapture cap;
	bool update_bg_model = true;
	CommandLineParser parser(argc, argv, "{help h||}{@input||}");
	if (parser.has("help"))
	{
		return 0;
	}
	string input = parser.get<std::string>("@input");
	if (input.empty())
		cap.open(0);
	else
		cap.open(samples::findFileOrKeep(input));
	if (!cap.isOpened())
	{
		printf("\nCan not open camera or video file\n");
		return -1;
	}
	Mat tmp_frame, bgmask, out_frame;
	Mat backGround, cmp;
	cap >> tmp_frame;
	if (tmp_frame.empty())
	{
		printf("can not read data from the video source\n");
		return -1;
	}
	namedWindow("video", 1);
	namedWindow("segmented", 1);
	Ptr<BackgroundSubtractorMOG2> bgsubtractor = createBackgroundSubtractorMOG2();
	bgsubtractor->setVarThreshold(10);
	int cnt = 0;
	int barkCnt;
	for (;;)
	{
		cap.read(tmp_frame);
		if (tmp_frame.empty())
			break;
		bgsubtractor->apply(tmp_frame, bgmask, update_bg_model ? -1 : 0);
		refineSegments(tmp_frame, bgmask, out_frame);
		if (cnt == 0) {
			int y = out_frame.rows, x = out_frame.cols;
			printf("이미지 크기 %dx%d\n", x, y);
		}
		if (cnt > 20) {
			bool inChk = 0;
			int y = out_frame.rows, x = out_frame.cols;
			for (int i = 0; i < y; ++i) {
				for (int j = 0; j < x; ++j) {
					int k = out_frame.at<uchar>(i, j);
					if (k) {
						bark();
						barkCnt = 30;
						inChk = 1;
						break;
					}
				}
				if (inChk) break;
			}
			if (!inChk) {
				--barkCnt;
				if (barkCnt == 0) {
					shutup();
				}
			}
		}
		imshow("video", tmp_frame);
		imshow("segmented", out_frame);
		char keycode = (char)waitKey(30);
		if (keycode == 27)
			break;
		if (keycode == ' ')
		{
			update_bg_model = !update_bg_model;
			printf("Learn background is in state = %d\n", update_bg_model);
		}
		++cnt;
	}
	return 0;
}