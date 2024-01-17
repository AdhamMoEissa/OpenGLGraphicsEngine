#ifndef DEBUG_H
#define DEBUG_H
#define DEBUGGING 1

#include <iostream>
#include <fstream>

namespace debug {
	enum DebugType{
		Information = 0,
		Warning = 1,
		Error = 2
	};
	enum LoggingLocation {
		ToConsole = 0,
		ToFile = 1,
		ToNone = 2
	};

	std::string* loggingFilePath = nullptr;
	std::fstream* loggingFile;
	LoggingLocation loggingLocation;

	bool setLoggingFilePath(const char* filePath = '\0')
	{
		if(loggingFilePath == nullptr)
		{
			loggingFilePath = new std::string(filePath);
			return 0;
		}
		else if(loggingFilePath->c_str() == (char*)'\0')
		{
			*loggingFilePath = filePath;
			return 0;
		}
		else
			return 1;
	}
	bool setLoggingFilePath(std::string filePath = '\0')
	{
		if(loggingFilePath == nullptr)
		{
			loggingFilePath = new std::string(filePath);
			return 0;
		}
		else if(loggingFilePath->c_str() == (char*)'\0')
		{
			*loggingFilePath = filePath;
			return 0;
		}
		else
			return 1;
	}

	//Sets up the logging file
	//returns 1 if an error occurred and returns 0 otherwise
	bool setUpLoggingFile()
	{
		if(loggingFilePath == nullptr || loggingFilePath->c_str() == (char*)'\0')
			return 1;
		loggingFile = new std::fstream(*loggingFilePath, std::ios::out | std::ios::trunc);
		if(!*loggingFile)
			return 1;



		return 0;
	}

	//logs message to preset logging location
	void log(const char* message, DebugType type = Information)
	{
		if(DEBUGGING)
		{
			if(setLoggingFilePath(nullptr))
				loggingLocation = ToFile;
			else
				loggingLocation = ToConsole;
		}
		else
		{
			loggingLocation = ToNone;
		}

		if(loggingLocation == ToNone)
			return;
		if(loggingLocation == ToConsole)
		{
			if(type == Information)
				std::cout << "Info : ";
			else if(type == Warning)
				std::cout << "Warning : ";
			else if(type == Error)
				std::cout << "Error : ";
			std::cout << message << std::endl;
			return;
		}
		if(loggingLocation == ToFile)
		{

		}
	}

	//logs message to preset logging location	
	void log(std::string message, DebugType type = Information)
	{
		if(DEBUGGING)
		{
			if(setLoggingFilePath(nullptr))
				loggingLocation = ToFile;
			else
				loggingLocation = ToConsole;
		}
		else
		{
			loggingLocation = ToNone;
		}
		if(loggingLocation == ToNone)
			return;
		if(loggingLocation == ToConsole)
		{
			if(type == Information)
				std::cout << "Info : ";
			else if(type == Warning)
				std::cout << "Warning : ";
			else if(type == Error)
				std::cout << "Error : ";
			std::cout << message << std::endl;
			return;
		}
		if(loggingLocation == ToFile)
		{

		}
	}

	//shows or hides depending on the input boolean
	void showConsoleWindow(bool input = DEBUGGING)
	{
		if(input)
		{
			::ShowWindow(::GetConsoleWindow(), SW_SHOW);
		}
		else
		{
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
		}
	}
}

#endif