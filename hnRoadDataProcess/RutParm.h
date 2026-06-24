#pragma once
#include "..\hnConfigService\configService.h"
class RutParm
{
public:
	RutParm();
	RutParm(QString iniFile);
	~RutParm();

public:
	int _partlen = 256;
	int _asp = 0;
	int _aep = 0;
	int _bsp = 0;
	int _bep = 0;
	int _csp = 0;
	int _cep = 0;
	int _gslen = 0;
	int _ThrPoint = 0;
	float _scaleval = 10;
	float _threshval = 0;
	int _hpixel = 2048;
	int _pixsize = 2;
private:
	configService * iniService;
};

