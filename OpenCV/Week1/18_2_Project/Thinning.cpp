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




// ����ȭ �ϴ� �Լ�!!!!! 
// cv�̹����� ��������Ƿ� cv�� ������� �������� ������ �ʿ� 

IplImage* Thinning::Thin(IplImage* InImage)
{
	// Output�̹��� �޸� ���� 
	IplImage* OutImage = 0;
	OutImage = (IplImage*)malloc(InImage->nSize);
	*OutImage = *InImage;

	OutImage->imageData = (char *)malloc(sizeof(InImage->imageData));
	OutImage->imageData = InImage->imageData;

	int i, j;
	// ����ϱ� ���ϰ� ���� �����͸� �����ϱ� ���� ���� 
	//      int w = InImage->width + 2; 
	//      int h = InImage->height + 2; 

	char* img = (char *)malloc((InImage->width + 2)*(InImage->height + 2));//(sizeof(InImage->imageData) + InImage->width*2 + InImage->height*2 - 4); 
	char* timg = (char *)malloc((InImage->width + 2)*(InImage->height + 2));//(sizeof(InImage->imageData) + InImage->width*2 + InImage->height*2 - 4); 

	// cv�������͸� �̾Ƴ��� ����ϱ� ���ϰ� �ٲ� //img[i+j*InImage->widthStep] 
	for (i = 0; i < InImage->width; i++)
	{
		for (j = 0; j < InImage->height; j++)
		{
			img[(i + 1) + (j + 1)*InImage->widthStep] = InImage->imageData[i + j * InImage->widthStep];
		}
	}

	// ������ �����ڸ��� 0���� �ڸ��� ä����. 
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

	//������ ��ü�� 1, ����� 0�� �ٲ�. 
	for (i = 1; i < InImage->width + 1; i++)
	{
		for (j = 1; j < InImage->height + 1; j++)
		{
			if (img[i + j * InImage->widthStep] != 0)
				img[i + j * InImage->widthStep] = 1;      // black 
			else
				img[i + j * InImage->widthStep] = 0;      // white 

			timg[i + j * InImage->widthStep] = 0;             // timg �ʱ�ȭ 
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

				//������ �߾� �κ��� �߽����� 1�� ������ ���� ���. 
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

	// 1�� 0�� 0�� 255�� �ٽ� ȯ�� 
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

	// �ٲپ� ����� �����͸� cv�������Ϳ� ���߾� ���� 
	for (i = 0; i < InImage->width; i++)
	{
		for (j = 0; j < InImage->height; j++)
		{
			OutImage->imageData[i + j * InImage->widthStep] = img[(i + 1) + (j + 1)*InImage->widthStep];
		}
	}
	return OutImage;
}