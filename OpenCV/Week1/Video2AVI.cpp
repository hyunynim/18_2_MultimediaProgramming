#include<opencv2/core.hpp>
#include<opencv2/videoio.hpp>
#include<opencv2/highgui.hpp>
#include<opencv.hpp>
#include<cstdio>
using namespace cv;
using namespace std;
int main(int argc, char ** argv){
	Mat frame;
	VideoCapture cap;
	int dID = 0;
	int apiID = cv::CAP_ANY;
	cap.open(dID + apiID);
	if (!cap.isOpened()) {
		cerr << "ERROR! Unable to open camera\n";
		return -1;
	}

	//-1인 경우, 코덱 목록을 확인할 수 있음
	//특정 코덱으로 설정하고 싶은 경우 VideoWriter::fourcc를 이용해야함
	//http://www.fourcc.org/codecs.php 참조
	int fourcc = -1;
	//fourcc = VideoWriter::fourcc('M', 'P', '4', '2');	//MPEG-4
	fourcc = 0;
	double fps = 30.0;
	cap.set(CAP_PROP_FRAME_WIDTH, 640.0);
	cap.set(CAP_PROP_FRAME_HEIGHT, 480.0);
	
	printf("fps = %lf, frame_size = (%d, %d)\n", fps, 640, 480);

	Size vSize;
	vSize.width = 640;
	vSize.height = 480;
	VideoWriter * vWriter = new VideoWriter("test.avi", fourcc, fps, vSize);
	if (!vWriter->isOpened())
		return -1;

	for (;;)
	{
		cap.read(frame);
		if (frame.empty()) {
			cerr << "ERROR! blank frame grabbed\n";
			break;
		}
		imshow("Live", frame);
		vWriter->write(frame);	//write를 이용해 frame 녹화
		if (waitKey(5) >= 0)
			break;
	}

	return 0;
}