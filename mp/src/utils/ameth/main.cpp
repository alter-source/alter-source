#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cstdio>
#include <Windows.h>
#include <direct.h>

std::string getCurrentPath() {
	char buffer[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, buffer);
	return std::string(buffer);
}

std::string getModifiedPath(const std::string& currentPath) {
	size_t pos = currentPath.find_last_of("\\/");
	if (pos != std::string::npos) {
		std::string parentPath = currentPath.substr(0, pos);
		pos = parentPath.find_last_of("\\/");
		if (pos != std::string::npos) {
			parentPath = parentPath.substr(0, pos);
		}
		return parentPath + "\\altersrc\\bin";
	}
	return "";
}

void packAddon(const std::string& addonDir) {
	std::string currentPath = getCurrentPath();

	std::string newPath = getModifiedPath(currentPath);
	_chdir(newPath.c_str());

	std::ostringstream oss;
	oss << "7z.exe a -r -t7z \"" << addonDir << ".7z\" \"" << addonDir << "\"";
	std::string command = oss.str();

	int result = system(command.c_str());
	if (result != 0) {
		std::cerr << "Error packing addon directory." << std::endl;
		return;
	}

	std::rename((addonDir + ".7z").c_str(), (addonDir + ".as").c_str());

	std::cout << "Packing complete." << std::endl;
}

void unpackAddon(const std::string& addonDir) {
	std::string currentPath = getCurrentPath();

	std::string newPath = getModifiedPath(currentPath);

	_chdir(newPath.c_str());

	std::rename((addonDir + ".as").c_str(), (addonDir + ".7z").c_str());

	std::ostringstream oss;
	oss << "7z.exe x -o\"" << addonDir << "\" \"" << addonDir << ".7z\"";
	std::string command = oss.str();

	int result = system(command.c_str());
	if (result != 0) {
		std::cerr << "Error unpacking addon archive." << std::endl;
		return;
	}

	std::remove((addonDir + ".7z").c_str());

	std::cout << "Unpacking complete." << std::endl;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "Usage: ameth <command> <addon_directory>" << std::endl;
		return 1;
	}

	std::string command = argv[1];
	std::string addonDir = argv[2];

	if (command == "pack") {
		packAddon(addonDir);
	}
	else if (command == "unpack") {
		unpackAddon(addonDir);
	}
	else {
		std::cerr << "Unknown command: " << command << std::endl;
		return 1;
	}

	return 0;
}