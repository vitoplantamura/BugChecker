#include "LogWnd.h"

#include "Root.h"
#include "Utils.h"

VOID LogWnd::GoToEnd()
{
	if (!destY0 && !destY1)
	{
		GoToEnd_Pending = TRUE;
		return;
	}

	const ULONG winDim = destY1 - destY0 + 1;

	if (contents.size() >= winDim)
		posY = contents.size() - winDim;
}

VOID LogWnd::AddUserCmd(const CHAR* psz)
{
	AddString(("\r:" + eastl::string(psz)).c_str());
}

VOID LogWnd::AddString(const CHAR* psz, BOOLEAN doAppend /*= FALSE*/, BOOLEAN refreshUi /*= FALSE*/)
{
	auto prevStart0to99 = Allocator::Start0to99;
	Allocator::Start0to99 = ALLOCATOR_START_0TO99_LOG;

	auto allocStart = (LONG64)Allocator::Perf_TotalAllocatedSize;

	{
		eastl::string s = psz;

		if (!doAppend)
		{
			if (!newLine)
				while (contents.size() && !contents.back().size())
					contents.pop_back();

			if (s.size())
				contents.push_back(eastl::move(s));

			newLine = TRUE;
		}
		else
		{
			for (CHAR& c : s)
				if (c != '\n' && c != '\b' && (c < 32 || c >= 128))
					c = '?';

			auto v = Utils::Tokenize(s, "\n");

			BOOLEAN isFirst = TRUE;

			for (auto& l : v)
			{
				if (newLine)
					newLine = FALSE;
				else if (isFirst)
				{
					if (contents.size())
					{
						l = contents.back() + l;
						contents.pop_back();
					}
				}

				for (CHAR& c : l)
					if (c == '\b')
						c = '\n';
				
				contents.push_back(eastl::move(l));

				isFirst = FALSE;
			}
		}
	}

	allocSize += (LONG64)Allocator::Perf_TotalAllocatedSize - allocStart;

	while (allocSize > LOG_WINDOW_CONTENTS_MAX_SIZE && contents.size())
	{
		allocStart = (LONG64)Allocator::Perf_TotalAllocatedSize;

		{
			contents.erase(contents.begin());
		}

		allocSize += (LONG64)Allocator::Perf_TotalAllocatedSize - allocStart;
	}

	Allocator::Start0to99 = prevStart0to99;

	// go to the end and refresh ui.

	GoToEnd();

	if (refreshUi)
	{
		DrawAll_End();
		DrawAll_Final();
	}
}

VOID LogWnd::Clear()
{
	contents.clear();
}

eastl::string LogWnd::GetLineToDraw(ULONG index)
{
	eastl::string s = contents[index];

	if (s.size() && s[0] == '\r')
		s = s.substr(1);

	return s;
}

VOID LogWnd::Left()
{
	if (posX > 0) posX--;
}

VOID LogWnd::Right()
{
	posX++;
}

VOID LogWnd::Up()
{
	if (posY > 0) posY--;
}

VOID LogWnd::Down()
{
	const ULONG winDim = destY1 - destY0 + 1;

	ULONG maxPosY = 0;

	if (contents.size() > winDim)
		maxPosY = contents.size() - winDim;

	posY++;

	if (posY > maxPosY)
		posY = maxPosY;
}

VOID LogWnd::PgUp()
{
	const ULONG winDim = destY1 - destY0 + 1;

	if (posY > winDim)
		posY -= winDim;
	else
		posY = 0;
}

VOID LogWnd::PgDn()
{
	const ULONG winDim = destY1 - destY0 + 1;

	ULONG maxPosY = 0;

	if (contents.size() > winDim)
		maxPosY = contents.size() - winDim;

	posY += winDim;

	if (posY > maxPosY)
		posY = maxPosY;
}

VOID LogWnd::Home()
{
	posY = 0;
}

VOID LogWnd::End()
{
	GoToEnd();
}
