#include "Thinning.h"

Thinning::Thinning() {}
Thinning::~Thinning() {}

void Thinning::DeleteA(char* img, char* timg, int cx, int cy, int widthStep)
{
	int i, j;

	for (i = 1; i < cx + 1; i++)
		for (j = 1; j < cy + 1; j++)
			if (timg[i + j * widthStep])
			{
				img[i + j * widthStep] = 0;
				timg[i + j * widthStep] = 0;
			}
}

int Thinning::nays(char* img, int i, int j, int widthStep)
{
	int a, b, N = 0;

	for (a = i - 1; a <= i + 1; a++)
		for (b = j - 1; b <= j + 1; b++)
			if (a != i || b != j)
				if (img[a + b * widthStep] >= 1)
					N++;
	return N;
}


int Thinning::Connect(char* img, int i, int j, int widthStep)
{
	int n = 0;

	if (img[i + (j + 1)*widthStep] >= 1 && img[(i - 1) + (j + 1)*widthStep] == 0) n++;
	if (img[(i - 1) + (j + 1)*widthStep] >= 1 && img[(i - 1) + j * widthStep] == 0) n++;
	if (img[(i - 1) + j * widthStep] >= 1 && img[(i - 1) + (j - 1)*widthStep] == 0) n++;
	if (img[(i - 1) + (j - 1)*widthStep] >= 1 && img[i + (j - 1)*widthStep] == 0) n++;
	if (img[i + (j - 1)*widthStep] >= 1 && img[(i + 1) + (j - 1)*widthStep] == 0) n++;
	if (img[(i + 1) + (j - 1)*widthStep] >= 1 && img[(i + 1) + j * widthStep] == 0) n++;
	if (img[(i + 1) + j * widthStep] >= 1 && img[(i + 1) + (j + 1)*widthStep] == 0) n++;
	if (img[(i + 1) + (j + 1)*widthStep] >= 1 && img[i + (j + 1)*widthStep] == 0) n++;
	return n;
}




// 세선화 하는 함수!!!!! 
// cv이미지를 사용했으므로 cv를 사용하지 않으려면 수정이 필요 

IplImage* Thinning::Thin(IplImage* InImage)
{
	// Output이미지 메모리 생성 
	IplImage* OutImage = 0;
	OutImage = (IplImage*)malloc(InImage->nSize);
	*OutImage = *InImage;

	OutImage->imageData = (char *)malloc(sizeof(InImage->imageData));
	OutImage->imageData = InImage->imageData;

	int i, j;
	// 계산하기 편하게 만든 데이터를 저장하기 위한 변수 
	//      int w = InImage->width + 2; 
	//      int h = InImage->height + 2; 

	char* img = (char *)malloc((InImage->width + 2)*(InImage->height + 2));//(sizeof(InImage->imageData) + InImage->width*2 + InImage->height*2 - 4); 
	char* timg = (char *)malloc((InImage->width + 2)*(InImage->height + 2));//(sizeof(InImage->imageData) + InImage->width*2 + InImage->height*2 - 4); 

	// cv영상데이터를 뽑아내서 계산하기 편하게 바꿈 //img[i+j*InImage->widthStep] 
	for (i = 0; i < InImage->width; i++)
	{
		for (j = 0; j < InImage->height; j++)
		{
			img[(i + 1) + (j + 1)*InImage->widthStep] = InImage->imageData[i + j * InImage->widthStep];
		}
	}

	// 영상의 가장자리에 0으로 자리를 채워줌. 
	for (i = 0; i < InImage->width + 2; i++)
	{
		img[i + 0 * InImage->widthStep] = 0;
		img[i + (InImage->height + 1)*InImage->widthStep] = 0;
	}

	for (j = 0; j < InImage->height + 2; j++)
	{
		img[0 + j * InImage->widthStep] = 0;
		img[(InImage->width + 1) + j * InImage->widthStep] = 0;
	}

	//영상의 물체를 1, 배경을 0로 바꿈. 
	for (i = 1; i < InImage->width + 1; i++)
	{
		for (j = 1; j < InImage->height + 1; j++)
		{
			if (img[i + j * InImage->widthStep] != 0)
				img[i + j * InImage->widthStep] = 1;      // black 
			else
				img[i + j * InImage->widthStep] = 0;      // white 

			timg[i + j * InImage->widthStep] = 0;             // timg 초기화 
		}
	}

	bool again = 1;
	int N;

	while (again)
	{
		again = 0;

		// first sub-iteration 
		for (i = 1; i < InImage->width + 1; i++)
		{
			for (j = 1; j < InImage->height + 1; j++)
			{
				if (img[i + j * InImage->widthStep] != 1)
					continue;

				//영상의 중앙 부분을 중심으로 1인 영상의 개수 계산. 
				N = nays(img, i, j, InImage->widthStep);

				if ((N >= 2 && N <= 6) && Connect(img, i, j, InImage->widthStep) == 1)
				{
					if ((img[i + (j + 1)*InImage->widthStep] * img[(i - 1) + j * InImage->widthStep] * img[i + (j - 1)*InImage->widthStep]) == 0 &&
						(img[(i - 1) + j * InImage->widthStep] * img[(i + 1) + j * InImage->widthStep] * img[i + (j - 1)*InImage->widthStep]) == 0)
					{
						timg[i + j * InImage->widthStep] = 1;
						again = 1;
					}
				}
			}
		}
		DeleteA(img, timg, InImage->width, InImage->height, InImage->widthStep);
		if (again == 0)
			break;

		// Second sub-iteration 
		for (i = 1; i < InImage->width + 1; i++)
		{
			for (j = 1; j < InImage->height + 1; j++)
			{
				if (img[i + j * InImage->widthStep] != 1)
					continue;

				N = nays(img, i, j, InImage->widthStep);

				if ((N >= 2 && N <= 6) && Connect(img, i, j, InImage->widthStep) == 1)
				{
					if ((img[(i - 1) + j * InImage->widthStep] * img[i + (j + 1)*InImage->widthStep] * img[(i + 1) + j * InImage->widthStep]) == 0 &&
						(img[i + (j + 1)*InImage->widthStep] * img[(i + 1) + j * InImage->widthStep] * img[i + (j - 1)*InImage->widthStep]) == 0)
					{
						timg[i + j * InImage->widthStep] = 1;
						again = 1;
					}
				}
			}
		}
		DeleteA(img, timg, InImage->width, InImage->height, InImage->widthStep);
	}

	// 1을 0로 0를 255로 다시 환원 
	for (i = 0; i < InImage->width + 2; i++)
	{
		for (j = 0; j < InImage->height + 2; j++)
		{
			if (img[i + j * InImage->widthStep] != 0)
				img[i + j * InImage->widthStep] = 0;
			else
				img[i + j * InImage->widthStep] = 255;
		}
	}

	// 바꾸어 사용한 데이터를 cv영상데이터에 맞추어 저장 
	for (i = 0; i < InImage->width; i++)
	{
		for (j = 0; j < InImage->height; j++)
		{
			OutImage->imageData[i + j * InImage->widthStep] = img[(i + 1) + (j + 1)*InImage->widthStep];
		}
	}
	return OutImage;
}