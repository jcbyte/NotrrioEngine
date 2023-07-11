#pragma once

#include <Windows.h>
#include <vector>
#include <iostream>
#include <functional>

namespace nottrio
{

	template<typename T>
	const auto GreaterThan = [](unsigned char* buffer1, unsigned char* buffer2) -> bool
	{
		T* t1 = (T*)buffer1; int32_t* t2 = (int32_t*)buffer2;
		return *t1 > *t2;
	}

		template<typename T>
	const auto LessThan = [](unsigned char* buffer1, unsigned char* buffer2) -> bool
	{
		T* t1 = (T*)buffer1; int32_t* t2 = (int32_t*)buffer2;
		return *t1 < *t2;
	}

		const auto NotEqual = [](unsigned char* buffer1, unsigned char* buffer2) -> bool
	{
		return std::memcmp(buffer1, buffer2, sizeof(int32_t)) != 0;
	};

	struct Page
	{
		unsigned char* addr;
		DWORD size;

		unsigned char* copy;
	};

	struct Match
	{
		unsigned char* addr;
		unsigned char* currentValue;
		unsigned char* previousValue;
	};

	enum class Scantype
	{
		EXACT_VALUE, GREATER_THAN, LESS_THAN, CHANGED_VALUE, UNKNOWN_INITIAL_VALUE
	};

	inline bool IsChangingValueScan(Scantype type)
	{
		return (type == Scantype::GREATER_THAN || type == Scantype::LESS_THAN || type == Scantype::CHANGED_VALUE);
	}

	class ProcessScanner
	{
	public:
		ProcessScanner(int procID);//, int searchValueSize); // pass a handle?
		ProcessScanner(const char* procTitle);//, int searchValueSize);

		~ProcessScanner();

		void BeginScan(Scantype scantype, int searchValueSize, unsigned char* value); // value size needs to be passed in here
		void NextScan(Scantype scantype, unsigned char* value, bool (*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2));
		void UpdateMatches();

		void WriteToAllMatches(unsigned char* value);

	private:
		void LoadPages();
		void CopyScanArea();

		void SingleValueInitialScan(unsigned char* searchValue);
		void SingleValueUpdateScan(unsigned char* newValue);



		unsigned char* ReadValueInCopy(unsigned char* addr);
		void CompareValueInitialScan(bool (*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2));
		void CompareUpdatedMatches(bool (*ComparitiveFunction)(unsigned char* buffer1, unsigned char* buffer2));

		int GetSizeOfScanArea()
		{
			int sum = 0;
			for (int i = 0; i < pages.size(); i++)
			{
				sum += pages[i].size;
			}
			return sum;
		}
		int GetNumOfPages() { return pages.size(); }

	private:
		int searchValueSize; // utterly devious hack

		HANDLE handle;
		std::vector<Page> pages;

		Scantype previousScan;
		const int readBufferSize = 2048;

		std::vector<Match> matches;
	};
}