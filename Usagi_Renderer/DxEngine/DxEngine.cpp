// DxEngine.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "DxEngine.h"

DxEngine* DxEngine::singleton_instance = nullptr;

DxEngine& DxEngine::Instance()
{
	if(singleton_instance == nullptr)
	{
		singleton_instance = new DxEngine;
	}
	return *singleton_instance;
}

void DxEngine::Destroy()
{
	delete singleton_instance;
}
