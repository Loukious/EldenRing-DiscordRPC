#include <string>
#include <minwindef.h>
#include <libloaderapi.h>
#include <fileapi.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace Utils
{
	static std::ofstream muLogFile;

	static std::string _GetModulePath(bool mainProcessModule)
	{
		HMODULE module = NULL;

		if (!mainProcessModule)
		{
			static char dummyStaticVarToGetModuleHandle = 'x';
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, &dummyStaticVarToGetModuleHandle, &module);
		}

		char lpFilename[MAX_PATH]; // undefined
		GetModuleFileNameA(module, lpFilename, MAX_PATH); // absolute\\path\\to\\mod.dll
		std::string modulePath = lpFilename; // "absolute\\path\\to\\mod.dll" (as a string instead of char arr[])

		return modulePath; // Pulling the raw data and processing it has been separated.
	}


	static std::string _GetModuleName(bool mainProcessModule)
	{
		std::string moduleName;

		//modded;
		//First half of the old _GetModuleName() moved to _GetModulePath().
		moduleName = _GetModulePath(mainProcessModule); // "absolute\\path\\to\\mod.dll" if mainProcessModule = false. No idea if = true.

		//Second half unchanged
		moduleName = strrchr(moduleName.c_str(), '\\'); // "mod.dll" (again if mainProcessModule = false, no idea if = true.)
		moduleName = moduleName.substr(1, moduleName.length());

		if (!mainProcessModule)
		{
			moduleName.erase(moduleName.find(".dll"), moduleName.length()); // "mod"
		}

		return moduleName;
	}

	static std::string GetCurrentProcessName()
	{
		return _GetModuleName(true);
	}

	static std::string GetCurrentModName()
	{
		static std::string currentModName = "NULL";
		if (currentModName == "NULL")
		{
			currentModName = _GetModuleName(false);
		}
		return currentModName;
	}


	static std::string GetModFolderPath()
	{
		static std::string modFolderPath = "NULL"; // "NULL" (i.e. runs the below if statement on first call)
		if (modFolderPath == "NULL")
		{
			modFolderPath = _GetModulePath(false); // "absolute\\path\\to\\mod.dll"
			modFolderPath.erase(modFolderPath.find(".dll"), modFolderPath.length()); // "absolute\\path\\to\\mod"
		}

		return modFolderPath;
	}

	static void OpenModLogFile()
	{
		if (!muLogFile.is_open())
		{
			CreateDirectoryA(std::string("mods\\" + GetCurrentModName()).c_str(), NULL);
			muLogFile.open("mods\\" + GetCurrentModName() + "\\log.txt");
		}
	}

	template<typename... Types>
	static void Log(Types... args)
	{
		OpenModLogFile();

		std::stringstream stream;
		stream << GetCurrentModName() << " > ";
		(stream << ... << args) << std::endl;
		std::cout << stream.str();

		if (muLogFile.is_open())
		{
			muLogFile << stream.str();
			muLogFile.flush();
		}
	}

	static void CloseLog()
	{
		if (muLogFile.is_open())
		{
			muLogFile.close();
		}
	}
}
