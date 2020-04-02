#include <qrect.h>

const int MAX_GRID_SIZE = 65;


struct SLLensParam
{
	float offsetX;
	float offsetY;
	double centerXY;
	double angle;
	float mirrorX;
	float mirrorY;

	float distorX;         //变形 x axis
	float distorY;         //变形 y axis
	float OrthoX;          //倾斜 x axis
	float OrthoY;		   //倾斜 y axis
	float trazpeoidX;      //梯形 x axis
	float trazpeoidY;      //梯形 y axis
	float ratioX;		   //比例 x axis
	float ratioY;		   //比例 y axis

	QRectF LenArea;		   //振镜工作范围
};


struct SLLens_COR_POINT
{
	float fx;
	float fy;
};

struct SLLens_Src_POINT
{
	float fx;
	float fy;
};
