#include "hnWorkMode.h"

bool hnWorkMode::addLineDiseType = false;

hnWorkMode::hnWorkMode()
{
	littleDrawRectType = false;
}

hnWorkMode::~hnWorkMode()
{
}

void hnWorkMode::setAddDiseaseMode()
{
	this->m_workMode = WorkMode::ADD_MODE;
}

void hnWorkMode::setDeleteDiseaseMode()
{
	this->m_workMode = WorkMode::DELETE_MODE;
}

void hnWorkMode::setEditMode()
{
	this->m_workMode = WorkMode::EDIT_MODE;
}

void hnWorkMode::setMoveMode()
{
	this->m_workMode = WorkMode::MOVE;
}

void hnWorkMode::setAddCtrlPointMode()
{
	m_workMode = WorkMode::ADD_CTRL_POINT;
}

void hnWorkMode::setMode(WorkMode mode)
{
	m_workMode = mode;
}

hnWorkMode::WorkMode hnWorkMode::getMode()
{
	return m_workMode;
}


