#pragma once

namespace BazisLib
{
	namespace _PosixSyncImpl
	{
		//! Implements a semaphore using a condition variable and a mutex. Efficiently emulates the TryWait() method not available on Posix.
		template<class _ConditionVariableWithMutex> class PosixBasedSemaphore
		{
		private:
			unsigned _Count;

			_ConditionVariableWithMutex _Condition;

		public:
			PosixBasedSemaphore(unsigned initialCount = 0)
				: _Count(initialCount)
			{
			}

			void Signal()
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);
				if (!_Count++)
					_Condition.Signal(true);
			}

			void Wait()
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);
				while(!_Count)
					_Condition.WaitWhenLocked();

				_Count--;
			}

			bool TryWait(unsigned timeoutInMsec = 0)
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);
				//ts is an absolute value computed outside the loop, as we are waiting maximum timeoutInMsec milliseconds starting from the function call.
				typename _ConditionVariableWithMutex::_Deadline ts = _ConditionVariableWithMutex::MsecToDeadline(timeoutInMsec);	
				while(!_Count)
					if (!_Condition.TryWaitWhenLocked(ts))
						return false;

				_Count--;
				return true;
			}
		};

		//! Implements an auto-reset or a manual-reset event based on a Posix condition variable
		/*!
			POSIX-based environments do not provide auto-reset and manual-reset event objects compatible with Windows event semantics.
			However, all of them provide POSIX condition variables. This class is a universal implementation of a Windows-style event based on
			a ConditionVariableWithMutex primitive provided by the underlying OS.
		*/
		template<class _ConditionVariableWithMutex> class PosixBasedEvent
		{
		private:
			_ConditionVariableWithMutex _Condition;
			bool _State, _AutoReset;

		public:
			PosixBasedEvent(bool initialState = false, BazisLib::EventType eventType = kManualResetEvent)
			{
				_State = initialState;
				_AutoReset = (eventType == kAutoResetEvent);
			}

			void Wait()
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);

				if (_AutoReset)
				{
					//IF more than 1 thread is released by pthread_cond_signal(), we still let only one thread through.
					while(!_State)
						_Condition.WaitWhenLocked();
					_State = false;
				}
				else
				{
					if (!_State)
						_Condition.WaitWhenLocked();
				}
			}


			bool TryWait(unsigned timeoutInMilliseconds)
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);
				if (_State)
				{
					if (_AutoReset)
						_State = false;
					return true;
				}

				typename _ConditionVariableWithMutex::_Deadline ts = _ConditionVariableWithMutex::MsecToDeadline(timeoutInMilliseconds);
				if (_AutoReset)
				{
					while (!_State)
						if (!_Condition.TryWaitWhenLocked(ts))
							return false;
					_State = false;
				}
				else
					if (!_Condition.TryWaitWhenLocked(ts))
						return false;

				return true;
			}

			void Set()
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);
				_State = true;
				_Condition.Signal(!_AutoReset);
			}

			void Reset()
			{
				FastLocker<_ConditionVariableWithMutex> lck(_Condition);
				_State = false;
			}
		};
	}
}