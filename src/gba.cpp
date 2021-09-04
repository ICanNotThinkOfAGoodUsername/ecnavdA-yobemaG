
#include "gba.hpp"

#define toPage(x) (x >> 15)

GameBoyAdvance::GameBoyAdvance() : cpu(*this), ppu(*this) {
	step = false;
	trace = true;

	for (int i = toPage(0x6000000); i < toPage(0x6018000); i++) {
		pageTableRead[i] = pageTableWrite[i] = &ppu.vram[(i - toPage(0x6000000)) << 15];
	}

	reset();
}

GameBoyAdvance::~GameBoyAdvance() {
	save();
}

void GameBoyAdvance::reset() {
	running = false;
	log.str("");

	cpu.reset();
	ppu.reset();

	if (step)
		running = true;
}

void GameBoyAdvance::run() {
	while (1) {
		if (running) {
			if (trace && (cpu.pipelineStage == 3)) {
				std::string disasm = cpu.disassemble(cpu.reg.R[15] - 8, cpu.pipelineOpcode3);
				std::string logLine = fmt::format("0x{:0>8X} | 0x{:0>8X} | {}\n", cpu.reg.R[15] - 8, cpu.pipelineOpcode3, disasm);
				if (logLine.compare(previousLogLine)) {
					log << logLine;
					previousLogLine = logLine;
				}
			}
			cpu.cycle();
			ppu.cycle();

			if (step && (cpu.pipelineStage == 3))
				running = false;
		}
	}
}

int GameBoyAdvance::loadRom(std::filesystem::path romFilePath_, std::filesystem::path biosFilePath_) {
	running = false;
	std::ifstream romFileStream;
	romFileStream.open(romFilePath_, std::ios::binary);
	if (!romFileStream.is_open()) {
		printf("Failed to open ROM!\n");
		return -1;
	}
	romFileStream.seekg(0, std::ios::end);
	size_t romSize = romFileStream.tellg();
	romFileStream.seekg(0, std::ios::beg);
	romBuff.resize(romSize);
	romFileStream.read(reinterpret_cast<char *>(romBuff.data()), romSize);
	romFileStream.close();
	
	romBuff.resize(0x2000000);
	{ // Round rom size to power of 2 https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
		u32 v = romSize - 1;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		romSize = v + 1;
	}

	// Fill open bus
	for (int i = romSize; i < 0x2000000; i += 2) {
		//
	}

	// Fill page table for rom buffer
	for (int pageIndex = 0; pageIndex < 0x400; pageIndex++) {
		u8 *ptr = &romBuff[pageIndex << 15];
		pageTableRead[pageIndex | 0x1000] = ptr; // 0x0800'0000 - 0x09FF'FFFF
		pageTableRead[pageIndex | 0x1400] = ptr; // 0x0A00'0000 - 0x0BFF'FFFF
		pageTableRead[pageIndex | 0x1800] = ptr; // 0x0C00'0000 - 0x0DFF'FFFF
	}

	running = true;
	return 0;
}

void GameBoyAdvance::save() {
	/*std::ofstream saveFileStream{"vram.bin", std::ios::binary | std::ios::trunc};
	saveFileStream.write(reinterpret_cast<const char*>(ppu.vram), sizeof(ppu.vram));
	saveFileStream.close();*/
}

template <typename T>
T GameBoyAdvance::read(u32 address) {
	int page = (address & 0x0FFFFFFF) >> 15;
	int offset = address & 0x7FFF;
	void *pointer = pageTableRead[page];

	if (pointer != NULL) {
		T val;
		std::memcpy(&val, (pointer + offset), sizeof(T));
		return val;
	} else {
		switch (page) {
		case toPage(0x04000000): // I/O
			switch (offset) {
			case 0x000 ... 0x056: // PPU
				return ppu.readIO<T>(address);
			}
			break;
		}
	}

	return 0;
}
template u16 GameBoyAdvance::read<u16>(u32);
template u32 GameBoyAdvance::read<u32>(u32);

template <typename T>
void GameBoyAdvance::write(u32 address, T value) {
	int page = (address & 0x0FFFFFFF) >> 15;
	int offset = address & 0x7FFF;
	void *pointer = pageTableWrite[page];

	if (pointer != NULL) {
		std::memcpy((pointer + offset), &value, sizeof(T));
	} else {
		switch (page) {
		case toPage(0x04000000): // I/O
			switch (offset) {
			case 0x000 ... 0x056: // PPU
				ppu.writeIO<T>(address, value);
				break;
			}
			break;
		}
	}
}
template void GameBoyAdvance::write<u8>(u32, u8);
template void GameBoyAdvance::write<u16>(u32, u16);
template void GameBoyAdvance::write<u32>(u32, u32);