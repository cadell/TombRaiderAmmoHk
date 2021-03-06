// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <TlHelp32.h> //This header file contains the module32 and snapshot stuff
#include <iostream>
#include <vector>
//#include <string>


DWORD GetPidFromSnapShot(const WCHAR* processName);
DWORD ModuleBaseAddy(DWORD processID, const WCHAR* moduleName);
std::vector<DWORD> findAddy(HANDLE hProc, DWORD addy, std::vector<DWORD> baseAddyOffset, std::vector<std::vector<DWORD>> ammoBaseOffset);
int main()
{
	DWORD bow  = 0x010D5E10;
	DWORD ar15 = 0x01FD17DC;
	std::vector<DWORD> baseAddyOffset;
	baseAddyOffset.push_back(bow);
	baseAddyOffset.push_back(ar15);
	std::vector<DWORD> BowBaseOffset = { 0x1C,0x8,0x30,0x294,0x4C8 };
	std::vector<DWORD> ARLookingGunBaseOffset = { 0x18,0x28};
	std::vector<std::vector<DWORD>> ammoBaseOffset;
	ammoBaseOffset.push_back(BowBaseOffset);
	ammoBaseOffset.push_back(ARLookingGunBaseOffset);
	DWORD pid = GetPidFromSnapShot((L"TombRaider.exe"));
	std::cout << "process PID " << pid << std::endl;
	DWORD addyTest = ModuleBaseAddy(pid,L"TombRaider.exe");
	std::cout << "Base Addy " <<std::hex << addyTest << std::endl;

	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	auto hold = findAddy(processHandle,addyTest,baseAddyOffset,ammoBaseOffset);
	for (int i =0; i < hold.size();i++)
	{
		std::cout << "ammo addy " << std::hex << hold[i] << std::endl;
	}
	int* ammo_value = new int[hold.size()];
	for (int i = 0; i < hold.size(); i++)
	{
		ReadProcessMemory(processHandle, (LPCVOID)hold[i], &ammo_value[i], sizeof(ammo_value[i]), NULL);
		std::cout << "the value on this address is " << std::dec << ammo_value[i] << std::endl;
	}
	
	

	while(1)
	{
		int ammo_state = 420;
		for (int i = 0; i < hold.size(); i++)
		{
			ReadProcessMemory(processHandle, (LPCVOID)hold[i], &ammo_value[i], sizeof(ammo_value[i]), NULL);
		}
		
		for (int i = 0; i < hold.size(); i++)
		{
			if (ammo_value[i] < ammo_state)
			{
				WriteProcessMemory(processHandle, (LPVOID)hold[i], &ammo_state, sizeof(ammo_state), NULL);
			}
		}

		Sleep(1000);
	}

	
    return 0;
}
DWORD GetPidFromSnapShot(const WCHAR* processName)
{
	DWORD pid = 0;
	PROCESSENTRY32 process;
	process.dwSize = sizeof(PROCESSENTRY32);

	auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		std::cout << "INVALID_HANDLE_VALUE" << std::endl;
		return pid;
	}
	else
	{
		BOOL processSnapshotCopyState = Process32First(hSnapshot,&process);
		if (!processSnapshotCopyState)
		{
			std::cout << "Error when trying to copy process from list" << GetLastError()<< std::endl;
			return pid;
		}
		else
		{
			if (!_wcsicmp(processName, process.szExeFile))
			{
				pid = process.th32ProcessID;
			}
			else
			{
				while (Process32Next(hSnapshot, &process))
				{
					if (!_wcsicmp(processName, process.szExeFile))
					{
						pid = process.th32ProcessID;
						break;
					}
				}
			}
		}
	}
	return pid;
}
DWORD ModuleBaseAddy(DWORD processID, const WCHAR* moduleName)
{
	DWORD baseAddy = 0;
	MODULEENTRY32 modEntry;
	modEntry.dwSize = sizeof(MODULEENTRY32);
	auto snapshot = CreateToolhelp32Snapshot(0x00000018, processID); //0x00000010 == TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		std::cout << "invalid handle value value" << GetLastError() << std::endl;
		baseAddy;
	}
	else
	{
		//We will now enumerate over the modules we obtained from the snapshot and add em to our lpme pointer structure
		// returns true if the module entry has been copied to the buffer false if otherwise 

		BOOL modCopyState = Module32First(snapshot, &modEntry);
		if (!modCopyState)
		{
			std::cout << "Failed to copy modules from the snapshot handle to our MODULEENTRY32 structure " << std::endl;
			return baseAddy;
		}
		else
		{
			if (!_wcsicmp(moduleName,modEntry.szModule)) //if the name of our module and the module in the snapshot are equal when wcsimp returns 0
			{
				baseAddy = (DWORD)modEntry.modBaseAddr;
			}
			else //module names are not equal we need to enumare the other modules with Module32Next function
			{
				while (Module32Next(snapshot, &modEntry))
				{
					if (!_wcsicmp(moduleName, modEntry.szModule)) //if the name of our module and the module in the snapshot are equal when wcsimp returns 0
					{
						baseAddy = (DWORD)modEntry.modBaseAddr;
						break;
					}
				}
			}
		}

	}
	return baseAddy;
}

std::vector<DWORD> findAddy(HANDLE hProc, DWORD addy, std::vector<DWORD> baseAddyOffset, std::vector<std::vector<DWORD>> ammoBaseOffset)
{
	std::vector<DWORD> fin_addy(ammoBaseOffset.size());
	//DWORD wep_base = addy+ baseAddyOffset;
	//fin_addy = {wep_base};
	//int size = sizeof(offset) / sizeof(DWORD);
	for (int i =0; i < ammoBaseOffset.size(); i++)
	{
		fin_addy[i]=addy+baseAddyOffset[i];
		for (int j = 0; j < ammoBaseOffset[i].size(); j++)//sizeof(offset)/sizeof(*offset); i++)
		{
			ReadProcessMemory(hProc, (LPCVOID)fin_addy[i], &fin_addy[i], sizeof(addy), NULL);
			fin_addy[i] += ammoBaseOffset[i][j];
		}
	}

	return fin_addy;
}