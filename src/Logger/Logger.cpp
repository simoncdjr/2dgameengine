#include "Logger.h"
#include <iostream>
#include <ctime>

std::vector<LogEntry> Logger::messages;
std::string Logger::ReturnTimeDate() {
	std::time_t now = std::time(nullptr);
	char output[80];

	//dd-mm-yyyy Time
	strftime(output, sizeof(output), "%d-%b-%Y %T", std::localtime(&now));

	return output;
}

void Logger::Log(const std::string& message) {
	LogEntry logEntry;
	logEntry.type = LOG_INFO;
	logEntry.message = ReturnTimeDate() + " | " + message;

	std::cout << "\e[32m" << logEntry.message << "\e[0m" << std::endl;

	messages.push_back(logEntry);
}

void Logger::Err(const std::string& message) {
	LogEntry logEntry;
	logEntry.type = LOG_ERROR;
	logEntry.message = ReturnTimeDate() + " | " + message;
	std::cerr << "\e[31m" << logEntry.message << "\e[0m" << std::endl;

	messages.push_back(logEntry);
}
