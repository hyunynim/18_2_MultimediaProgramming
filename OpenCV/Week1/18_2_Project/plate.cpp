#include<opencv2/core.hpp>
#include<opencv2/videoio.hpp>
#include<opencv2/highgui.hpp>
#include<opencv.hpp>
#include<iostream>
#include<cstdio>
#include<algorithm>
using namespace cv;
using namespace std;
struct pos {
	int x, y;
};
double len(int x, int y) {
	return sqrt(x * x + y * y);
}
bool recChk(vector<Point> pnt) {
	vector<pos> p;
	for (int i = 0; i < pnt.size(); ++i) 
		p.push_back({ pnt[i].x, pnt[i].y });
	auto cmp = [&](pos p1, pos p2) {
		return atan((double)p1.y / p1.x) < atan((double)p2.y / p2.x);
	};
	sort(p.begin(), p.end(), cmp);
	int cnt = 0;
	for (int i = 0; i < p.size() - 1; ++i) {
		int x1 = p[i].x;
		int y1 = p[i].y;
		int x2 = p[i + 1].x;
		int y2 = p[i + 1].y;
		int len1 = len(x1, y1);
		int len2 = len(x2, y2);
		if (i == 0) {
			int x = p.back().x;
			int y = p.back().y;
			int len3 = len(x, y);
			if ((double)(x1 * x + y1 * y) / (len1 * len3) <= 0.5)
				++cnt;
		}
		if ((double)(x1 * x2 + y1 * y2) / (len1 * len2) <= 0.5)
			++cnt;
	}
	return cnt == 4;
}
static void help(const char* programName)
{
	cout <<
		"\nA program using pyramid scaling, Canny, contours and contour simplification\n"
		"to find squares in a list of images (pic1-6.png)\n"
		"Returns sequence of squares detected on the image.\n"
		"Call:\n"
		"./" << programName << " [file_name (optional)]\n"
		"Using OpenCV version " << CV_VERSION << "\n" << endl;
}
int thresh = 50, N = 11;
const char* wndname = "Square Detection Demo";
// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
static double angle(Point pt1, Point pt2, Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1 * dy2) / sqrt((dx1*dx1 + dy1 * dy1)*(dx2*dx2 + dy2 * dy2) + 1e-10);
}
// returns sequence of squares detected on the image.
static void findSquares(const Mat& image, vector<vector<Point> >& squares)
{
	squares.clear();
	Mat pyr, timg, gray0(image.size(), CV_8U), gray;
	// down-scale and upscale the image to filter out the noise
	pyrDown(image, pyr, Size(image.cols / 2, image.rows / 2));
	pyrUp(pyr, timg, image.size());
	vector<vector<Point> > contours;
	// find squares in every color plane of the image
	for (int c = 0; c < 3; c++)
	{
		int ch[] = { c, 0 };
		mixChannels(&timg, 1, &gray0, 1, ch, 1);
		// try several threshold levels
		for (int l = 0; l < N; l++)
		{
			// hack: use Canny instead of zero threshold level.
			// Canny helps to catch squares with gradient shading
			if (l == 0)
			{
				// apply Canny. Take the upper threshold from slider
				// and set the lower to 0 (which forces edges merging)
				//Canny(gray0, gray, 0, thresh, 5);
				Canny(gray0, gray, 0, thresh, 7);
				// dilate canny output to remove potential
				// holes between edge segments
				dilate(gray, gray, Mat(), Point(-1, -1));
			}
			else
			{
				// apply threshold if l!=0:
				//     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
				gray = gray0 >= (l + 1) * 255 / N;
			}
			// find contours and store them all as a list
			findContours(gray, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
			vector<Point> approx;
			// test each contour
			for (size_t i = 0; i < contours.size(); i++)
			{
				// approximate contour with accuracy proportional
				// to the contour perimeter
				approxPolyDP(contours[i], approx, arcLength(contours[i], true)*0.02, true);
				// square contours should have 4 vertices after approximation
				// relatively large area (to filter out noisy contours)
				// and be convex.
				// Note: absolute value of an area is used because
				// area may be positive or negative - in accordance with the
				// contour orientation
				if (approx.size() == 4 &&
					fabs(contourArea(approx)) > 1000 &&
					isContourConvex(approx)
					)
				{
					double maxCosine = 0;
					for (int j = 2; j < 5; j++)
					{
						// find the maximum cosine of the angle between joint edges
						double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
						maxCosine = MAX(maxCosine, cosine);
					}
					// if cosines of all angles are small
					// (all angles are ~90 degree) then write quandrange
					// vertices to resultant sequence
					if (maxCosine < 0.3)
						squares.push_back(approx);
				}
			}
		}
	}
}
// the function draws all the squares in the image
static void drawSquares(Mat& image, const vector<vector<Point> >& squares)
{
	if (squares.empty()) return;
	int minx = 10000, miny = 10000;
	int maxx = 0, maxy = 0;
	for (size_t i = 0; i < squares.size(); i++)
	{
		const Point* p = &squares[i][0];
		minx = min(minx, p->x);
		miny = min(miny, p->y);
		maxx = max(maxx, squares[i].back().x);
		maxy = max(maxy, squares[i].back().y);
		int n = (int)squares[i].size();
		polylines(image, &p, &n, 1, true, Scalar(0, 255, 0), 3, LINE_AA);
	}
	printf("번호판 감지[(%d, %d) ~ (%d, %d)]", minx, miny, maxx, maxy);
	Mat tmp;
	tmp = image(Rect(minx, miny, maxx - minx, maxy - miny));
	imshow(wndname, image);
	imshow("test", tmp);
}
int main(int argc, char** argv)
{
	vector<vector<Point> > squares;
		string filename = "18_2_project\\3.jpg";
		Mat image = imread(filename, IMREAD_COLOR);
		if (image.empty())
		{
			cout << "Couldn't load " << filename << endl;
		}
		imshow("origin img", image);
		findSquares(image, squares);
		drawSquares(image, squares);
		int c = waitKey();
		if (c == 27)
			return 0;
}