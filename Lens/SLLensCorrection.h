#include "SLLensParam.h"
#include <qpoint.h>
#include <qrect.h>
#include <vector>
using namespace std;

class SLSourcePoint
{
	friend class SLLensCorrection;
public:
	SLSourcePoint();
	~SLSourcePoint() {}

	void initSrcPoint();

private:
	SLLens_Src_POINT m_Src_Point[65][65];  //源坐标值表
};

class SLLENSCORRECTION_EXPORT SLLensCorrection
{
	static SLLensCorrection* m_this;
	SLLensCorrection();
public:

	static SLLensCorrection *instance();

	void release();

	/**
	*\biref
	*设置，获取振镜参数
	*/
	void setParam(SLLensParam &param);
	SLLensParam Param()const;
	
	/**
	*\biref
	*使用校正函数
	*/
	void refCorrection();


	/**
	*\brief
	*计算每一个纠正网格坐标值
	*/
	void formulaPoint(float &x, float &y);

	/**
	*\brief
	*将校正表写入文件
	*fileName: 文件名(包含路径，及后缀)
	*/
	bool writeFile(QString fileName);


	/**
	*\biref
	*读取振镜校正表,将校正表写入数据结构中
	*fileName: 校正表文件名(包含路径,后缀)
	*/
	bool readFile(QString fileName);

	/**
	*\brief
	*是否已加载校正表
	*/
	bool loadLens();

	/**
	*\biref
	*设置加载振镜校正,只做标志位处理，不实际加载振镜校正表,需要加载振镜校正表用: readFile(QString) 函数
	*/
	void setLensLoad(bool);

	
	/**
	*\biref
	*对网格坐标值校正表进行线性插值
	*x and y: 为需要校正的点
	*ratio : 将x and y按比例缩放成-32768 ~ 32768 范围内的像素点
	*reutrn 经过补偿后的point
	*/
	QPointF& linearInterpolation(double x,double y,double ratio);
		

private:
	/**
	*\brief
	*计算X方向的补偿量
	*dx1,dx2,dx3,dx4：为网格四个顶点的补偿量
	*x1,x2,y1,y2: 为四个网格顶点
	*x and y: 坐标x and y值
	*return x的补偿量
	*得到补偿量加上源点就是校正后的点
	*/
	double Calculate_X_Direction(double dx1, double dx2,double dx3,double dx4,double x1,double x2, double y1,double y2, double x , double y);

	/**
	*\biref
	*计算y方向的补偿量
	*dy1,dy2,dy3,dy4：为网格四个顶点的补偿量
	*x1,x2,y1,y2: 为四个网格顶点
	*x and y: 坐标x and y值
	*return x的补偿量
	*得到补偿量加上源点就是校正后的点
	*/
	double Calculate_Y_Direction(double dy1,double dy2,double dy3,double dy4,double y1,double y2,double x1,double x2,double x,double y);


private:

	SLLens_COR_POINT m_Cor_Point[MAX_GRID_SIZE][MAX_GRID_SIZE];  //坐标值校正表

	SLLensParam m_Param;

	QRectF m_WorkRange;
	
	QPointF pStart;

	bool m_isLoad;
};


