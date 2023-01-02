#pragma once
#include "cmndef.h"
#include "assert.h"

#ifdef BZS_POSIX
#include "Posix/thread.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/thread.h"
#elif defined (_WIN32)
#include "Win32/thread.h"
#elif defined (BZSLIB_KEXT)
#include "KEXT/thread.h"
#else
#error No thread implementation defined for this platform
#endif

namespace BazisLib
{
#ifdef WIN32
#define __THREAD_STARTER __stdcall
#else
#define __THREAD_STARTER
#endif

#ifndef THREAD_STARTER_ADDITIONAL_ARGS
#define THREAD_STARTER_ADDITIONAL_ARGS
#endif

	template <bool AutoDelete> class _Thread : public _ThreadPrivate::_BasicThread<_Thread<AutoDelete> >
	{
	private:
		typedef _ThreadPrivate::_BasicThread<_Thread<AutoDelete> > _Base;

		static typename _Base::_ThreadBodyReturnType __THREAD_STARTER ThreadStarter(void *lp THREAD_STARTER_ADDITIONAL_ARGS)
		{
			ASSERT(lp);
			_Thread *pThread = ((_Thread*)((_Base*)lp));
			int result = pThread->ThreadBody();
			if (AutoDelete)
				delete pThread;
			return _Base::_ReturnFromThread(lp, result);
		}

		friend class _ThreadPrivate::_BasicThread<_Thread>;

	protected:
		virtual int ThreadBody()=0;
		virtual ~_Thread() {}

	public:
		_Thread() {}
	};

	class FunctionThread : public _ThreadPrivate::_BasicThread<FunctionThread>
	{
	private:
		typedef int (*PTHREADPROC)(void *lpParam);
		typedef _ThreadPrivate::_BasicThread<FunctionThread> _Base;

		PTHREADPROC m_pProc;
		void *m_lpParam;

	public:
		static inline _Base::_ThreadBodyReturnType __THREAD_STARTER ThreadStarter(void *lp THREAD_STARTER_ADDITIONAL_ARGS)
		{
			FunctionThread *pThread = ((FunctionThread *)((_Base*)lp));
			ASSERT(pThread && pThread->m_pProc);
			return _Base::_ReturnFromThread(lp, pThread->m_pProc(pThread->m_lpParam));
		}

	public:
		FunctionThread(PTHREADPROC pProc, void *lpParam = NULL) :
			m_pProc(pProc),
			m_lpParam(lpParam)
			{
				ASSERT(pProc);
			}
	};

		//! Allows executing an arbitrary method of an arbitrary class in a separate thread
		/*! This class represents a thread executing an arbitrary method of an arbitrary class.
			The typical use of this class is the following:
<pre>
class ThreadDemo
{
public:
	//Any compatible return type, any calling convention!
	int ThreadBody()
	{
		return 0;
	}

	MemberThread m_Thread;

	Test()
		: m_Thread(this, &Test::ThreadBody)
	{
	}
};
</pre>
	\remarks Internally, this class does the following:
		- Converts a pointer to member to void * and stores it in m_pThreadProc
		- Sets a pointer to correct MemberInvoker to m_pMemberInvoker
		- Starts a thread via _BasicThread
		- _BasicThread invokes ThreadStarter, that calls correct MemberInvoker via m_pMemberInvoker
		- MemberInvoker performs correct (w.r.t. calling convention, return type) call to the member
		*/
	class MemberThread : public _ThreadPrivate::_BasicThread<MemberThread>
	{
	private:
		typedef int (MemberThread::*PMEMBER_INVOKER)();
		typedef _ThreadPrivate::_BasicThread<MemberThread> _Base;
		void *m_pClass;

		PMEMBER_INVOKER m_pMemberInvoker;
		INT_PTR m_pThreadProc[2];

	public:

		static inline _Base::_ThreadBodyReturnType __THREAD_STARTER ThreadStarter(void *lp THREAD_STARTER_ADDITIONAL_ARGS)
		{
			MemberThread *pThread = ((MemberThread *)((_Base *)lp));
			return _Base::_ReturnFromThread(lp, (pThread->*pThread->m_pMemberInvoker)());
		}

	private:
		template <class _MemberClass, typename _MemberType> int MemberInvoker()
		{
			_MemberType pMember;
			C_ASSERT(sizeof(pMember) <= sizeof(m_pThreadProc));
			memcpy(&pMember, m_pThreadProc, sizeof(pMember));
			return (((_MemberClass *)m_pClass)->*pMember)();
		}

	public:
		//! Converts a pointer to a member to a void * and selects a corresponding MemberInvoker.
		/*!
			\remarks Note that the template argument for this method is chosen automatically
						during compilation. Also, only one correct version of MemberInvoker is
						compiled and referenced by m_pMemberInvoker.
		*/
		template <class _MemberClass, typename _MemberType> MemberThread(_MemberClass *pClass, _MemberType pProc)
			: m_pClass(pClass)
			{
				ASSERT(pProc);
				//HACK! HACK! HACK! C++ does not allow converting member pointers to void *. We override this using memcpy().
				C_ASSERT(sizeof(_MemberType) <= sizeof(m_pThreadProc));
				memcpy(m_pThreadProc, &pProc, sizeof(_MemberType));

				//MemberInvoker will convert m_pThreadProc back to _MemberType and call it. This allows us
				//to omit _MemberType specification during class declaration. As _MemberType is a template parameter
				//of the constructor, it is determined automatically by the compiler.
				m_pMemberInvoker = &MemberThread::MemberInvoker<_MemberClass, _MemberType>;
			}
	};
	
	typedef _Thread<false>		Thread;
	typedef _Thread<true>		AutoDeletingThread;
}


//The following classes should be declared inside platform-specific headers
using BazisLib::Thread;
using BazisLib::FunctionThread;
