
#include "stdafx.h"

#define MD5_LENGTH 16

int loadUpdater(const char *filename);
int loadUboot(const char *filename);
int RunScript(const char *filename);
int IdentifyDevice(CString str);//modified by TX