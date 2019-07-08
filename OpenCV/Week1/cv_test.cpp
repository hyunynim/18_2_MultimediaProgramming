#include <core.hpp>
#include <highgui.hpp>

#include <iostream>
#include <string>
using namespace cv;
using namespace std;

int main()
{
	Mat image = imread("desert.jpg", IMREAD_COLOR);
	if (image.empty()){
		cout << "이미지 없음\n";
		system("pause");
		return -1;
	}

	namedWindow("Display window", WINDOW_AUTOSIZE);

	imshow("Display window", image);
	waitKey(0);
	return 0;
}
