# LensCorrection
laser grid correction method

example:

void main()
{
  SLLensCorrection * lens = SLLensCorrection::instance();
  /*
  //加载振镜校正表,只加载一次
	if (!lens->loadLens())   //未加载
	{
		QString filename = "*.ctf";
    bool error = SLLensCorrection::instance()->readFile(filename);
		if (!error)   //文件不存在,加载失败
		{
			//使用默认参数校正
			lens->refCorrection();
			lens->writeFile(filename);
		}
	}	
  */
  //x and y : coordinate point, ratio: point scale
  QPoint p = lens->linearInterpolation(x, y,ratio);
}
