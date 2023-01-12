#include "Main.h"

#include "Root.h"
#include "Platform.h"
#include "Ps2Keyb.h"
#include "Glyph.h"
#include "Utils.h"
#include "QuickJSCppInterface.h"

#ifdef _AMD64_
extern "C" VOID Amd64_sgdt(BYTE * dest);
#endif

//

extern "C" VOID BreakInAndDeleteBreakPoints();

BcCoroutine Main::EntryPoint() noexcept
{
	// reset the trace and bp vars.

	Root::I->BpHitIndex = -1;

	// make sure that the PsLoadedModuleList pointer is not null.

	if (!Root::I->PsLoadedModuleList)
	{
		co_await BcAwaiter_StateManipulate{
		[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			pState->ApiNumber = DbgKdGetVersionApi;
		},
		[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			if (pState->ApiNumber == DbgKdGetVersionApi)
			{
				if (NT_SUCCESS(pState->ReturnStatus))
				{
					Root::I->PsLoadedModuleList = pState->u.GetVersion64.PsLoadedModuleList;
					Platform::KernelBaseAddress = Platform::GetKernelBaseAddress(Root::I->PsLoadedModuleList);
					return TRUE;
				}
			}

			return FALSE;
		} };
	}

	// get the context i.e. all the processor registers.

	BYTE Context[4096]; // sizeof(CONTEXT) is 1232 on Windows 10 20H2 64 bit.
	ULONG ContextLen = 0;

	co_await BcAwaiter_StateManipulate{
	[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

		pState->ApiNumber = DbgKdGetContextApi;
	},
	[&Context, &ContextLen](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

		if (pState->ApiNumber == DbgKdGetContextApi)
		{
			if (NT_SUCCESS(pState->ReturnStatus) &&
				SecondBuffer->pData != NULL && SecondBuffer->Length > 0)
			{
				::memcpy(Context, SecondBuffer->pData, SecondBuffer->Length);
				ContextLen = SecondBuffer->Length;
				return TRUE;
			}
		}

		return FALSE;
	} };

	// are we in 32 bit compatibility mode (i.e. in a WoW64 process)?

	BOOLEAN is32bitCompat = FALSE;

#ifdef _AMD64_
	{
		auto cs = ((CONTEXT*)Context)->SegCs;

		auto rpl = cs & 3;
		auto tl = (cs >> 2) & 1;
		auto sel = cs >> 3;

		if (rpl == 3 && tl == 0)
		{
			BYTE gdtr[10];
			Amd64_sgdt(gdtr);

			auto pGdt = (ULONG64*)*(ULONG64*)(gdtr + 2);

			auto desc = pGdt[sel];

			auto flagsAndLimit = (BYTE)((desc >> 48) & 0xFF);

			is32bitCompat = (flagsAndLimit & 0b100000) == 0; // L flag
		}
	}
#endif

	// check if we need to break into the debugger.

	BOOLEAN breakIn = FALSE;
	BOOLEAN updateContext = FALSE;
	BOOLEAN deleteAllBreakPoints = FALSE;
	BOOLEAN regsWndInvOld = FALSE;

	if (Root::I->StateChange.NewState == DbgKdExceptionStateChange)
	{
		if ((Root::I->StateChange.u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT || _6432_(Root::I->StateChange.u.Exception.ExceptionRecord.ExceptionCode == STATUS_WX86_BREAKPOINT, FALSE)) &&
			Root::I->StateChange.u.Exception.ExceptionRecord.NumberParameters >= 1 &&
			Root::I->StateChange.u.Exception.ExceptionRecord.ExceptionInformation[0] == BREAKPOINT_BREAK)
		{
			// is this our hardcoded INT3 ?

			if (((CONTEXT*)Context)->_6432_(Rip, Eip) == (ULONG_PTR) & ::BreakInAndDeleteBreakPoints)
			{
				deleteAllBreakPoints = TRUE;
			}

			// skip hardcoded INT3s, incrementing RIP/EIP.

			auto& cr = Root::I->StateChange.AnyControlReport._6432_(Amd64ControlReport, X86ControlReport);

			if (cr.InstructionCount >= 1 &&
				cr.InstructionStream[0] == 0xCC)
			{
				((CONTEXT*)Context)->_6432_(Rip, Eip)++;
				updateContext = TRUE;
				if (!deleteAllBreakPoints)
				{
					breakIn = TRUE;
					regsWndInvOld = TRUE; // don't show diffs in regs window.
				}
			}
		}
		else if (Root::I->StateChange.u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP || _6432_(Root::I->StateChange.u.Exception.ExceptionRecord.ExceptionCode == STATUS_WX86_SINGLE_STEP, FALSE))
		{
			ULONG64 pc = ((CONTEXT*)Context)->_6432_(Rip, Eip);
			
			if (BpTrace::Get().nextAddr == pc)
			{
				BpTrace::Get().nextAddr = 0;
			}
			else
			{
				breakIn = !Root::I->StepOutThread;
			}
		}
	}

	// check whether we received a request to delete all the breakpoints through an INT3.

	if (deleteAllBreakPoints)
	{
		for (BreakPoint& bp : Root::I->BreakPoints)
		{
			if (bp.handle)
				co_await BcAwaiter_StateManipulate{
				[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					pState->ApiNumber = DbgKdRestoreBreakPointApi;

					pState->ReturnStatus = STATUS_PENDING;

					pState->u.RestoreBreakPoint.BreakPointHandle = bp.handle;
				},
				[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					return pState->ApiNumber == DbgKdRestoreBreakPointApi && NT_SUCCESS(pState->ReturnStatus);
				} };
		}

		Root::I->BreakPoints.clear();
	}

	// disassemble the current instruction at PC, getting the PC of the next instruction.

	ZydisDecodedInstruction thisInstr = {};
	ZydisDecodedOperand thisOps[ZYDIS_MAX_OPERAND_COUNT_VISIBLE] = {};

	JumpInstrType jumpType = JumpInstrType::None;
	ULONG64 stepAddr = 0;

	co_await BcAwaiter_Join{ DisasmCurrInstr(thisInstr, thisOps, jumpType, stepAddr, Context, is32bitCompat) };

	// was a BugChecker breakpoint the cause of the state change ?

	eastl::vector<BreakPoint*> recreateList;
	BOOLEAN pcBpProcessed = FALSE;

	ULONG64 prevBpTraceAddr = BpTrace::Get().addr;
	BpTrace::Get().addr = 0;

	ULONG64 currPc = ((CONTEXT*)Context)->_6432_(Rip, Eip);

	for (BreakPoint& bp : Root::I->BreakPoints)
	{
		if (bp.address == prevBpTraceAddr || (bp.address > currPc - 64 && bp.address < currPc + 64))
		{
			if (bp.address == currPc)
			{
				bp.handle = 0; // bps at PC are deleted by KD.

				if (!pcBpProcessed) // the step bp and a normal bp can have the same address: in this case, we process only the step bp.
				{
					pcBpProcessed = TRUE;

					// we need to single step, then we can recreate the bp at PC.
					
					Root::I->Trace = TRUE;
					BpTrace::Get().addr = currPc;
					BpTrace::Get().nextAddr = stepAddr;

					// check the conditions of the BP.

					if (!breakIn)
					{
						breakIn = TRUE;

						if (bp.skip)
						{
							breakIn = FALSE;
						}
						else if (bp.ethread)
						{
							if (bp.ethread != Root::I->StateChange.Thread)
								breakIn = FALSE;
						}
						else if (bp.eprocess)
						{
							ULONG64 eprocess = 0;
							co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(eprocess) };

							if (bp.eprocess != eprocess)
								breakIn = FALSE;
						}

						if (breakIn && bp.when.size())
						{
							NullableU64 nu64;

							co_await BcAwaiter_Join{ QuickJSCppInterface::Eval(bp.when.c_str(), Context, ContextLen, is32bitCompat, &nu64, TRUE) };

							if (!nu64.isNull)
							{
								if (!nu64.value)
									breakIn = FALSE;
							}
							else
							{
								Root::I->LogWindow.AddString("BPX WHEN expression: unable to convert javascript result to ULONG64.");
							}

							// very special case: the user script modified EIP/RIP!

							if (currPc != ((CONTEXT*)Context)->_6432_(Rip, Eip))
							{
								co_await BcAwaiter_Join{ DisasmCurrInstr(thisInstr, thisOps, jumpType, stepAddr, Context, is32bitCompat) };

								// prevent BC from breaking in at the next INT1 if it happens at "stepAddr". Don't update "BpTrace::Get().addr" since the old value is required to recreate this breakpoint.

								BpTrace::Get().nextAddr = stepAddr;
							}
						}

						if (breakIn && bp.isStepBp && Root::I->StepOutThread) // special case when we are in step out mode: don't break in!
							breakIn = FALSE;

						if (breakIn && !bp.isStepBp)
						{
							Root::I->BpHitIndex = &bp - &Root::I->BreakPoints[0];

							regsWndInvOld = TRUE; // don't show old/new values differences in registers window.

							Root::I->LogWindow.AddString(("Break due to " + bp.cmd).c_str());
						}
					}
				}
			}
			else
			{
				recreateList.push_back(&bp);
			}
		}
	}

	// recreate the breakpoints, in reverse order, leaving the step bp (if present) as last.

	for (int i = recreateList.size() - 1; i >= 0; i--)
	{
		BreakPoint& bp = *recreateList[i];

		co_await BcAwaiter_StateManipulate{
		[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			pState->ApiNumber = DbgKdWriteBreakPointApi;

			pState->ReturnStatus = STATUS_PENDING;

			pState->u.WriteBreakPoint.BreakPointHandle = 0;
			pState->u.WriteBreakPoint.BreakPointAddress = bp.address;
		},
		[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			if (pState->ApiNumber == DbgKdWriteBreakPointApi)
			{
				if (NT_SUCCESS(pState->ReturnStatus) &&
					pState->u.WriteBreakPoint.BreakPointHandle)
				{
					bp.handle = pState->u.WriteBreakPoint.BreakPointHandle;

					return TRUE;
				}
			}

			return FALSE;
		} };
	}

	// if we changed something, update the processor registers.

	if (updateContext)
	{
		co_await BcAwaiter_StateManipulate{
		[&Context, &ContextLen](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			if (SecondBuffer->pData != NULL && ContextLen > 0 && SecondBuffer->MaxLength >= ContextLen)
			{
				pState->ApiNumber = DbgKdSetContextApi;

				::memcpy(SecondBuffer->pData, Context, ContextLen);
				SecondBuffer->Length = ContextLen;
			}
		},
		[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			return pState->ApiNumber == DbgKdSetContextApi && NT_SUCCESS(pState->ReturnStatus);
		} };
	}

	// manage the step out mode case.

	if (!breakIn && Root::I->StepOutThread)
	{
		if (Root::I->StepOutThread == Root::I->StateChange.Thread)
		{
			co_await BcAwaiter_Join{ Cmd::StepOver(Context, &thisInstr) };

			if (Cmd::IsRet(thisInstr.mnemonic))
				Root::I->StepOutThread = 0;
		}
	}

	// if a breakin is not requested, return from the coroutine, i.e. trigger a DbgKdContinueApi state manipulate request.

	if (!breakIn)
	{
		co_return;
	}

	// we can remove the step bp.

	co_await BcAwaiter_Join{ SetStepBp(0) };

	// reset the step out mode flag.

	Root::I->StepOutThread = 0;

	// invalidate the old registers in the registers window, if requested.

	if (regsWndInvOld)
	{
		Root::I->RegsWindow.InvalidateOldContext();
	}

	// get image file name and irql.

	co_await BcAwaiter_Join{ Platform::GetCurrentImageFileName(Root::I->CurrentImageFileName) };

	co_await BcAwaiter_Join{ Platform::GetIrql(Root::I->CurrentIrql) };

	// draw the interface.

	ULONG64 jumpDest = jumpType == JumpInstrType::None ? 0 : jumpType == JumpInstrType::NotJumping ? -1 : stepAddr;

	co_await BcAwaiter_Join{ DrawInterface(Context, is32bitCompat, jumpDest) };

	// read from keyboard.

	BOOLEAN shiftState = FALSE;
	BOOLEAN ctrlState = FALSE;
	BOOLEAN altState = FALSE;

	while (TRUE)
	{
		// make the cursor blink.

		if (Root::I->CursorX >= 0 && Root::I->CursorY >= 0)
		{
			BOOLEAN state = (__rdtsc() / Root::I->CursorBlinkTime) % 2 == 0;

			BOOLEAN current = Root::I->CursorDisplayX >= 0 && Root::I->CursorDisplayY >= 0;

			if (state != current)
			{
				if (state)
				{
					Root::I->CursorDisplayX = Root::I->CursorX;
					Root::I->CursorDisplayY = Root::I->CursorY;
				}
				else
				{
					Root::I->CursorDisplayX = -1;
					Root::I->CursorDisplayY = -1;
				}

				Root::I->FrontBuffer[Root::I->CursorY * Root::I->WndWidth + Root::I->CursorX] = 0;
				Wnd::DrawAll_Final();
			}
		}

		// read from the keyboard.

		USHORT b = Ps2Keyb::ReadAscii(shiftState, ctrlState, altState);
		BYTE scancode = b >> 8;
		BYTE chr = b & 0xFF;

		if (chr >= 32 && chr < 128)
		{
			if (Root::I->CursorFocus == CursorFocus::Log)
				Root::I->InputLine.AddChar((CHAR)chr);
			else if (Root::I->CursorFocus == CursorFocus::Code)
				Root::I->CodeWindow.Add(eastl::string(1, (CHAR)chr));
		}
		else
		{
			BOOLEAN exit = FALSE;

			switch (scancode)
			{
			case MACRO_SCANCODE_ESC:
			{
				Wnd::SwitchBetweenLogCode();

				Wnd::DrawAll_End(); // in order to update the help text.
				Wnd::DrawAll_Final();
			}
			break;

			case MACRO_SCANCODE_F1:
			{
				co_await BcAwaiter_Join{ Root::I->CodeWindow.Eval(Context, ContextLen, is32bitCompat) };
			}
			break;

			case MACRO_SCANCODE_ENTER:
			{
				if (Root::I->CursorFocus == CursorFocus::Log)
				{
					eastl::pair<eastl::string, Cmd*> cmd = Root::I->InputLine.Enter();

					if (cmd.second)
					{
						co_await BcAwaiter_Join{ ProcessCmd(cmd, exit, Context, ContextLen, is32bitCompat, jumpDest, &thisInstr, thisOps) };
					}
				}
				else if (Root::I->CursorFocus == CursorFocus::Code)
				{
					Root::I->CodeWindow.Enter();
				}
			}
			break;

			case MACRO_SCANCODE_BackSp:
			{
				if (Root::I->CursorFocus == CursorFocus::Log)
					Root::I->InputLine.BackSpace();
				else if (Root::I->CursorFocus == CursorFocus::Code)
					Root::I->CodeWindow.BackSpace();
			}
			break;

			case MACRO_SCANCODE_Del:
			{
				if (Root::I->CursorFocus == CursorFocus::Log)
					Root::I->InputLine.Del();
				else if (Root::I->CursorFocus == CursorFocus::Code)
					Root::I->CodeWindow.Del();
			}
			break;

			case MACRO_SCANCODE_Tab:
			{
				if (Root::I->CursorFocus == CursorFocus::Log)
					Root::I->InputLine.Tab(shiftState);
				else if (Root::I->CursorFocus == CursorFocus::Code)
					Root::I->CodeWindow.Tab();
			}
			break;

			case MACRO_SCANCODE_F8:
			{
				co_await BcAwaiter_Join{ ProcessCmd(
					eastl::pair<eastl::string, Cmd*>("T", GetCmd("T")),
					exit, Context, ContextLen, is32bitCompat, jumpDest, &thisInstr, thisOps) };
			}
			break;

			case MACRO_SCANCODE_F5:
			case MACRO_SCANCODE_PrtSc:
			{
				co_await BcAwaiter_Join{ ProcessCmd(
					eastl::pair<eastl::string, Cmd*>("X", GetCmd("X")),
					exit, Context, ContextLen, is32bitCompat, jumpDest, &thisInstr, thisOps) };
			}
			break;

			case MACRO_SCANCODE_F10:
			{
				co_await BcAwaiter_Join{ ProcessCmd(
					eastl::pair<eastl::string, Cmd*>("P", GetCmd("P")),
					exit, Context, ContextLen, is32bitCompat, jumpDest, &thisInstr, thisOps) };
			}
			break;

			case MACRO_SCANCODE_Left:
			case MACRO_SCANCODE_Right:
			case MACRO_SCANCODE_Up:
			case MACRO_SCANCODE_Down:
			case MACRO_SCANCODE_PgUp:
			case MACRO_SCANCODE_PgDn:
			case MACRO_SCANCODE_Home:
			case MACRO_SCANCODE_End:
			{
				if (ctrlState)
				{
					LONG navigate = 0;

					const ULONG contentsDim = Root::I->DisasmWindow.destY1 - Root::I->DisasmWindow.destY0 + 1;

					BOOLEAN proceed = TRUE;

					switch (scancode)
					{
					case MACRO_SCANCODE_Left:
						if (Root::I->DisasmWindow.posX > 0) Root::I->DisasmWindow.posX--;
						navigate = DisasmWnd::NAVIGATE_PRESERVE_POS;
						break;
					case MACRO_SCANCODE_Right:
						Root::I->DisasmWindow.posX++;
						navigate = DisasmWnd::NAVIGATE_PRESERVE_POS;
						break;
					case MACRO_SCANCODE_Up:
						navigate--;
						break;
					case MACRO_SCANCODE_Down:
						navigate++;
						break;
					case MACRO_SCANCODE_PgUp:
						navigate -= contentsDim - Root::I->DisasmWindow.symNameLinesNum;
						break;
					case MACRO_SCANCODE_PgDn:
						navigate += contentsDim - Root::I->DisasmWindow.symNameLinesNum;
						break;
					default:
						proceed = FALSE;
						break;
					}

					if (proceed)
					{
						co_await BcAwaiter_Join{ Root::I->DisasmWindow.Disasm(Context, Root::I->DisasmWindow.current32bitCompat, jumpDest, navigate) };

						Wnd::DrawAll_End();
						Wnd::DrawAll_Final();
					}
				}
				else if (altState)
				{
					switch (scancode)
					{
					case MACRO_SCANCODE_Left:
						Root::I->LogWindow.Left();
						break;
					case MACRO_SCANCODE_Right:
						Root::I->LogWindow.Right();
						break;
					case MACRO_SCANCODE_Up:
						Root::I->LogWindow.Up();
						break;
					case MACRO_SCANCODE_Down:
						Root::I->LogWindow.Down();
						break;
					case MACRO_SCANCODE_PgUp:
						Root::I->LogWindow.PgUp();
						break;
					case MACRO_SCANCODE_PgDn:
						Root::I->LogWindow.PgDn();
						break;
					case MACRO_SCANCODE_Home:
						Root::I->LogWindow.Home();
						break;
					case MACRO_SCANCODE_End:
						Root::I->LogWindow.End();
						break;
					}

					Wnd::DrawAll_End();
					Wnd::DrawAll_Final();
				}
				else
				{
					if (Root::I->CursorFocus == CursorFocus::Log)
					{
						switch (scancode)
						{
						case MACRO_SCANCODE_Left:
							Root::I->InputLine.Left();
							break;
						case MACRO_SCANCODE_Right:
							Root::I->InputLine.Right();
							break;
						case MACRO_SCANCODE_Up:
							Root::I->InputLine.Up();
							break;
						case MACRO_SCANCODE_Down:
							Root::I->InputLine.Down();
							break;
						case MACRO_SCANCODE_Home:
							Root::I->InputLine.Home();
							break;
						case MACRO_SCANCODE_End:
							Root::I->InputLine.End();
							break;
						}
					}
					else if (Root::I->CursorFocus == CursorFocus::Code)
					{
						switch (scancode)
						{
						case MACRO_SCANCODE_Left:
							Root::I->CodeWindow.Left();
							break;
						case MACRO_SCANCODE_Right:
							Root::I->CodeWindow.Right();
							break;
						case MACRO_SCANCODE_Up:
							Root::I->CodeWindow.Up();
							break;
						case MACRO_SCANCODE_Down:
							Root::I->CodeWindow.Down();
							break;
						case MACRO_SCANCODE_Home:
							Root::I->CodeWindow.Home();
							break;
						case MACRO_SCANCODE_End:
							Root::I->CodeWindow.End();
							break;
						case MACRO_SCANCODE_PgUp:
							Root::I->CodeWindow.PgUp();
							break;
						case MACRO_SCANCODE_PgDn:
							Root::I->CodeWindow.PgDn();
							break;
						}
					}
				}
			}
			break;

			default:
				co_await BcAwaiter_RecvTimeout{};
			}

			if (exit) break;
		}
	}

	// return from the coroutine, i.e. trigger a DbgKdContinueApi state manipulate request.

	co_return;
}

BcCoroutine Main::ProcessCmd(const eastl::pair<eastl::string, Cmd*>& cmd, BOOLEAN& exit, BYTE* Context, ULONG ContextLen, BOOLEAN is32bitCompat, ULONG64 jumpDest, ZydisDecodedInstruction* thisInstr, ZydisDecodedOperand* thisOps) noexcept
{
	CmdParams params;

	params.cmd = eastl::move(cmd.first);
	params.context = Context;
	params.contextLen = ContextLen;
	params.is32bitCompat = is32bitCompat;
	params.jumpDest = jumpDest;
	params.thisInstr = thisInstr;
	params.thisOps = thisOps;

	co_await BcAwaiter_Join{ cmd.second->Execute(params) };

	if (params.result == CmdParamsResult::Continue)
		exit = TRUE;
	else if (params.result == CmdParamsResult::RefreshCodeAndRegsWindows)
	{
		Root::I->RegsWindow.DrawRegs(params.context, params.is32bitCompat);

		co_await BcAwaiter_Join{ Root::I->DisasmWindow.Disasm(params.context, Root::I->DisasmWindow.current32bitCompat, params.jumpDest, DisasmWnd::NAVIGATE_PRESERVE_POS) };

		Wnd::DrawAll_End();
		Wnd::DrawAll_Final();
	}
	else if (params.result == CmdParamsResult::Unassemble)
	{
		// is a 32 bit module in a 64 bit native OS?

		BOOLEAN compatMode = FALSE;

		if (_6432_(TRUE, FALSE))
		{
			if (!(params.resultUL64 & 0xFFFFFFFF00000000))
			{
				ULONG64 eproc = 0;
				co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(eproc) };

				if (eproc)
				{
					ULONG64 wow64Peb = 0;
					co_await BcAwaiter_Join{ Platform::GetWow64Peb(wow64Peb, eproc) };

					if (wow64Peb)
					{
						compatMode = TRUE; // this is the default if we are in a WOW64 process and address < 4GB.

						auto fif = eastl::find_if(Root::I->NtModules->begin(), Root::I->NtModules->end(),
							[&](const eastl::pair<ULONG64, eastl::vector<NtModule>>& e) { return e.first == eproc; });

						if (fif != Root::I->NtModules->end())
						{
							for (auto& mod : fif->second)
							{
								if (params.resultUL64 >= mod.dllBase && params.resultUL64 < mod.dllBase + mod.sizeOfImage)
								{
									if (mod.arch == 64) compatMode = FALSE; // override the default value, see, for example, wow64cpu.dll.
									break;
								}
							}
						}
					}
				}
			}
		}

		// unassemble in the code window.

		Root::I->DisasmWindow.SetPos(params.resultUL64);

		co_await BcAwaiter_Join{ Root::I->DisasmWindow.Disasm(params.context, compatMode, params.jumpDest, DisasmWnd::NAVIGATE_PRESERVE_POS) };

		Wnd::DrawAll_End();
		Wnd::DrawAll_Final();
	}
	else if (params.result == CmdParamsResult::RedrawUi)
	{
		co_await BcAwaiter_Join{ DrawInterface(Context, is32bitCompat, jumpDest) };
	}

	co_return;
}

Cmd* Main::GetCmd(const CHAR* id)
{
	for (auto& c : Root::I->Cmds)
		if (Utils::AreStringsEqualI(id, c.get()->GetId()))
			return c.get();

	return NULL;
}

BcCoroutine Main::SetStepBp(ULONG64 addr) noexcept
{
	// we allow only one step bp.

	for (int i = Root::I->BreakPoints.size() - 1; i >= 0; i--)
		if (Root::I->BreakPoints[i].isStepBp)
		{
			// remove the bp from the system.

			ULONG handle = Root::I->BreakPoints[i].handle;

			if (handle)
				co_await BcAwaiter_StateManipulate{
				[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					pState->ApiNumber = DbgKdRestoreBreakPointApi;

					pState->ReturnStatus = STATUS_PENDING;

					pState->u.RestoreBreakPoint.BreakPointHandle = handle;
				},
				[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					return pState->ApiNumber == DbgKdRestoreBreakPointApi && NT_SUCCESS(pState->ReturnStatus);
				} };

			// remove the bp from the vector.

			Root::I->BreakPoints.erase(Root::I->BreakPoints.begin() + i);
		}

	// is the address valid ?

	if (!addr)
		co_return;

	// do not trace with a step breakpoint.

	Root::I->Trace = FALSE;

	// try to create the BP. We accept failure, since a BP at "addr" could have been already set.

	ULONG handle = 0;

	co_await BcAwaiter_StateManipulate{
	[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

		pState->ApiNumber = DbgKdWriteBreakPointApi;

		pState->ReturnStatus = STATUS_PENDING;

		pState->u.WriteBreakPoint.BreakPointHandle = 0;
		pState->u.WriteBreakPoint.BreakPointAddress = addr;
	},
	[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

		if (pState->ApiNumber == DbgKdWriteBreakPointApi)
		{
			if (NT_SUCCESS(pState->ReturnStatus) &&
				pState->u.WriteBreakPoint.BreakPointHandle)
			{
				handle = pState->u.WriteBreakPoint.BreakPointHandle;

				return TRUE;
			}
		}

		return FALSE;
	} };

	// add the BP at the beginning of the vector, so it will be processed first.

	BreakPoint bp;

	bp.address = addr;
	bp.handle = handle;
	bp.ethread = Root::I->StateChange.Thread;
	bp.isStepBp = TRUE;

	Root::I->BreakPoints.insert(Root::I->BreakPoints.begin(), eastl::move(bp));

	// return to the caller.

	co_return;
}

BcCoroutine Main::DisasmCurrInstr(ZydisDecodedInstruction& thisInstr, ZydisDecodedOperand* thisOps, JumpInstrType& jumpType, ULONG64& stepAddr, BYTE* Context, BOOLEAN is32bitCompat) noexcept
{
	const size_t thisInstrMaxSize = 64;

	auto ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)((CONTEXT*)Context)->_6432_(Rip, Eip), thisInstrMaxSize };
	if (ptr)
	{
		ZydisDecoder decoder;
		::ZydisDecoderInit(&decoder,
			_6432_(is32bitCompat ? ZYDIS_MACHINE_MODE_LONG_COMPAT_32 : ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_MACHINE_MODE_LEGACY_32),
			_6432_(is32bitCompat ? ZYDIS_STACK_WIDTH_32 : ZYDIS_STACK_WIDTH_64, ZYDIS_STACK_WIDTH_32));

		if (ZYAN_SUCCESS(::ZydisDecoderDecodeFull(&decoder,
			ptr, thisInstrMaxSize,
			&thisInstr, thisOps,
			ZYDIS_MAX_OPERAND_COUNT_VISIBLE, ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY)))
		{
			ULONG64 stepAddrImm = 0, stepAddrPtr = 0, stepAddrPtrSize = 0;

			jumpType = X86Step::CalculateStepDestination((CONTEXT*)Context, thisInstr, thisOps, is32bitCompat, stepAddrImm, stepAddrPtr, stepAddrPtrSize);

			if (stepAddrImm)
			{
				stepAddr = stepAddrImm;
			}
			else if (stepAddrPtr && stepAddrPtrSize)
			{
				// try to read the destination of the jump.

				ULONG64 stepAddrPtrReadResult = 0;

				if (stepAddrPtrSize == _6432_(is32bitCompat ? 4 : 8, 4))
				{
					auto ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)stepAddrPtr, (size_t)stepAddrPtrSize };
					if (ptr)
						stepAddrPtrReadResult = stepAddrPtrSize == 8 ? *(ULONG64*)ptr : *(ULONG32*)ptr;
				}

				if (stepAddrPtrReadResult)
					stepAddr = stepAddrPtrReadResult;
			}
		}
	}
}

BcCoroutine Main::DrawInterface(BYTE* Context, BOOLEAN is32bitCompat, ULONG64 jumpDest) noexcept
{
	// little hack: determine the vertical dimension of the registers window.

	BOOLEAN sign = Root::I->RegsDisasmDivLineY < 0;

	Wnd::DrawAll_Start(); // <=== need to call this twice: here and in the draw interface code below, since RegsDisasmDivLineY may change.

	Root::I->RegsWindow.DrawRegs(Context, is32bitCompat);

	Root::I->RegsDisasmDivLineY = (sign ? -1 : 1) * (Root::I->RegsWindow.contents.size() + 1);

	// draw the interface.

	Wnd::DrawAll_Start();

	co_await BcAwaiter_Join{ Root::I->DisasmWindow.Disasm(Context, is32bitCompat, jumpDest) };

	Wnd::DrawAll_End();
	Wnd::DrawAll_Final();
}
