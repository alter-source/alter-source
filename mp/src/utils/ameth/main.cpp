#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cstdio>

void packAddon(const std::string& addonDir) {
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
