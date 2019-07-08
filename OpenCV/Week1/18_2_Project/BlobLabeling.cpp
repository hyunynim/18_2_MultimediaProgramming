#include "BlobLabeling.h"

#define _DEF_MAX_BLOBS		10000
#define _DEF_MAX_LABEL		  100

CBlobLabeling::CBlobLabeling(void)
//������
//���̺� �������� �ʱⰪ ����
//�Ӱ谪 0 
//��� ���� 10000��
//�̹��� ����
//���̺� ���� ����
{
	m_nThreshold = 0;
	m_nBlobs = _DEF_MAX_BLOBS;
	m_Image = NULL;
	m_recBlobs = NULL;
	m_vPoint = NULL;
}
CBlobLabeling::~CBlobLabeling(void)
//�ı���
//�޸𸮸� ���� �������ش�.
{
	if (m_Image != NULL)	cvReleaseImage(&m_Image);

	if (m_recBlobs != NULL)
	{
		delete m_recBlobs;
		m_recBlobs = NULL;
	}
}
void CBlobLabeling::SetParam(IplImage* image, int nThreshold)
//���̺���� �Ӱ谪 ����.
//�Ӱ谪���� ������ ���� �ȼ���.
//���̺�� ����Ǵ� �ּ� �ȼ� ����.
//���� ũ�� �⿵���� ���ŵǰ�, ���� ������ ������ ������ ���� �� �ִ�.
{
	if (m_recBlobs != NULL)
	{
		delete m_recBlobs;

		m_recBlobs = NULL;
		m_nBlobs = _DEF_MAX_BLOBS;
	}

	if (m_Image != NULL)	cvReleaseImage(&m_Image);

	m_Image = cvCloneImage(image);

	m_nThreshold = nThreshold;
}
void CBlobLabeling::DoLabeling()
//���̺��� ȣ���ϴ� �Լ�
{
	m_nBlobs = Labeling(m_Image, m_nThreshold);
}

int CBlobLabeling::Labeling(IplImage* image, int nThreshold)
//���̺��� ����.
//�̹����� �Ӱ谪�� �޾Ƽ� �����Ѵ�.
//�̹����� ���̿� �ʺ� �޾Ƽ� �� ���� ���� ��ŭ�� ���۸� �����Ѵ�.
{
	if (image->nChannels != 1) 	return 0;

	int nNumber;

	int nWidth = image->width;
	int nHeight = image->height;

	unsigned char* tmpBuf = new unsigned char[nWidth * nHeight];

	int i, j;

	for (j = 0; j < nHeight; j++)
		for (i = 0; i < nWidth; i++)
			tmpBuf[j*nWidth + i] = (unsigned char)image->imageData[j*image->widthStep + i];

	// ���̺��� ���� ����Ʈ �ʱ�ȭ
	InitvPoint(nWidth, nHeight);

	// ���̺�
	nNumber = _Labeling(tmpBuf, nWidth, nHeight, nThreshold);

	// ����Ʈ �޸� ����
	DeletevPoint();

	if (nNumber != _DEF_MAX_BLOBS)		m_recBlobs = new CvRect[nNumber];

	if (nNumber != 0)	DetectLabelingRegion(nNumber, tmpBuf, nWidth, nHeight);

	for (j = 0; j < nHeight; j++)
		for (i = 0; i < nWidth; i++)
			image->imageData[j*image->widthStep + i] = tmpBuf[j*nWidth + i];

	delete tmpBuf;
	return nNumber;
}
void CBlobLabeling::InitvPoint(int nWidth, int nHeight)
// m_vPoint �ʱ�ȭ �Լ�
//m_vPoint���� �湮�� �ȼ��� üũ���ش�.
//�ʺ�� ���̸� ���Ѹ�ŭ�� �湮 �迭�� ����.
{
	int nX, nY;

	m_vPoint = new Visited[nWidth * nHeight];

	for (nY = 0; nY < nHeight; nY++)
	{
		for (nX = 0; nX < nWidth; nX++)
		{
			m_vPoint[nY * nWidth + nX].bVisitedFlag = false;
			m_vPoint[nY * nWidth + nX].ptReturnPoint.x = nX;
			m_vPoint[nY * nWidth + nX].ptReturnPoint.y = nY;
		}
	}
}

void CBlobLabeling::DeletevPoint()
//m_vPoint �ı���
{
	delete m_vPoint;
}
int CBlobLabeling::_Labeling(unsigned char *DataBuf, int nWidth, int nHeight, int nThreshold)
// Size�� nWidth�̰� nHeight�� DataBuf���� 
// nThreshold(�Ӱ谪)���� ���� ������ ������ �������� blob���� ȹ��
{
	//int Index = 0,
	int num = 0;
	int nX, nY, k, l;
	int StartX, StartY, EndX, EndY;

	//����� ������Ʈ�� ã�´�.
	//y���� ����, x���� �ʺ�
	//x����� Ȯ��
	for (nY = 0; nY < nHeight; nY++)
	{
		for (nX = 0; nX < nWidth; nX++)
		{
			if (DataBuf[nY * nWidth + nX] == 255)
				//���ο� ������Ʈ�ΰ� Ȯ��.
				//������Ʈ == 255(���)
			{
				num++;

				DataBuf[nY * nWidth + nX] = num;

				StartX = nX, StartY = nY, EndX = nX, EndY = nY;

				__NRFIndNeighbor(DataBuf, nWidth, nHeight, nX, nY, &StartX, &StartY, &EndX, &EndY);

				if (__Area(DataBuf, StartX, StartY, EndX, EndY, nWidth, num) < nThreshold)
				{
					for (k = StartY; k <= EndY; k++)
					{
						for (l = StartX; l <= EndX; l++)
						{
							if (DataBuf[k * nWidth + l] == num)
								DataBuf[k * nWidth + l] = 0;
						}
					}
					--num;

					if (num > 250)
						return  0;
				}
			}
		}
	}

	return num;
}


void CBlobLabeling::DetectLabelingRegion(int nLabelNumber, unsigned char *DataBuf, int nWidth, int nHeight)
// Blob labeling�ؼ� ����� ����� rec�� ��
{
	int nX, nY;
	int nLabelIndex;

	bool bFirstFlag[255] = { false, };

	for (nY = 1; nY < nHeight - 1; nY++)
	{
		for (nX = 1; nX < nWidth - 1; nX++)
		{
			nLabelIndex = DataBuf[nY * nWidth + nX];

			if (nLabelIndex != 0)	// Is this a new component?, 255 == Object
			{
				if (bFirstFlag[nLabelIndex] == false)
				{//�÷��װ� false�� �ʱ�ȭ
					m_recBlobs[nLabelIndex - 1].x = nX;
					m_recBlobs[nLabelIndex - 1].y = nY;
					m_recBlobs[nLabelIndex - 1].width = 0;
					m_recBlobs[nLabelIndex - 1].height = 0;

					bFirstFlag[nLabelIndex] = TRUE;
				}
				else
					//�÷��װ� Ʈ���
					// ���� ������ ���� ����, ���� �Ʒ� �κ��� ���� ����
				{
					int left = m_recBlobs[nLabelIndex - 1].x;
					int right = left + m_recBlobs[nLabelIndex - 1].width;
					int top = m_recBlobs[nLabelIndex - 1].y;
					int bottom = top + m_recBlobs[nLabelIndex - 1].height;

					//�� ���Ⱚ�� ���� x�� y��ǥ�� �缳��

					if (left >= nX)	left = nX;
					if (right <= nX)	right = nX;
					if (top >= nY)	top = nY;
					if (bottom <= nY)	bottom = nY;

					//������ ���Ⱚ�� �������� ��� �簢���� ��ǥ ����.

					m_recBlobs[nLabelIndex - 1].x = left;
					m_recBlobs[nLabelIndex - 1].y = top;
					m_recBlobs[nLabelIndex - 1].width = right - left;
					m_recBlobs[nLabelIndex - 1].height = bottom - top;

				}
			}

		}
	}

}


int CBlobLabeling::__NRFIndNeighbor(unsigned char *DataBuf, int nWidth, int nHeight, int nPosX, int nPosY, int *StartX, int *StartY, int *EndX, int *EndY)
// Blob Labeling�� ���� ���ϴ� function
{
	CvPoint CurrentPoint;

	CurrentPoint.x = nPosX;
	CurrentPoint.y = nPosY;

	m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].bVisitedFlag = TRUE;
	m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.x = nPosX;
	m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.y = nPosY;

	while (1)
	{
		if ((CurrentPoint.x != 0) && (DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x - 1] == 255))   // -X ����
		{
			if (m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x - 1].bVisitedFlag == false)
			{
				DataBuf[CurrentPoint.y  * nWidth + CurrentPoint.x - 1] = DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x];	// If so, mark it
				m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x - 1].bVisitedFlag = TRUE;
				m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x - 1].ptReturnPoint = CurrentPoint;
				CurrentPoint.x--;

				if (CurrentPoint.x <= 0)
					CurrentPoint.x = 0;

				if (*StartX >= CurrentPoint.x)
					*StartX = CurrentPoint.x;

				continue;
			}
		}

		if ((CurrentPoint.x != nWidth - 1) && (DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x + 1] == 255))   // -X ����
		{
			if (m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x + 1].bVisitedFlag == false)
			{
				DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x + 1] = DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x];	// If so, mark it
				m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x + 1].bVisitedFlag = TRUE;
				m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x + 1].ptReturnPoint = CurrentPoint;
				CurrentPoint.x++;

				if (CurrentPoint.x >= nWidth - 1)
					CurrentPoint.x = nWidth - 1;

				if (*EndX <= CurrentPoint.x)
					*EndX = CurrentPoint.x;

				continue;
			}
		}

		if ((CurrentPoint.y != 0) && (DataBuf[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x] == 255))   // -X ����
		{
			if (m_vPoint[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x].bVisitedFlag == false)
			{
				DataBuf[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x] = DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x];	// If so, mark it
				m_vPoint[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x].bVisitedFlag = TRUE;
				m_vPoint[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x].ptReturnPoint = CurrentPoint;
				CurrentPoint.y--;

				if (CurrentPoint.y <= 0)
					CurrentPoint.y = 0;

				if (*StartY >= CurrentPoint.y)
					*StartY = CurrentPoint.y;

				continue;
			}
		}

		if ((CurrentPoint.y != nHeight - 1) && (DataBuf[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x] == 255))   // -X ����
		{
			if (m_vPoint[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x].bVisitedFlag == false)
			{
				DataBuf[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x] = DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x];	// If so, mark it
				m_vPoint[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x].bVisitedFlag = TRUE;
				m_vPoint[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x].ptReturnPoint = CurrentPoint;
				CurrentPoint.y++;

				if (CurrentPoint.y >= nHeight - 1)
					CurrentPoint.y = nHeight - 1;

				if (*EndY <= CurrentPoint.y)
					*EndY = CurrentPoint.y;

				continue;
			}
		}

		if ((CurrentPoint.x == m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.x)
			&& (CurrentPoint.y == m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.y))
		{
			break;
		}
		else
		{
			CurrentPoint = m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint;
		}
	}

	return 0;
}

// ������ ���� blob�� Į�� ���� ������ ũ�⸦ ȹ��
int CBlobLabeling::__Area(unsigned char *DataBuf, int StartX, int StartY, int EndX, int EndY, int nWidth, int nLevel)
{
	int nArea = 0;
	int nX, nY;

	for (nY = StartY; nY < EndY; nY++)
		for (nX = StartX; nX < EndX; nX++)
			if (DataBuf[nY * nWidth + nX] == nLevel)
				++nArea;

	return nArea;
}
/******************************************************************************************************/
////////////////////////////////////////////
// �⿵ ���Ÿ� ���� ���̺� ��ƾ///////////
////////////////////////////////////////////
// nWidth�� nHeight���� ���� rec�� ��� ����
void CBlobLabeling::BlobSmallSizeConstraint(int nWidth, int nHeight)
{
	_BlobSmallSizeConstraint(nWidth, nHeight, m_recBlobs, &m_nBlobs);
}

void CBlobLabeling::_BlobSmallSizeConstraint(int nWidth, int nHeight, CvRect* rect, int *nRecNumber)
{
	if (*nRecNumber == 0)	return;

	int nX;
	int ntempRec = 0;

	CvRect* temp = new CvRect[*nRecNumber];

	for (nX = 0; nX < *nRecNumber; nX++)
	{
		temp[nX] = rect[nX];
	}

	for (nX = 0; nX < *nRecNumber; nX++)
	{
		if ((rect[nX].width > nWidth) && (rect[nX].height > nHeight))
		{
			temp[ntempRec] = rect[nX];
			ntempRec++;
		}
	}

	*nRecNumber = ntempRec;

	for (nX = 0; nX < *nRecNumber; nX++)
		rect[nX] = temp[nX];

	delete temp;
}
// nWidth�� nHeight���� ū rec�� ��� ����

void CBlobLabeling::BlobBigSizeConstraint(int nWidth, int nHeight)
{
	_BlobBigSizeConstraint(nWidth, nHeight, m_recBlobs, &m_nBlobs);
}

void CBlobLabeling::_BlobBigSizeConstraint(int nWidth, int nHeight, CvRect* rect, int* nRecNumber)
{
	if (*nRecNumber == 0)	return;

	int nX;
	int ntempRec = 0;

	CvRect* temp = new CvRect[*nRecNumber];

	for (nX = 0; nX < *nRecNumber; nX++)
	{
		temp[nX] = rect[nX];
	}

	for (nX = 0; nX < *nRecNumber; nX++)
	{
		if ((rect[nX].width < nWidth) && (rect[nX].height < nHeight))
		{
			temp[ntempRec] = rect[nX];
			ntempRec++;
		}
	}

	*nRecNumber = ntempRec;

	for (nX = 0; nX < *nRecNumber; nX++)
		rect[nX] = temp[nX];

	delete temp;
}