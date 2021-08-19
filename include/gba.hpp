
#ifndef GBA_H
#define GBA_H

#include <cstring>
#include <iostream>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "typedefs.hpp"
#include "arm7tdmi.hpp"

namespace GBA {
	void reset();
	void run();

	int loadRom(std::filesystem::path romFilePath_, std::filesystem::path biosFilePath_);
	void save();

	bool running;
};

#endif