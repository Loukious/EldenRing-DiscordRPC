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

	static std::string _GetModuleName(bool mainProcessModule)
	{
		HMODULE module = NULL;

		if (!mainProcessModule)
		{
			static char dummyStaticVarToGetModuleHandle = 'x';
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, &dummyStaticVarToGetModuleHandle, &module);
		}

		char lpFilename[MAX_PATH];
		GetModuleFileNameA(module, lpFilename, sizeof(lpFilename));
		std::string moduleName = strrchr(lpFilename, '\\');
		moduleName = moduleName.substr(1, moduleName.length());

		if (!mainProcessModule)
		{
			moduleName.erase(moduleName.find(".dll"), moduleName.length());
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
		return std::string("mods\\" + GetCurrentModName());
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
