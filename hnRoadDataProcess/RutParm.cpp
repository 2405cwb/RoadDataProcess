#include "RutParm.h"
 
RutParm::RutParm(QString iniFile)
{
	iniService = configService::getPtr();
	iniService->loadCfg(iniFile); 
	_asp = iniService->ReadInteger("camera", "rutastart", 0); 
	_aep =iniService->ReadInteger("camera", "rutaend", 2048);
	_bsp =iniService->ReadInteger("camera", "rutbstart", 0);
	_bep =iniService->ReadInteger("camera", "rutbend", 2048);
	_csp =iniService->ReadInteger("camera", "rutcstart", 0);
	_cep =iniService->ReadInteger("camera", "rutcend", 2048);
	_hpixel = iniService->ReadInteger("camera", "hpixel", 2048);
	_pixsize = iniService->ReadInteger("camera", "pixsize", 2); 
	_gslen = iniService->ReadInteger("rut", "gslen", 32) / 2 * 2 + 1;
	_scaleval = iniService->ReadInteger("rut", "scaleval", 10);
	_partlen = _aep - _asp - 2;//157¡¢170¡¢186
	_threshval = iniService->ReadInteger("rut", "threshval", 28);
	_ThrPoint = iniService->ReadInteger("rut", "threshpointnum", _partlen / 4);
}

RutParm::RutParm()
{

}

RutParm::~RutParm()
{
}
