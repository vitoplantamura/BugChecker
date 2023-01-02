#pragma once
#include "cmndef.h"

namespace BazisLib
{
	namespace Win32
	{
		//! Allows to turn standard GDI controls into double-buffered ones
		/*! This class allows using standard GDI controls, such as list views, in a double-buffered
			mode. That approach completely removes flickering on fast information update for these
			controls. You should never use the _DoubleBufferedControlHook directly. Instead use the
			DoubleBufferedControlHook or one of the specialized clases, such as DoubleBufferedStaticText:
<pre>
class WindowClass
{
...
	DoubleBufferedControlHook m_Hook;
...

	OnCreate(...) //Or OnInitDialog()
	{
		...
		m_Hook.HookControl(GetDlgItem(...));
		// ---OR---
		m_Hook.HookControl(m_hwndSomeControl);
	}

}
</pre>
			\param _CallWindowProc allows specifying user-defined function that calls the original window procedure,
				   performing some additional filtering, if required. By default, this parameter is set to
				   CallWindowProc function provided by Windows. See the DoubleBufferedStaticText implementation for
				   usage example

			\attention The _DoubleBufferedControlHook class introduces several restrictions to the hooked window:
<ul>
	<li>The GWL_USERDATA is managed by the _DoubleBufferedControlHook class and should not be changed by
		any other facility.
	<li>The hooked window should have the WM_PRINT message support. Most of the standard GDI controls support it
	(even under Windows CE where it is simply mapped to WM_PAINT with HDC in wParam). However, hooking user-provided
	controls may produce no result, as their window procedures may be unaware of this message.
</ul>
			\remarks On Windows CE the STATIC control does not support WM_PRINT message, that way the DoubleBufferedControlHook
					 will not work with it. You should use DoubleBufferedStaticText instead.
		*/
		template <LRESULT (WINAPI *_CallWindowProc)(WNDPROC, HWND, UINT, WPARAM, LPARAM) = CallWindowProc> class _DoubleBufferedControlHook
		{
		private:
			WNDPROC m_OldWindowProc;
			HBRUSH m_hBackBrush;

		private:
			//! Overrides the window proc for the hooked control
			/*! This static method is called each time a message is sent to the hooked control. It actually
				handles only two messages: WM_ERASEBKGND and WM_PAINT. All other messages are simply
				forwarded to the original message function. For the WM_ERASEBKGND no work is performed, and
				WM_PAINT handler creates a temporary bitmap, invokes the original window procedure with it,
				and then copies the bitmap to the original DC.
				\remarks The present version creates the bitmap/memory DC pair on each WM_PAINT call. This
						 may be slower than having a cached one, however, this does not use the memory for
						 bitmap outside the WM_PAINT call.
			    \attention DO NOT use the GWL_USERDATA for a hooked control.
			*/
			static LRESULT CALLBACK NewWindowProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
			{
				_DoubleBufferedControlHook *pHook = (_DoubleBufferedControlHook *)GetWindowLong(hWnd, GWL_USERDATA);
				if (!pHook)
					return DefWindowProc(hWnd, wMsg, wParam, lParam);
				if (wMsg == WM_ERASEBKGND)
				{
					return 0;
				}
				if (wMsg == WM_PAINT)
				{
					PAINTSTRUCT ps;
					if (wParam == 0)
						BeginPaint(hWnd, &ps);
					else
						ps.hdc = (HDC)wParam;

					GetClientRect(hWnd, &ps.rcPaint);

					HDC hDc = CreateCompatibleDC(ps.hdc);
					ASSERT(hDc);
					HBITMAP hBmp = CreateCompatibleBitmap(ps.hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
					ASSERT(hBmp);
					SelectObject(hDc, hBmp);
					FillRect(hDc, &ps.rcPaint, pHook->m_hBackBrush);
					
					_CallWindowProc(pHook->m_OldWindowProc, hWnd, WM_PRINT, (WPARAM)hDc, lParam);

					BitBlt(ps.hdc,
						   ps.rcPaint.left ,
						   ps.rcPaint.top,
						   ps.rcPaint.right - ps.rcPaint.left,
						   ps.rcPaint.bottom - ps.rcPaint.top,
						   hDc,
						   0,
						   0,
						   SRCCOPY);


					DeleteDC(hDc);
					DeleteObject(hBmp);

					if (wParam == 0)
						EndPaint(hWnd, &ps);
					return 0;
				}
				return CallWindowProc(pHook->m_OldWindowProc, hWnd, wMsg, wParam, lParam);
			}

		public:
			//! Creates the double-buffer hook control with a background brush
			/*! As we ignore the WM_ERASEBKGND message and do all the erasing inside WM_PAINT,
				the background color or brush should be known to our override class. Use either
				this constructor, or the constructor taking a system color reference to specify
				it.
			*/
			_DoubleBufferedControlHook(HBRUSH hBackBrush) :
			  m_OldWindowProc(NULL),
			  m_hBackBrush(hBackBrush)
			{
			}

			_DoubleBufferedControlHook(ULONG SystemColor = COLOR_BTNFACE) :
			  m_OldWindowProc(NULL)
			{
#ifdef UNDER_CE
				if (SystemColor & SYS_COLOR_INDEX_FLAG)
					m_hBackBrush = (HBRUSH)SystemColor;
				else
#endif
					m_hBackBrush = CreateSolidBrush(GetSysColor(SystemColor));
			}

			~_DoubleBufferedControlHook()
			{
#ifdef UNDER_CE
				if (!((ULONG)m_hBackBrush & SYS_COLOR_INDEX_FLAG))
#endif
					DeleteObject(m_hBackBrush);
			}

			//! Performs actual control hooking
			/*! This method should be called after the control that is to be hooked was
				created.
				\param hWnd Specifies the window handle for the control to hook.
				\remarks The DoubleBufferedControlHook would most likely not work with
						 user-provided controls, because they do not support WM_PRINT.
						 See class description for details.
			*/
			bool HookControl(HWND hWnd)
			{
				if (m_OldWindowProc)
					return false;
				m_OldWindowProc = (WNDPROC)GetWindowLongPtr(hWnd, GWL_WNDPROC);
				SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)this);
				SetWindowLongPtr(hWnd, GWL_WNDPROC, (LONG)NewWindowProc);
				return true;
			}

		};

		namespace StaticTextHelper
		{
			inline LRESULT WINAPI WindowProcInvoker(
				WNDPROC lpPrevWndFunc,
				HWND hWnd,
				UINT wMsg,
				WPARAM wParam,
				LPARAM lParam)
			{
				enum {STATIC_BUF_SIZE = 128};
				if (wMsg == WM_PRINT)
				{
					HDC hDc = (HDC)wParam;
					RECT rect;
					GetClientRect(hWnd, &rect);
					SetBkMode(hDc, TRANSPARENT);
					unsigned textLen = GetWindowTextLength(hWnd);
					if (textLen >= STATIC_BUF_SIZE)
					{
						TCHAR *pText = new TCHAR[textLen + 1];
						if (pText)
						{
							GetWindowText(hWnd, pText, textLen + 1);
							DrawText(hDc, pText, textLen, &rect, DT_WORDBREAK);
						}
						delete pText;
					}
					else
					{
						TCHAR buf[STATIC_BUF_SIZE];
						GetWindowText(hWnd, buf, STATIC_BUF_SIZE);
						DrawText(hDc, buf, textLen, &rect, DT_WORDBREAK);
					}
					return 0;
				}
				return CallWindowProc(lpPrevWndFunc, hWnd, wMsg, wParam, lParam);
			}
		}


		typedef _DoubleBufferedControlHook<> DoubleBufferedControlHook;
		typedef _DoubleBufferedControlHook<StaticTextHelper::WindowProcInvoker> DoubleBufferedStaticText;
	}

}