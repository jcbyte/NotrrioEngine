#include "ProcessScanner.h"

// TODO
// fix memort leaks
// save pages to hard drive to programs with lots of memory dont explode pc

namespace nottrio
{
	ProcessScanner::ProcessScanner(int procID)
	{
		handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
	}

	ProcessScanner::ProcessScanner(const char* procTitle)
	{
		HWND window = FindWindowA(nullptr, procTitle);
		DWORD pid;
		GetWindowThreadProcessId(window, &pid);

		handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	}

	ProcessScanner::~ProcessScanner()
	{
		CloseHandle(handle);
	}

	void ProcessScanner::BeginScan(Scantype scantype, int valueSize, unsigned char* value)
	{
		searchValueSize = valueSize;
		matches.clear(); // memory leak

		if (scantype == Scantype::EXACT_VALUE && value == nullptr)
		{
			throw "No value was provided for the scan of type EXACT_VALUE";
		}

		LoadPages();

		if (scantype == Scantype::EXACT_VALUE)
		{
			SingleValueInitialScan(value);
		}
		else if (scantype == Scantype::UNKNOWN_INITIAL_VALUE)
		{
			CopyScanArea();
		}

		previousScan = scantype;
	}

	void ProcessScanner::NextScan(Scantype scantype, unsigned char* value, bool (*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2))
	{
		if (scantype != Scantype::EXACT_VALUE && ComparitiveFunction == nullptr
			||
			IsChangingValueScan(scantype) && ComparitiveFunction == nullptr
			)
		{
			throw "comparative function not given for comparative scan";
		}

		if (scantype == Scantype::EXACT_VALUE)
		{
			SingleValueUpdateScan(value);
		}
		else if (scantype == Scantype::UNKNOWN_INITIAL_VALUE)
		{
			throw "UKNOWN INITIAL VALUE cannot be used for next scan";
		}
		else if (previousScan == Scantype::UNKNOWN_INITIAL_VALUE && IsChangingValueScan(scantype))
		{
			CompareValueInitialScan(ComparitiveFunction);
		}
		else if (IsChangingValueScan(scantype)) // shouldnt need condition here but kept just in case
		{
			UpdateMatches();
			CompareUpdatedMatches(ComparitiveFunction);
		}

		previousScan = scantype;
	}

	void ProcessScanner::SingleValueInitialScan(unsigned char* searchValue)
	{
		if (searchValue == nullptr) return;

		unsigned char* readBuffer = new unsigned char[readBufferSize];
		SIZE_T read;

		for (int i = 0; i < pages.size(); i++)
		{
			for (int j = 0; j < pages[i].size; j += readBufferSize)
			{
				ReadProcessMemory(handle, pages[i].addr + j, readBuffer, readBufferSize, &read);

				for (int k = 0; k < readBufferSize; k += searchValueSize)
				{
					if (std::memcmp(searchValue, readBuffer + k, searchValueSize) == 0)
					{
						Match match;
						match.addr = pages[i].addr + j + k;
						match.currentValue = new unsigned char[searchValueSize];
						match.previousValue = new unsigned char[searchValueSize];
						std::memcpy(match.currentValue, readBuffer + k, searchValueSize);
						std::memcpy(match.previousValue, readBuffer + k, searchValueSize);

						matches.push_back(match);
					}
				}
			}
		}

		delete[] readBuffer;
	}

	void ProcessScanner::SingleValueUpdateScan(unsigned char* newValue)
	{
		if (newValue == nullptr) return;

		std::vector<Match> stillMatches;

		unsigned char* readBuffer = new unsigned char[searchValueSize];
		for (int i = 0; i < matches.size(); i++)
		{
			SIZE_T size;
			ReadProcessMemory(handle, matches[i].addr, readBuffer, searchValueSize, &size);

			if (std::memcmp(readBuffer, newValue, searchValueSize) == 0)
			{
				stillMatches.push_back(matches[i]);
				std::memcpy(matches[i].currentValue, newValue, searchValueSize);
				std::memcpy(matches[i].previousValue, newValue, searchValueSize);
			}
		}

		delete[] readBuffer;
		matches = stillMatches;

	}

	void ProcessScanner::WriteToAllMatches(unsigned char* value)
	{
		SIZE_T size;

		for (int i = 0; i < matches.size(); i++)
		{
			if (!WriteProcessMemory(handle, matches[i].addr, value, searchValueSize, &size))
			{
				std::cout << "write process memory error: " << GetLastError() << ", addr: 0x" << std::hex << (int)matches[i].addr << std::dec << std::endl;
			}
		}
	}

	void ProcessScanner::CopyScanArea()
	{
		if (pages.size() == 0) return;

		SIZE_T size;
		for (int i = 0; i < pages.size(); i++)
		{
			pages[i].copy = new unsigned char[pages[i].size];
			if (!ReadProcessMemory(handle, pages[i].addr, pages[i].copy, pages[i].size, &size))
			{
				std::cout << std::hex << "error reading between 0x" << (int)pages[i].addr << " and 0x" << (int)(pages[i].size + pages[i].addr) << std::dec << std::endl; // the two errors are pages with memInfo.protect = 260
			}
		}
	}

	unsigned char* ProcessScanner::ReadValueInCopy(unsigned char* addr)
	{
		unsigned char* searchPtr = pages[0].addr;

		int selectedPage = 0;
		for (int i = 0; i < pages.size(); i++) // will cause crash if addr is beyond final page idk should deal with at some point
		{
			if (pages[i + 1].addr > addr)
			{
				selectedPage = i;
				break;
			}
		}

		int dif = addr - pages[selectedPage].addr;
		return pages[selectedPage].copy + dif;
	}

	void ProcessScanner::CompareValueInitialScan(bool (*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2))
	{
		unsigned char* readBuffer = new unsigned char[searchValueSize];

		for (int i = 0; i < pages.size(); i++)
		{
			for (unsigned char* readAddr = pages[i].addr; readAddr < pages[i].addr + pages[i].size; readAddr += searchValueSize)
			{
				SIZE_T size;
				if (!ReadProcessMemory(handle, readAddr, readBuffer, searchValueSize, &size))
				{
					throw "Could not read memory in preveously read page";
				}

				unsigned char* copyAddr = pages[i].copy + (readAddr - pages[i].addr);

				if (ComparitiveFunction(readBuffer, copyAddr))
				{
					Match match;
					match.addr = readAddr;

					match.previousValue = new unsigned char[searchValueSize];
					std::memcpy(match.previousValue, copyAddr, searchValueSize);

					match.currentValue = new unsigned char[searchValueSize];
					std::memcpy(match.currentValue, readBuffer, searchValueSize);

					matches.push_back(match);
				}
			}
		}

		// delte paeg buffers <------------------------------------------------------------- DO AT SOME POINT
	}

	void ProcessScanner::CompareUpdatedMatches(bool(*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2))
	{
		std::vector<Match> newMatches;

		for (int i = 0; i < matches.size(); i++)
		{
			if (ComparitiveFunction(matches[i].currentValue, matches[i].previousValue))
			{
				newMatches.push_back(matches[i]);
			}
		}

		matches = newMatches;
	}

	void ProcessScanner::UpdateMatches()
	{
		unsigned char* readBuffer = new unsigned char[searchValueSize];
		std::vector<Match> newMatches;

		for (int i = 0; i < matches.size(); i++)
		{
			SIZE_T size;
			ReadProcessMemory(handle, matches[i].addr, readBuffer, searchValueSize, &size);

			//delete[] matches[i].currentValue; MEMEORY LEAK <-------------------------------------------------------------------------------------------------
			std::memcpy(matches[i].currentValue, readBuffer, searchValueSize);
		}

		delete[] readBuffer;
	}

	/*
	void ProcessScanner::CompareValueUpdateMatchesScan(bool (*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2)) Old way of doing it
	{
		unsigned char* readBuffer = new unsigned char[searchValueSize];

		std::vector<Match> newMatches;

		for (int i = 0; i < matches.size(); i++)
		{
			SIZE_T size;
			ReadProcessMemory(handle, matches[i].addr, readBuffer, searchValueSize, &size);

			int readInt = *(int*)readBuffer;
			int matchInt = *(int*)matches[i].buffer;

			if (ComparitiveFunction(readBuffer, matches[i].buffer))
			{
				newValueMatches.push_back(matches[i]);
				matches.push_back(matches[i].addr);
			}
		}

		valueMatches = newValueMatches;
	}*/

	void ProcessScanner::LoadPages()
	{
		unsigned char* addr = 0;
		MEMORY_BASIC_INFORMATION memInfo;

		pages.clear();

		while (VirtualQueryEx(handle, addr, &memInfo, sizeof(memInfo)))
		{
			Page page;
			page.addr = addr;
			page.size = memInfo.RegionSize;

			if ((memInfo.State == MEM_COMMIT)
				&&
				(memInfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
				&& memInfo.Protect != 260) // HACK
			{
				pages.push_back(page);
			}

			addr += memInfo.RegionSize;
		}
	}
}