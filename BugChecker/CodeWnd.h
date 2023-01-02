#pragma once

#include "BugChecker.h"

#include "Wnd.h"
#include "BcCoroutine.h"

class CodeWnd : public Wnd
{
public:

	CodeWnd();

	virtual eastl::string GetLineToDraw(ULONG index);

	VOID Add(const eastl::string& str);
	VOID Enter();
	VOID BackSpace();
	VOID Left();
	VOID Right();
	VOID Up();
	VOID Down();
	VOID Del();
	VOID Home();
	VOID End();
	VOID Tab();
	VOID PgUp();
	VOID PgDn();

	BcCoroutine Eval(BYTE* context, ULONG contextLen, BOOLEAN is32bitCompat) noexcept;

private:

	eastl::string& GetLine(ULONG y, BOOLEAN add = FALSE, eastl::string* deletedStr = NULL);
	ULONG GetCurrentLineId();
	eastl::string& GetCurrentLine();

	VOID SyntaxColor(ULONG y);

	VOID Finalize();
};
