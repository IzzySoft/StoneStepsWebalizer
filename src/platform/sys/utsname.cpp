/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    utsname.cpp
*/
#include "../../pch.h"

#include <windows.h>
#include <errno.h>
#include <stdio.h>
#include "utsname.h"

extern "C" int uname(struct utsname *name)
{
	OSVERSIONINFO verinfo = {0};
	DWORD buffsize;

	if(name == NULL)
		return EFAULT;

	buffsize = SYS_NMLN;
	GetComputerName(name->machine, &buffsize);
	strcpy(name->nodename, name->machine);

	verinfo.dwOSVersionInfoSize = sizeof(verinfo);
	GetVersionEx(&verinfo);
	
	sprintf(name->version, "%d.%d.%d", verinfo.dwMajorVersion, verinfo.dwMinorVersion, verinfo.dwBuildNumber);
	strcpy(name->release, name->version);

	sprintf(name->sysname, "Windows");

	return 0;
}