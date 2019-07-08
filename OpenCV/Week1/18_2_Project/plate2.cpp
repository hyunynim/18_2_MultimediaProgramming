#include "opencv2/core.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
using namespace cv;
using namespace std;
int thresh = 50, N = 11;
const char* wndname = "Plate Detection";

static double angle(Point pt1, Point pt2, Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1 * dy2) / sqrt((dx1*dx1 + dy1 * dy1)*(dx2*dx2 + dy2 * dy2) + 1e-10);
}

void findSquares(const UMat& image, vector<vector<Point> >& squares, double scal)
{
	squares.clear();
	UMat pyr, timg, gray0(image.size(), CV_8U), gray;

	pyrDown(image, pyr, Size(image.cols / 2, image.rows / 2));
	pyrUp(pyr, timg, image.size());
	vector<vector<Point> > contours;

	for (int c = 0; c < 3; c++)
	{
		int ch[] = { c, 0 };
		mixChannels(timg, gray0, ch, 1);

		for (int l = 0; l < N; l++)
		{
			if (l == 0)
			{
				Canny(gray0, gray, 0, thresh, 5);
				dilate(gray, gray, UMat(), Point(-1, -1));
			}
			else
			{
				threshold(gray0, gray, (l + 1) * 255 / N, 255, THRESH_BINARY);
			}
			findContours(gray, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
			vector<Point> approx;

			for (size_t i = 0; i < contours.size(); i++)
			{

				approxPolyDP(contours[i], approx, arcLength(contours[i], true)*0.02, true);

				if (approx.size() == 4 &&
					fabs(contourArea(approx)) > 1000 &&
					isContourConvex(approx))
				{
					double maxCosine = 0;
					for (int j = 2; j < 5; j++)
					{

						double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
						maxCosine = MAX(maxCosine, cosine);
					}

					if (maxCosine <= scal)
						squares.push_back(approx);
				}
			}
		}
	}
}

void drawSquares(UMat& _image, const vector<vector<Point> >& squares, int k)
{
	Mat image = _image.getMat(ACCESS_WRITE);
	if (squares.empty()) return;
	int minx = 10000, miny = 10000;
	int maxx = 0, maxy = 0;
	for (size_t i = 0; i < squares.size(); i++)
	{
		const Point* p = &squares[i][0];
		for (int j = 0; j < squares[i].size(); ++j) {
			minx = min(minx, squares[i][j].x);
			miny = min(miny, squares[i][j].y);
			maxx = max(maxx, squares[i][j].x);
			maxy = max(maxy, squares[i][j].y);
		}
		int n = (int)squares[i].size();
		polylines(image, &p, &n, 1, true, Scalar(0, 255, 0), 3, LINE_AA);
	}
	printf("번호판 감지[(%d, %d) ~ (%d, %d)]\n", minx, miny, maxx, maxy);
	Mat tmp;
	tmp = image(Rect(minx, miny, maxx - minx, maxy - miny));
	char msg[123];
	sprintf(msg, "%d detection", k);
	if (tmp.rows < 2 || tmp.cols < 2) return;
	imshow(msg, tmp);
}

UMat drawSquaresBoth(const UMat& image,
	const vector<vector<Point> >& sqs, int k)
{
	UMat imgToShow(Size(image.cols, image.rows), image.type());
	image.copyTo(imgToShow);
	drawSquares(imgToShow, sqs, k);
	return imgToShow;
}
int main(int argc, char** argv)
{
	string inputName = "18_2_project/1.jpg";
	string outfile = "tmp.jpg";
	int iterations = 10;
	namedWindow(wndname, WINDOW_AUTOSIZE);
	vector<vector<Point> > squares;
	UMat image;
	imread(inputName, IMREAD_COLOR).copyTo(image);
	imshow("origin", image);
	if (image.empty())
	{
		cout << "Couldn't load " << inputName << endl;
		return EXIT_FAILURE;
	}
	int j = iterations;
	int64 t_cpp = 0;

	UMat result;
	do
	{
		char msg[123];
		int64 t_start = getTickCount();
		findSquares(image, squares, 0.51 - 0.05 * (j + 1));
		if (squares.empty())continue;
		t_cpp += cv::getTickCount() - t_start;
		t_start = getTickCount();
		cout << "run loop: " << j << endl;
		if(result.cols == 0 || result.rows == 0)
			result = drawSquaresBoth(image, squares, j + 1);
		drawSquaresBoth(image, squares, j + 1);
		waitKey(1);
		sprintf(msg, "%d window", j + 1);
		squares.clear();
	} while (--j);
	cout << "average time: " << 1000.0f * (double)t_cpp / getTickFrequency() / iterations << "ms" << endl;
	if (result.cols == 0 || result.rows == 0) {
		cout << "non dection";
		return 0;
	}
	imshow(wndname, result);
	imwrite(outfile, result);
	waitKey(0);
	return EXIT_SUCCESS;
}