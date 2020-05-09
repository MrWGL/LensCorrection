#include "SLLensCorrection.h"
#include <algorithm>
#include <cmath>
#include <qstring.h>
#include <qfile.h>
#include <qvector.h>
#include <qdebug.h>
#include  <iostream>
//#include <file>

SLLensCorrection* SLLensCorrection::m_this = nullptr;

const double degree = 3.14159 / 180;
const double maxPixel = 65535;

SLSourcePoint srcPoint;



SLLensCorrection::SLLensCorrection()
{
	m_Param.angle = 0.0;
	m_Param.centerXY = 0.0;
	m_Param.offsetX = 0.0;
	m_Param.offsetY = 0.0;

	m_Param.distorX = 0.0;
	m_Param.distorY = 0.0;
	m_Param.trazpeoidX = 0.0;
	m_Param.trazpeoidY = 0.0;
	m_Param.OrthoX = 0.0;
	m_Param.OrthoY = 0.0;
	m_Param.ratioX = 1;
	m_Param.ratioY = 1;

	m_Param.LenArea = { 0,0,100,100 };

	m_isLoad = false;
}

SLLensCorrection* SLLensCorrection::instance()
{
	if (m_this == nullptr)
	{
		m_this = new SLLensCorrection;
	}

	return m_this;
}

void SLLensCorrection::release()
{
	if (m_this)
	{
		delete m_this;
		m_this = nullptr;
	}
}


void SLLensCorrection::setParam(SLLensParam &p)
{
	m_Param = p;
}

SLLensParam SLLensCorrection::Param()const
{
	return m_Param;
}


void SLLensCorrection::refCorrection()
{

	int ix, iy;
	float fx = -32768;
	float fy = -32768;
	float fGap = 1024;

	for (ix = 0; ix < MAX_GRID_SIZE; ix++)
	{
		for (iy = 0; iy < MAX_GRID_SIZE; iy++)
		{
			m_Cor_Point[ix][iy].fx = fx;
			m_Cor_Point[ix][iy].fy = fy;

			fy += fGap;
		}

		fx += fGap;

		fy = -32768;
	}


	for (ix = 0; ix < MAX_GRID_SIZE; ix++)
	{
		for (iy = 0; iy < MAX_GRID_SIZE; iy++)
		{
			formulaPoint(m_Cor_Point[ix][iy].fx, m_Cor_Point[ix][iy].fy);
		}
	}
}


void SLLensCorrection::formulaPoint(float &x, float &y)
{
	double dx, dy;

	dx = x;
	dy = y;

	double sx, sy;

	if (m_Param.OrthoX != 0)
		sx = tan(m_Param.OrthoX * degree);
	else
		sx = 0;

	if (m_Param.OrthoY != 0)
		sy = tan(m_Param.OrthoY * degree);
	else
		sy = 0;

	double dNewX = dx + sx * dy;
	double dNewY = dy + sy * dx;

	dx = dNewX;
	dy = dNewY;

	if (m_Param.trazpeoidX != 0)
	{
		dx = dNewX + (dNewX * dNewY * 4 * m_Param.trazpeoidX / (655.35 * 655.35));
	}

	if (m_Param.trazpeoidY != 0)
	{
		dy = dNewY + (dNewY * dNewX * 4 * m_Param.trazpeoidY / (655.35 * 655.35));
	}

	if (m_Param.distorX != 0 || m_Param.distorY != 0)
	{
		double dxx = (dy * dy) / maxPixel;
		dxx = (dxx * dx) / maxPixel;
		dxx = (dxx * m_Param.distorX);
		dxx = dx + dxx;

		double dyy = (dx * dx) / maxPixel;
		dyy = (dy  * dyy) / maxPixel;
		dyy = (dyy * m_Param.distorY);
		dyy = dy + dyy;

		dx = dxx;
		dy = dyy;
	}

	dx = dx * m_Param.ratioX;
	dy = dy * m_Param.ratioY;

	x = (float)dx;
	y = (float)dy;
}

bool SLLensCorrection::writeFile(QString fileName)
{
	if (fileName.isEmpty())
		return false;

	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		return false;
	}

	int iX, iY;
	for (iX = 0; iX < MAX_GRID_SIZE; iX++)
	{
		for (iY = 0; iY < MAX_GRID_SIZE; iY++)
		{

			qint64 x = file.write((const char*)&m_Cor_Point[iX][iY].fx, sizeof(float));
			qint64 y = file.write((const char*)&m_Cor_Point[iX][iY].fy, sizeof(float));
		}
	}

	file.close();

	return true;
}

bool SLLensCorrection::readFile(QString fileName)
{
	QFile file(fileName);
	if (!file.exists())
	{
		m_isLoad = false;
		return false;
	}

    if (file.open(QIODevice::ReadOnly))
	{
		for (int ix = 0; ix < MAX_GRID_SIZE; ix++)
		{
			for (int iy = 0; iy < MAX_GRID_SIZE; iy++)
			{
				float x = 0, y = 0;
				file.read((char*)&x, sizeof(float));
				file.read((char*)&y, sizeof(float));
				m_Cor_Point[ix][iy].fx = 0;
				m_Cor_Point[ix][iy].fy = 0;

				m_Cor_Point[ix][iy].fx = x;
				m_Cor_Point[ix][iy].fy = y;
			}
		}
	}

	file.close();
	m_isLoad = true;

	return true;
}

bool SLLensCorrection::loadLens()
{
	return m_isLoad;
}

void SLLensCorrection::setLensLoad(bool b)
{
	m_isLoad = b;
}


QPointF& SLLensCorrection::linearInterpolation(double x, double y, double ratio)
{	

	x *= ratio;
	y *= ratio;

	pStart.setX(x);
	pStart.setY(y);
	if (x == 30000 && y == (-29670))
	{
		int a = 0;
	}


	//缩小查找范围,更快找到 point 所在区域,优化整个图形校正所需时间

	//将坐标系划分为四个象限，判断点在哪一个象限中
	/*(0,0)在中心,在第4象限网格中查找,(4:x>0,y<0)象限在右上角，并非按照笛卡尔坐标系划分象限*/
	if (pStart.x() > 0 && pStart.y() < 0)
	{
		//将网格划分区域，减少x轴搜索范围 (x > 0 && x < 32768 / 4)
		if (pStart.x() > 0 && pStart.x() < m_Cor_Point[(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 8)][0].fx)
		{
            for (int iX = MAX_GRID_SIZE / 2 + 1; iX < 48; iX++)    //缩小搜索范围
			{
				//缩小 y 轴搜索范围 y < 0 && y > -(32768 / 2)
                if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 4)].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE / 2; iY++)   //缩小搜索范围
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

							//计算 x point 补偿量
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;							
							x = pStart.x() / ratio + xInterpolation;
							
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小 y 轴搜索范围, y < -(32768/2) && y > -32768
				else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
                    for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

							//计算 x point 补偿量
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;					
							x = pStart.x() / ratio + xInterpolation;
							
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		//x > 32768 / 4 && x < 32768 / 2
        else if (pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 8 ]->fx && pStart.x() < m_Cor_Point[MAX_GRID_SIZE -  MAX_GRID_SIZE / 4]->fx)
		{
            for (int iX = (MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 8) + 1; iX < MAX_GRID_SIZE; iX++)
			{
				// y < 0 && y > -(32768 / 2)
                if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

							//计算 x point 补偿量
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}

                else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
                    for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

                            //计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
                            double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

                            //计算 y point 补偿量
                            double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

                            pStart.setX(x);
                            pStart.setY(y);
                            return pStart;
						}
					}
				}
			}
		}

		// 缩小 x 轴搜索范围, x > (32768 / 2) && x < (32768 / 2 + 32768 / 4)
		else if (pStart.x() > m_Cor_Point[(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)]->fx &&pStart.x() < m_Cor_Point[MAX_GRID_SIZE - MAX_GRID_SIZE / 8][0].fx)

		{
			for (int iX = (MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 8) + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围, y < 0 && y > -(32768 / 2)
				if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 4 )].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE / 2; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

							//计算 x point 补偿量
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围, y < -(32768 / 2) && y > -32768
				else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4 + 1].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
					for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

                            //计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
                            double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

                            //计算 y point 补偿量
                            double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

                            pStart.setX(x);
                            pStart.setY(y);
                            return pStart;
						}
					}
				}
			}
		}
        else if (pStart.x() > m_Cor_Point[MAX_GRID_SIZE -  MAX_GRID_SIZE / 4]->fx && pStart.x() < m_Cor_Point[MAX_GRID_SIZE - 1]->fx)
		{
            for (int iX = MAX_GRID_SIZE - MAX_GRID_SIZE / 4 + 1; iX < MAX_GRID_SIZE; iX++)
			{
				if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][16].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE / 2; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//double dGap = 1024.00;

							//计算 x point 补偿量
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围, y < -(32768 / 2) && y > -32768
				else if (pStart.y() < m_Cor_Point[iX][16 + 1].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
					for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{

						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

                            //计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
                            double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

                            //计算 y point 补偿量
                            double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

                            pStart.setX(x);
                            pStart.setY(y);
                            return pStart;
						}
					}
				}
			}
		}
		double x = (pStart.x()) / ratio;
		double y = (pStart.y()) / ratio;
		pStart.setX(x);
		pStart.setY(y);
		return pStart;
	}

	else if (pStart.x() > 0 && pStart.y() > 0)   //(0,0)在中心，在第一象限网格中查找 x: 0 ~ 32768 y: 0 ~ 32768
	{
		//缩小X轴搜索范围, x > 0 && x < (32768 / 4) 
		if (pStart.x() > 0 && pStart.x() < m_Cor_Point[(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 8)][0].fx)
		{
			for (int iX = MAX_GRID_SIZE / 2 + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围, y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE ; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;							

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围,y > (32768 / 2 ) && y < 32768
				else if (pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy)
				{
					/*****在此未计算到 iX: 47 iY : 57****/
					for (int iY = ((MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)) + 1; iY < MAX_GRID_SIZE; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		//x > (32768 / 4) && x < (32768 / 2)
		else if (pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 8]->fx && pStart.x() < m_Cor_Point[MAX_GRID_SIZE - MAX_GRID_SIZE / 4]->fx)
		{
			for (int iX = (MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 8) + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围, y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围, y > (32768 / 2) && y < 32768
				else if (pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy &&
					pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy);
				{
					for (int iY = (MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4) + 1; iY < MAX_GRID_SIZE; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		//缩小X轴搜索范围,x > (32768 / 2) && x < (32768 / 2 + 32768 / 4)
        else if (pStart.x() > m_Cor_Point[(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)]->fx && pStart.x() < m_Cor_Point[MAX_GRID_SIZE - MAX_GRID_SIZE / 8][0].fx)
		{
			for (int iX = (MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 8) + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围, y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE ; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围, y > (32768 / 2) && y < 32768
				else if (pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy &&
					pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy);
				{
                    for (int iY = (MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4) + 1; iY < MAX_GRID_SIZE; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		//x > (32768 - 32768 / 4) && x < 32768
		else if (pStart.x() > m_Cor_Point[MAX_GRID_SIZE - MAX_GRID_SIZE / 4]->fx && pStart.x() < m_Cor_Point[MAX_GRID_SIZE - 1]->fx)
		{
			for (int iX = MAX_GRID_SIZE - MAX_GRID_SIZE / 4 + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围, y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围, y > (32768 / 2) && y < 32768
				else if (pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy &&
					pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy);
				{
					for (int iY = (MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4) + 1; iY < MAX_GRID_SIZE; iY++)
					{

						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fy, srcPoint.m_Src_Point[iX][iY].fy, srcPoint.m_Src_Point[iX - 1][iY - 1].fx, srcPoint.m_Src_Point[iX][iY].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		double x = (pStart.x()) / ratio;
		double y = (pStart.y()) / ratio;
		pStart.setX(x);
		pStart.setY(y);
		return pStart;
	}
	else if (pStart.x() < 0 && pStart.y() > 0)    //(0,0)在中心,在第二象限网格中查找x:0 ~ -32768 y:0 ~ 32768
	{
		//缩小X轴搜索范围,x < 0 && x > -(32768/4)
		if (pStart.x() < 0 && pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 4 + MAX_GRID_SIZE / 8][0].fx)
		{
			for (int iX = (MAX_GRID_SIZE / 4 + MAX_GRID_SIZE / 8) + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围,y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < (MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4); iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;							
							x = pStart.x() / ratio + xInterpolation;						
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围,y > (32768 / 2 ) && y < 32768
				else if (pStart.y() > m_Cor_Point[iX][(MAX_GRID_SIZE / 2) + (MAX_GRID_SIZE / 4)].fy && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;							
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}


		//x < -(32768 / 4) && x > -(32768 / 2)
		else if (pStart.x() < m_Cor_Point[MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 8]->fx && pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 4]->fx)
		{
			for (int iX = MAX_GRID_SIZE / 4 + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围,y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				else if (pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4].fy && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy)
				{
					for (int iY = (MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4) + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}


		//缩小X轴搜索范围,x < -(32768 / 2) && x > -(32768 / 2 + 32768 / 4)
		else if (pStart.x() < m_Cor_Point[MAX_GRID_SIZE / 4][0].fx && pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 8][0].fx)
		{
			for (int iX = MAX_GRID_SIZE / 8 + 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围,y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}

				else if (pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4].fy && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy)
				{
					for (int iY = (MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4) + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

                            //计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
                            double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

                            //计算 y point 补偿量
                            double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

                            pStart.setX(x);
                            pStart.setY(y);
                            return pStart;
						}
					}
				}
			}
		}



		//x < -(32768 - 32768 / 4) && x > -(32768)
		else if (pStart.x() < m_Cor_Point[MAX_GRID_SIZE / 4]->fx && pStart.x() > m_Cor_Point[0]->fx)
		{
            for (int iX = 1; iX < MAX_GRID_SIZE; iX++)
			{
				//缩小Y轴搜索范围,y > 0 && y < (32768 / 2)
				if (pStart.y() > 0 && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4].fy)
				{
					for (int iY = MAX_GRID_SIZE / 2 + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx,
								m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());
							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;
							
							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}


				else if (pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 2 + MAX_GRID_SIZE / 4].fy && pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE - 1].fy)
				{
					for (int iY = (MAX_GRID_SIZE / 2 ) + 1; iY < MAX_GRID_SIZE; iY++)
					{
						if (pStart.x() >= m_Cor_Point[iX - 1][iY].fx && pStart.y() <= m_Cor_Point[iX][iY].fy &&
							pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

                            //计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
                            double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

                            //计算 y point 补偿量
                            double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

                            double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

                            pStart.setX(x);
                            pStart.setY(y);
                            return pStart;
						}
					}
				}
			}
		}
		double x = (pStart.x()) / ratio;
		double y = (pStart.y()) / ratio;
		pStart.setX(x);
		pStart.setY(y);
		return pStart;
	}
	//(0,0)在中心,在第三象限网格中查找, x: 0 ~ (-32768) y: 0 ~ (-32768)
	else if (pStart.x() < 0 && pStart.y() < 0)
	{
		//缩小X轴搜索范围, x < 0 && x > -(32768 / 4)
        if (pStart.x() < 0 && pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 4 + MAX_GRID_SIZE / 8 ][0].fx)
		{
            for (int iX =  20; iX < MAX_GRID_SIZE / 2 ; iX++)
			{
				//缩小Y轴搜索范围,y < 0 && y > -(32768 / 2)
				if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围,y < -(32768 / 2) && y > -32768
				else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
                    for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		//x < -(32768 / 4) && x > - (32768 / 2)
        else if (pStart.x() < m_Cor_Point[MAX_GRID_SIZE / 4 + MAX_GRID_SIZE / 8]->fx && pStart.x() > m_Cor_Point[MAX_GRID_SIZE / 4 ]->fx)
		{
            for (int iX =  12; iX < MAX_GRID_SIZE / 2; iX++)
			{
				//缩小Y轴搜索范围,y < 0 && y > -(32768 / 2)
				if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围,y < -(32768 / 2) && y > -32768
				else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
                    for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
                            x = pStart.x() / ratio + xInterpolation;
                            y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		//x < -(32768 / 2) && x > - (32768 - 32768 / 4)
        else if (pStart.x() < m_Cor_Point[MAX_GRID_SIZE / 4]->fx && pStart.x() > m_Cor_Point[ MAX_GRID_SIZE / 8]->fx)
		{
			for (int iX = MAX_GRID_SIZE / 16 + 1; iX < MAX_GRID_SIZE / 2; iX++)
			{
				//缩小Y轴搜索范围,y < 0 && y > -(32768 / 2)
				if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy)
				{
                    for (int iY = 16+ 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围,y < -(32768/2) && y > -32768
                else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
					for (int iY = 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1]->fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX]->fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

        else if (pStart.x() < m_Cor_Point[MAX_GRID_SIZE / 8]->fx && pStart.x() > m_Cor_Point[0]->fx)
		{
			for (int iX = 1; iX < MAX_GRID_SIZE / 2; iX++)
			{
				//缩小Y轴搜索范围,y < 0 && y > -(32768 / 2)
                if (pStart.y() < 0 && pStart.y() > m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy)
				{
                    for (int iY = 16 + 1; iY < MAX_GRID_SIZE / 2; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
				//缩小Y轴搜索范围,y < -(32768/2) && y > -32768
				else if (pStart.y() < m_Cor_Point[iX][MAX_GRID_SIZE / 4].fy && pStart.y() > m_Cor_Point[iX][0].fy)
				{
                    for (int iY = 1; iY < MAX_GRID_SIZE / 2 ; iY++)
					{
						if (pStart.x() > m_Cor_Point[iX - 1][iY].fx && pStart.y() < m_Cor_Point[iX][iY].fy
							&& pStart.x() < m_Cor_Point[iX][iY].fx  && pStart.y() > m_Cor_Point[iX][iY - 1].fy)
						{
							//每个网格的大小为1024
							//double dGap = 1024.00;

							//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
							double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

							//计算 y point 补偿量
							double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

							double x, y;
							x = pStart.x() / ratio + xInterpolation;
							y = pStart.y() / ratio + yInterpolation;

							pStart.setX(x);
							pStart.setY(y);
							return pStart;
						}
					}
				}
			}
		}

		double x = (pStart.x()) / ratio;
		double y = (pStart.y()) / ratio;
		pStart.setX(x);
		pStart.setY(y);
		return pStart;
	}
	else      //处于(0,0) 或点不在范围内，不做处理
	{
		double x = (pStart.x()) / ratio;
		double y = (pStart.y()) / ratio;
		pStart.setX(x);
		pStart.setY(y);
		return pStart;
	}

}


QPointF& SLLensCorrection::linearInterpolation(QPointF p, double ratio)
{
	double x = p.x() * ratio;
	double y = p.y() * ratio;

	pStart.setX(x);
	pStart.setY(y);

	int iX = 0, iY = 0;
	binarySearch(pStart,iX,iY);
	

	if (iX > (MAX_GRID_SIZE - 1))   //确保 iX and iY 不大于 64
		iX = MAX_GRID_SIZE - 1;
	if (iY > (MAX_GRID_SIZE - 1))
		iY = MAX_GRID_SIZE - 1;

	//计算 x point 补偿量,以左下角顶点为基准，减去相对应值，得到四个顶点值
	double xInterpolation = Calculate_X_Direction(m_Cor_Point[iX - 1][iY].fx, m_Cor_Point[iX][iY].fx, m_Cor_Point[iX - 1][iY - 1].fx, m_Cor_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, pStart.x(), pStart.y());

	//计算 y point 补偿量
	double yInterpolation = Calculate_Y_Direction(m_Cor_Point[iX - 1][iY].fy, m_Cor_Point[iX][iY].fy, m_Cor_Point[iX - 1][iY - 1].fy, m_Cor_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fy, srcPoint.m_Src_Point[iX][iY - 1].fy, srcPoint.m_Src_Point[iX - 1][iY].fx, srcPoint.m_Src_Point[iX][iY - 1].fx, pStart.x(), pStart.y());

	x = pStart.x() / ratio + xInterpolation;
	y = pStart.y() / ratio + yInterpolation;

	pStart.setX(x);
	pStart.setY(y);
	return pStart;
}

double SLLensCorrection::Calculate_X_Direction(double dx1, double dx2, double dx3, double dx4, double x1, double x2, double y1, double y2, double x, double y)
{
	double dD = 0.0, dT = 0.0;

	//用网格坐标补偿后的值 - 未补偿时初始值
	dx1 = dx1 - x1;
	dx2 = dx2 - x2;
	dx3 = dx3 - x1;
	dx4 = dx4 - x2;

	//对下部两点做线性算法
	dD = (dx1 * (x2 - x) + dx2 * (x - x1)) / (x2 - x1);

	//对上部两点做线性算法
	dT = (dx3 * (x2 - x) + dx4 * (x - x1)) / (x2 - x1);


	//对上下两部做线性算法
	double dx = (dD * (y2 - y) + dT * (y - y1)) / (y2 - y1);

	return dx;
}

double SLLensCorrection::Calculate_Y_Direction(double dy1, double dy2, double dy3, double dy4, double y1, double y2, double x1, double x2, double x, double y)
{
	double dL = 0.0, dR = 0.0;

	dy1 -= y1;
	dy2 -= y1;
	dy3 -= y2;
	dy4 -= y2;

	//对左部两点做线性算法
	dL = (dy1 * (y2 - y) + dy3 * (y - y1)) / (y2 - y1);

	//对右部两点做线性算法
	dR = (dy2 * (y2 - y) + dy4 * (y - y1)) / (y2 - y1);

	//对左右两部做线性算法
	double dy = (dL * (x2 - x) + dR * (x - x1)) / (x2 - x1);

	return dy;
}

SLSourcePoint::SLSourcePoint()
{
	initSrcPoint();
}


void SLSourcePoint::initSrcPoint()
{
	int ix, iy;
	float fx = -32768;
	float fy = -32768;
	float fGap = 1024;

	for (ix = 0; ix < MAX_GRID_SIZE; ix++)
	{
		for (iy = 0; iy < MAX_GRID_SIZE; iy++)
		{
			m_Src_Point[ix][iy].fx = fx;
			m_Src_Point[ix][iy].fy = fy;

			fy += fGap;
		}

		fx += fGap;

		fy = -32768;
	}
}


void  SLLensCorrection::binarySearch(const QPointF &p,int &x,int &y)
{
	int length = MAX_GRID_SIZE - 1;   //length = 64
	int iX = 0, iY = 0;

	int pos_x = 1 , pos_y = 1;
	while (iX <= length)   //查找X所在索引
	{
		int mid = iX + ((length - iX) >> 1);
		if (m_Cor_Point[mid]->fx > p.x())
		{
			length = mid - 1;
		}
		else if (m_Cor_Point[mid]->fx < p.x())
		{
			iX = mid + 1;
		}			

		if (p.x() > m_Cor_Point[mid - 1]->fx && p.x() <= m_Cor_Point[mid]->fx)
		{
			pos_x = mid;
			break;
		}
	}

	length = MAX_GRID_SIZE - 1;
	while (iY <= length)   //查找Y所在索引
	{
		int mid = iY + ((length - iY) >> 1);
		if (m_Cor_Point[pos_x][mid].fy > p.y())
		{
			length = mid - 1;
		}
		else if (m_Cor_Point[pos_x][mid].fy < p.y())
		{
			iY = mid + 1;
		}		

		if (p.y() <= m_Cor_Point[pos_x][mid].fy && p.y() > m_Cor_Point[pos_x][mid - 1].fy)
		{
			pos_y = mid;
			break;
		}
	}

	x = pos_x;
	y = pos_y;
}
