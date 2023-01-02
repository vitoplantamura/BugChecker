#pragma once
#include "cmndef.h"
#include <set>
#include <assert.h>
#include "atomic.h"
#include "sync.h"
#include "status.h"

#if !defined(ASSERT) && !defined(BZSLIB_WINKERNEL)
#define ASSERT _ASSERT
#endif

//Disable "'this' used in member init list" warning. Our use case that generates this warning is safe.
#pragma warning(disable:4355)

namespace BazisLib
{
	//! Contains classes that implement the referencing counting within the service/client model.
	/*! \namespace BazisLib::ObjectManager
		\attention Before implementing anything using ObjectManager, see the AUTO_THIS macro reference
		The object manager differs from most of garbage collectors that provide automatic object deletion when
		such object is no longer used. The main idea of the object manager is to automate reference counting
		and to eliminate the need for a separate 'service thread'. An object should be deleted synchronously and
		instantly when it is no longer used. This requirement introduces several limiting conditions on the object
		structure. A detailed explanation of Object Manager concepts and use examples can be found
		\ref objmgr "in this article".
	*/

	/*! \page objmgr Object Manager concepts
		\section objmgr_constref Changes in BazisLib 2.01
		A new template class has been added to BazisLib 2.01: ConstManagedPointer. References to it should be used as
		function/template parameters to optimize performance. See its description for details.
		\section objmgr_trees General considerations and definitions
		An object that supports reference counting is called a <b>managed object</b>. Every class that represents a
		managed object is a direct or inderect descendant of the \link ObjectManager::ManagedObjectBase ManagedObjectBase \endlink
		class. A managed object cannot be allocated in stack or statically. The only correct way to allocate such objects is to
		use the new() operator. <i>The debug build of the framework checks the newly created objects whether they were
		allocated from heap, however, the release build does not.</i> All managed objects are organized in a tree-like
		structure. Practically, an object that wants to store the pointer to another object, should become a <b>client</b>
		for that object. An object, that may have clients is called a <b>service</b> and should be a direct or indirect
		descendant of the \link ObjectManager::ServiceObject ServiceObject \endlink class. A <b>root service</b> that
		is not a client of anyone, should be a child (directly) of the \link ObjectManager::ServiceObject ServiceObject \endlink
		class. The object manager ensures that a service is not deleted until all its clients are
		freed. A client may be a service at the same time. In the simpliest case, such object can inherit the 
		\link ObjectManager::IntermediateServiceObject IntermediateServiceObject\endlink class and use the
		IntermediateServiceObject::GetParentService() method to get its service. However, if an object (no matter,
		whether it is a service or not) wants to be a client of a multiple services, it should use managed field
		pointers. Mind that you should avoid <b>circular references</b> when designing your classes. This means, that
		a service cannot become a client for any of its clients. The debug build of the framework detects such problems.
		\section objmgr_fieldp Managed field pointers
		If you want to store a pointer to a service inside a client, you should never do it directly. Instead, use the
		following scheme:
		<ul>
			<li>Use DECLARE_REFERENCE() macro to declare a reference.</li>
			field as a simple pointer: dereference, or assign new values.
			<li>Use INIT_REFERENCE() macro to initialize a reference from a constructor.</li>
			<li>Use ZERO_REFERENCE() macro to initialize an empty (zeroed) reference. Basically,
				the ZERO_REFERENCE(a) call is equivalent to INIT_REFERENCE(a, NULL).</li>
			<li>Use COPY_REFERENCE() macro to copy a reference inside a copy constructor.</li>
		</ul>
		To illustrate the rules by an example, let's assume that we have two service classes:
		a <b>Disk</b> and a <b>File</b>. A <b>File</b> is a client for a <b>Disk</b>. Also, a <b>Disk</b> object is
		deleted after all file objects are freed. Let's see how this example is implemented:
			<pre>
class Disk;

class File
{
private:
	DECLARE_REFERENCE(Disk, m_pDisk);
	int m_SomeVariable;

public:
	File(Disk *pDisk) :
	INIT_REFERENCE(m_pDisk, pDisk),
	m_SomeVariable(SomeValue)
	{
	}

	File(const File &file) :
	COPY_REFERENCE(m_pDisk, file)
	m_SomeVariable(SomeValue)
	{
	}
};
			</pre>
			
		Note that we declared a copy constructor. It is required only if you want to enable copying of your objects.
		If not, it is not required and an attempt to perform such copying will result in a compilation error. In
		case you really need it, simply use the COPY_REFERENCE() macro to ensure that all pointers are updated
		correctly.
		\remarks The implementation of such managed pointers is very simple. On creation, it registers its owner
		as a client (by calling ServiceObject::RegisterClient()) and on deletion (or when its value is changed),
		it calls ServiceObject::DeregisterClient(). This is smarter than incrementing/decrementing the reference
		counter, as it provides additional features, such as broadcasting.
		\section objmgr_mptr Generic managed pointers
		If you want to declare a managed pointer that will not be a field of a managed object, you should use the
		ObjectManager::ManagedPointer class. It ensures that the reference counter is decreased automatically after
		the pointer is deleted. It also supports copying.
		\section objmgr_umptr Using unmanaged pointers
		An unmanaged pointer is a native C++ pointer (like SomeObject *pObj). Note that if some method returns you
		an unmanaged pointer, you are responsible for releasing it by calling ManagedObjectBase::Release() method.
		However, when you initialize any type of managed pointer using an unmanaged one, it takes the ownership
		of the object (it does not retain the object another time, however, it releases it on deletion). Note that
		the only way to get an unmanaged pointer to an object is to call the ManagedPointer::RetainAndGet() method
		that returns a previously retained unmanaged pointer.
		The described scheme implies a set of simple rules to follow:
		<ul>
			<li>When possible, avoid using unmanaged pointers. If you got an unmanaged pointer, use it to iniaialize
				a managed one immediately and forget the unmanaged one.</li>
			<li>Use references to ConstManagedPointer class when declaring method parameters/return types</li>
			<li>DO NOT use an unmanaged pointer after you passed it to some method, or initialized a managed one</li>
		</ul>
		A sample illustrating object manager usage is located under SAMPLES\\COMMON\\OBJMGR, while an autotest program
		is in TESTS\\OBJMGR.
		\section objmgr_cmds Executing commands
		There are some cases that can require a service to tell its clients some information. For example, a disk
		object that detected device removal may want to notify the file objects about this event. Moreover, this
		is the only way for a service to interact with its clients. To send a message to all the clients of a service,
		you should call the ServiceObject::CallClients() method. It notifies every client of the service by calling
		ManagedObjectBase::ExecuteCommand() method. If one of the clients returns an error code, the method does not
		call other clients and return that code. On successful termination, it returns a value of CommonErrorType::Success.
		\remarks Note that recursive child calls are not supported to prevent command code collisions. Normally,
		a set of command codes that can be sent by a service to its clients is defined by the type of this service.
		That way, a client that connects to a service should be aware of handling the codes provided by that service.
		If the <b>IgnoreNotSupported</b> parameter is set, the clients returning \link BazisLib::CommonErrorType
		NotSupported \endlink value will not cause the call to fail, if not - they will.
		\section objmgr_autointf Auto interfaces
		An auto interface is an interface for a managed object. Actually, it is an interface that is virtually inherited
		from the ServiceObject class. Virtual inheritance ensures that an object implementing multiple managed interfaces
		contains a single instance of the ServiceObject class. See \ref AUTO_INTERFACE and \ref AUTO_OBJECT documentation
		for usage examples. Note that if you want to declare two equivalent interfaces, one managed and one unmanaged, use the
		MAKE_AUTO_INTERFACE macro. To declare a pair of classes (one for managed and one for unmanaged interface), use
		the following scheme: <pre>
template \<class _Parent\> class _Cxxx : public _Parent
{
	...
};

typedef _Cxxx<Ixxx> UnmanagedXxx;
typedef _Cxxx<AIxxx> ManagedXxx;
		</pre>

		\section objmgr_summary Key points to remember
		<ul>
			<li>Avoid circular references</li>
			<li>Allocate managed objects only in heap</li>
			<li>If you want to use a service from a client, use DECLARE_REFERENCE() macro</li>
			<li>If you want to declare a managed pointer inside the stack, or as a parameter, or as a return type,
			use the ObjectManager::ManagedPointer template class.</li>
			<li>If you want to notify all clients of a service, use the ServiceObject::CallClients() method</li>
			<li>An unmanaged pointer is always a retained one, whenever you see one. This fact is used in managed
			pointer constructors. So, DO NOT use an unmanaged pointer AFTER you create a managed one based on it.</li>
		</ul>
	*/

	namespace ObjectManager
	{
#ifdef BZSLIB_WINKERNEL
		typedef FastMutex ObjectManagerMutex;
		typedef FastMutexLocker ObjectManagerMutexLocker;
#else
		typedef Mutex ObjectManagerMutex;
		typedef MutexLocker ObjectManagerMutexLocker;
#endif


		template <class _ManagedType> class ManagedPointer;
		template <class _ManagedType, class _InternalCollection> class ManagedCollectionBase;

//! Always use AUTO_THIS instead of 'this' pointer when passing 'this' as a managed parameter.
/*! One of the key principles of the ObjectManager library is that an unmanaged pointer is always referenced.
	It means that if something is created based on an unmanaged pointer, it is not referenced anymore.
	However, there is a language case that does not comply to this rule: the 'this' pointer. It is an unmanaged
	pointer that can be obtained using the corresponding keyword without referencing the object.
	To avoid this problem, use the AUTO_THIS macro instead of 'this' when passing a managed parameter.
	Example:<pre>

static void SomeStatic(ManagedPointer<SomeClass> param)
{
...
}

void SomeClass::SomeFunc()
{
ManagedPointer<SomeClass> ptr = AUTO_THIS;	//Correct

SomeStatic(this);	//Incorrect!!!

SomeStatic(AUTO_THIS);	//Correct!
}
</pre>

*/
#define AUTO_THIS (Retain(), (this))

#define _MP(x) const BazisLib::ConstManagedPointer<x> &

		//! Used as a template argument to derive unmanaged classes
		class EmptyClass {};

		//! Represents a managed object.
		/*! This class represents a managed object. Any class that supports reference counting should be a descendant
			of this class. For details on managed objects, see \ref objmgr "this article".
		*/
		class ManagedObjectBase
		{
		public:
			enum
			{
				HEAP_ALLOCATION_SIGNATURE	=	'JBOM'	//= "MOBJ" = "Managed OBJect"
			};

		protected:
			//! Defines a system command used in ExecuteSystemCommand() method.
			enum SystemCommand
			{
				InvalidCommand,
	#ifdef _DEBUG
				FindCircularReferences,		//Param1 - pointer to a object to find
				DumpChildren,				//Param1 - Root DebugObjectReference; Param2 - pointer to parent DebugObjectReference
	#endif
				bzsQueryInterface,			//Param1 - interface GUID, Param 2 - interface version, Param 3 - non-managed interface pointer
			};

	#ifdef _DEBUG
			//! Represents a single managed object for debugging purposes.
			/*! This class is used to generate object tree dumps or to illustrate circular reference paths if they
				are detected.
				\remarks This class is not used in this version of the framework and is reserved for future use.
			*/
			struct DebugObjectReference
			{
				ManagedObjectBase *pNextObject;
				ManagedObjectBase *pFirstChild;

				DebugObjectReference(ManagedObjectBase *nextObject = NULL, ManagedObjectBase *firstChild = NULL) :
					pNextObject(nextObject),
					pFirstChild(firstChild)
				{
				}

				~DebugObjectReference()
				{
					if (pNextObject)
						delete pNextObject;
					if (pFirstChild)
						delete pFirstChild;
				}

			};
	#endif
	#pragma endregion

		private:
			AtomicInt32 m_ReferenceCount;

		//Managed objects that are non-services cannot be created outside the object manager library
		//If you want to define a managed object, use the ServiceObject class instead
		private:
			friend class ServiceObject;
			friend class ManagedObjectWithWeakPointerSupport;
			template <class _ServiceType> friend class WeakManagedPointer;

			//! Creates a managed object
			/*! The default constructor creates the managed object and sets its reference counter to 1.
				It means that a single ManagedObjectBase::Release() call is required to delete it immediately
				after creation.
				\remarks In the debug build of the framework, the ManagedObjectBase::ManagedObjectBase() constructor
					     checks whether the object is allocated from heap. This is done with the help of the
						 overloaded ManagedObjectBase::operator new().
			*/
			ManagedObjectBase()
			{
#ifdef _DEBUG
				ASSERT(!((INT_PTR)this % sizeof(unsigned)));	//Ensure proper buffer alignment
				//(this + 0) points to vtable and is initialized before the constructor. (this + 4h) points to the
				//first uninitialized DWORD. We fail, if it is not set to HEAP_ALLOCATION_SIGNATURE, as this
				//usually means that the object was not allocated from heap (using overriden new() operator) and
				//that case is bad since our objects are automatically deleted when reference count reaches zero.
				if (((unsigned *)this)[sizeof(void *)/sizeof(unsigned)] != HEAP_ALLOCATION_SIGNATURE)
				{
					//Oops! You tried to allocate a managed object from stack or statically. This is not allowed.
					//See notes in the beginning of the file for details.
					ASSERT(!"Managed objects should be allocated from heap by operator new()!");
				}
#endif
				m_ReferenceCount = 1;
			}

			//! Deletes a managed object
			/*! This destructor deletes a managed object. Note that managed objects should only be freed by calling
				the ManagedObject::Release() method. Direct call of the <b>delete</b> operator is not allowed. The
				debug build of the framework checks this.
			*/
			virtual ~ManagedObjectBase()
			{
				ASSERT(!m_ReferenceCount);
			}

		public:
			//! Returns the reference counter.
			/*! This method returns the reference counter of a managed object. This can be useful for debugging,
				however, you should never make any assumptions about the reference counter value.
			*/
			unsigned GetReferenceCount()
			{
				return m_ReferenceCount;
			}

			//! System command handler.
			/*! This method is an overrideable handler for system service-to-client commands. Such commands differ from
				user commands (handled by ManagedObjectBase::ExecuteCommand) and can even be recursive.
				Note that you are not allowed to call this method directly, or use any codes except defined by framework.
				To handle your own commands, override the ManagedObjectBase::ExecuteCommand method.
				Further details on service-to-client commands can be found \ref objmgr_cmds "here".
				\param CommandCode Specifies the command code to execute.
				\param Recursive Specifies whether a service that received a command should forward it to its clients.
				\return If your object returns an error code, the command will be aborted and no other objects
						will be notified. See SystemCommand definition for details on various command types.
			*/
			virtual ActionStatus ExecuteSystemCommand(SystemCommand CommandCode, bool Recursive, void *pParam1 = NULL, void *pParam2 = NULL, void *pParam3 = NULL)
			{
				return MAKE_STATUS(NotSupported);
			}

			//! Service-to-client command handler.
			/*! This method is an overrideable handler for user service-to-client commands. You should use the
				ServiceObject::CallClients() method to send a command to the clients of a service.
				Further details on service-to-client commands can be found \ref objmgr_cmds "here".
			*/
			virtual ActionStatus ExecuteCommand(class ServiceObject *Caller, unsigned CommandCode, void *pParam1 = NULL, void *pParam2 = NULL, void *pParam3 = NULL) {return MAKE_STATUS(NotSupported);}

#ifdef _DEBUG
			//! Debug heap allocation operator
			/*! This operator allocates memory for all managed objects when the framwork is built in the debug mode.
				It initializes the memory with a special pattern: HEAP_ALLOCATION_SIGNATURE, that, in turn, allows
				constructors to distinguish between heap-allocated objects and static or stack-allocated ones. It also
				allows to determine whether a reference (or a managed pointer) is a part (field) of a managed object.
			*/
			void *operator new(size_t Size) throw()
			{
				ASSERT(!(Size % sizeof(unsigned)));
				unsigned *pData = new unsigned[Size / sizeof(unsigned)];
				if (!pData)
					return NULL;
				ASSERT(!((INT_PTR)pData % sizeof(unsigned)));	//Ensure proper buffer alignment
				for (unsigned i = 0; i < (Size / sizeof(unsigned)); i++)
					pData[i] = HEAP_ALLOCATION_SIGNATURE;
				return pData;
			}
#endif

			template <class _Interface> inline ManagedPointer<_Interface> QueryInterface(const GUID *pGUID, unsigned version);
		};

		class ManagedObjectWithWeakPointerSupport : public ManagedObjectBase
		{
		private:
			class WeakPointerExtension
			{
			private:
				AtomicUInt32 m_WeakReferenceCount;
				bool m_bObjectIsAlive;
				ObjectManagerMutex m_Lock;

			public:
				WeakPointerExtension(ManagedObjectBase *pObj)
					: m_bObjectIsAlive(true)
					, m_WeakReferenceCount(0)
				{
				}

			public:
				void Retain()
				{
					m_WeakReferenceCount++;
				}

				void Release()
				{
					if (!--m_WeakReferenceCount)
					{
						{
							ObjectManagerMutexLocker lck(m_Lock);
							if (m_bObjectIsAlive)
								return;	//Cannot delete the extension yet
						}
						delete this;
					}
				}

				operator bool()
				{
					return m_bObjectIsAlive;
				}

			public:
				//! Thread-safe w.r.t. ReferenceAndGetObject() and Release().
				bool TryDetachObject(ManagedObjectWithWeakPointerSupport *pObj)
				{
					ASSERT(m_bObjectIsAlive);
					ASSERT(pObj->m_pWeakPointerExtensionInternal == this);

					{
						ObjectManagerMutexLocker lck(m_Lock);
						if (pObj->m_ReferenceCount)	//ReferenceAndGetObject() has been called
							return false;

						m_bObjectIsAlive = false;				
						if (m_WeakReferenceCount)
							return true;	//Do not delete self. The extension object will be deleted when the last weak reference is removed.
					}

					delete this;
					return true;
				}

				//! Thread-safe w.r.t. TryDetachObject()
				template <class _ManagedClass> _ManagedClass *ReferenceAndGetObject(_ManagedClass *pObj)
				{
					if (!m_bObjectIsAlive)
						return NULL;
					ObjectManagerMutexLocker lck(m_Lock);
					if (!m_bObjectIsAlive)
						return NULL;
					ASSERT(pObj->m_pWeakPointerExtensionInternal == this);
					pObj->Retain();
					return pObj;
				}
			};
			
		private:
			//! Never use directly! Call GetWeakPointerExtension() instead;
			/*! Weak pointer extensions are used by weak pointers. Initially this field is 0. As soon as at least 1 weak pointer
				is created, it becomes initializes and stays initialized until the object is deleted.
			*/
			WeakPointerExtension *m_pWeakPointerExtensionInternal;	
			template <class _ManagedClass> friend class WeakManagedPointer;

		protected:
			ObjectManagerMutex m_Lock;

		public:
			ManagedObjectWithWeakPointerSupport()
				: m_pWeakPointerExtensionInternal(NULL)
			{
			}

			~ManagedObjectWithWeakPointerSupport()
			{
				ASSERT(!m_pWeakPointerExtensionInternal);
			}

			//! Increments the reference counter.
			/*! This method increments the reference counter of a managed object. It is recommended to use
				the ManagedPointer class instead of calling Retain()/Release() directly.
			*/
			unsigned Retain()
			{
				return ++m_ReferenceCount;
			}

			//! Decrements the reference counter.
			/*! This method decrements the reference counter of a managed object. When reference counter reaches
				zero, the object is deleted. It is recommended to use the ManagedPointer class instead of calling
				Retain()/Release() directly.
			*/
			unsigned Release()
			{
				register int r = --m_ReferenceCount;
				if (!r)
				{
					if (m_pWeakPointerExtensionInternal)
						if (!m_pWeakPointerExtensionInternal->TryDetachObject(this))
							return 1;

					m_pWeakPointerExtensionInternal = NULL;
					delete this;
				}
				return r;
			}

			WeakPointerExtension *GetWeakPointerExtension()
			{
				ObjectManagerMutexLocker lck(m_Lock);
				if (!m_pWeakPointerExtensionInternal)
					m_pWeakPointerExtensionInternal = new WeakPointerExtension(this);
				return m_pWeakPointerExtensionInternal;
			}		
		};

		//! Represents a service (an object that may have clients)
		/*! This class represents a service object - a special managed object that can have client objects connected
			to it. This is done via RegisterClient()/RemoveClient() calls.
			For details on managed objects and services, see \ref objmgr "this article".
		*/
		class ServiceObject : public ManagedObjectWithWeakPointerSupport
		{
		private:
			std::set<ManagedObjectBase *> m_Children;

			template <class _ServiceType> friend class ParentServiceReference;
			template <class _ServiceType> friend class IntermediateServiceObject;

			template <class _ManagedType, class _InternalCollection> friend class ManagedCollectionBase;

		private:
			//! Registers a client for a service
			/*! This method registers a client for a service, that, in turn, increases the reference counter.
				\param pObject Specifies the client. This parameter cannot be NULL.
				\remarks You should never call this method directly. Use the DECLARE_REFERENCE() mechanism instead.
			*/
			void RegisterClient(ManagedObjectBase *pObject)
			{
				m_Lock.Lock();
				ASSERT(pObject != NULL);
				ASSERT(m_Children.find(pObject) == m_Children.end());
	#ifdef _DEBUG
				ASSERT(pObject->ExecuteSystemCommand(FindCircularReferences, true, (ManagedObjectBase *)this).Successful());
	#endif
				m_Children.insert(pObject);
				m_Lock.Unlock();

				Retain();
			}

			//! Deregisters a client for a service
			/*! This method deregisters a client for a service, that, in turn, decreases the reference counter.
				\param pObject Specifies the client. This parameter cannot be NULL.
				\remarks You should never call this method directly. Use the DECLARE_REFERENCE() mechanism instead.
			*/
			void RemoveClient(ManagedObjectBase *pObject)
			{
				m_Lock.Lock();
				ASSERT(m_Children.find(pObject) != m_Children.end());
				m_Children.erase(pObject);
				m_Lock.Unlock();
				Release();
			}

		public:
			//! Deletes the ServiceObject instance.
			/*! This destructor deletes a ServiceObject instance. Note that managed objects should only be freed by calling
				the ManagedObject::Release() method. Direct call of the <b>delete</b> operator is not allowed. This
				ensures that the object has no clients when it is deleted. The debug build of the framework checks this.
			*/
			~ServiceObject()
			{
				ASSERT(!m_Children.size());
			}

			//! Sends a command to all clients of a service.
			/*! This method is used to send some command to all of the clients of a particular service. It invokes
				the ManagedObjectBase::ExecuteCommand() method. Note that the set of the commands that can be sent
				by a service depends on that service type. That way, you can define any commands you wish for your
				services. If you want a service to forward such command to its clients (note that the clients should
				be aware of such command, i.e. it should reside in the list of the commands sent by your service),
				simply override the ManagedObjectBase::ExecuteCommand() method and call CallClients() from there.
				\param CommandCode Specifies the command code to send
				\param IgnoreNotSupported Specifies whether a NotSupported code should be treated as
						\link BazisLib::CommonErrorType Success\endlink.
				\return This method return \link BazisLib::CommonErrorType Success\endlink if all the method returned
						\link BazisLib::CommonErrorType Success\endlink (if <b>IgnoreNotSupported</b>
						parameter is set, the \link BazisLib::CommonErrorType NotSupported \endlink status code
						is also treated as \link BazisLib::CommonErrorType Success\endlink). If one of the
						clients returned an error code, the method terminates and returns that code.
			*/
			ActionStatus CallClients(unsigned CommandCode, bool IgnoreNotSupported, void *pParam1 = NULL, void *pParam2 = NULL, void *pParam3 = NULL)
			{
				std::set<ManagedObjectBase *> children;
				{
					ObjectManagerMutexLocker lck(m_Lock);
					children = m_Children;
				}

				for (std::set<ManagedObjectBase *>::iterator it = children.begin(); it != children.end(); it++)
				{
					ActionStatus status = (*it)->ExecuteCommand(this, CommandCode, pParam1, pParam2, pParam3);
					if (IgnoreNotSupported && (status == NotSupported))
						continue;
					if (!status.Successful())
						return status;
				}
				return MAKE_STATUS(Success);
			}


		public:
			//! System command handler.
			/*! See ManagedObjectBase::ExecuteSystemCommand() for details.
			*/
			virtual ActionStatus ExecuteSystemCommand(SystemCommand CommandCode, bool Recursive, void *pParam1 = NULL, void *pParam2 = NULL, void *pParam3 = NULL) override
			{
				switch (CommandCode)
				{
	#ifdef _DEBUG
				case FindCircularReferences:
					{
						ObjectManagerMutexLocker lck(m_Lock);
						if (m_Children.find((ManagedObjectBase *)pParam1) != m_Children.end())
						{
							//We are trying to register a client for a service that is already a client for our client.
							//Such circular paths are not allowed by design. See call stack for details.
							ASSERT(!"Circular references not allowed");
							return MAKE_STATUS(UnknownError);
						}
					}
					break;
	#else
				case InvalidCommand:
	#endif
				default:
					return MAKE_STATUS(NotSupported);
				}
				if (Recursive)
				{
					ObjectManagerMutexLocker lck(m_Lock);
					for (std::set<ManagedObjectBase *>::iterator it = m_Children.begin(); it != m_Children.end(); it++)
					{
						ActionStatus status = (*it)->ExecuteSystemCommand(CommandCode, Recursive, pParam1, pParam2, pParam3);
						if (!status.Successful())
							return status;
					}
				}
				return MAKE_STATUS(Success);
			}
		};

		//! Represents a simple intermediate service
		/*! A simple intermediate service is a service that is a client one and only one service.
			Such 'service-for-service' is called a parent service. This class allows automatic
			registration/deregistration within a parent service. To get the parent service pointer,
			use the IntermediateServiceObject::GetParentService() pointer.
			\param _ServiceType Specifies the parent service type.
		*/
		template <class _ServiceType> class IntermediateServiceObject : public ServiceObject
		{
		private:
			_ServiceType *m_pService;

		public:
			IntermediateServiceObject(_ServiceType *pObject) :
				m_pService(pObject)
			{
				ASSERT(m_pService);
				m_pService->RegisterClient(this);
			}

			~IntermediateServiceObject()
			{
				m_pService->RemoveClient(this);
			}

			_ServiceType *GetParentService()
			{
				return m_pService;
			}
		};

		template <class _ManagedType> class ManagedPointer;
		template <class _ServiceType> class ParentServiceReference;

		//! Represents a read-only managed pointer for function parameters. Should only be used for creating references/pointers.
		/*! This class represents a read-only managed pointer and serves as a base for the ParentServiceReference
			and ManagedPointer template classes. The typical usage scheme for this class is the following: 
<pre>
void SomeFunctionOrMethod(const ConstManagedPointer &pObj)
{
	...
}
</pre>
			You can also use ManagedPointer class references as function parameters, however in case of passing
			a ParentServiceReference to such a function, a temporary ManagedPointer instance will be created,
			that will result in an extra Retain()/Release() call pair. That way, it is highly recommended to use
			this class in function/method parameters, as it is the common base for both ManagedPointer and
			ParentServiceReference. However, it does not provide any other logic except for querying operators.
			\attention This class has non-working private constructors. They allow to ensure that no instance
					   of this class will be created directly (except for a part of a friend child class).
					   Also, this class does not contain any freeing/releasing logic. That way, DO NOT create
					   instances of this class directly.
		*/
		template <class _ServiceType> class ConstManagedPointer
		{
		private:
			_ServiceType *m_pTarget;

		private:
			//This constructors do not initialize m_pTarget to support EnsureCorrectAllocation() in children
			ConstManagedPointer() {}
			ConstManagedPointer(const ConstManagedPointer &ptr) {}
			~ConstManagedPointer() {}

			friend class ManagedPointer<_ServiceType>;
			friend class ParentServiceReference<_ServiceType>;
			template<class _CompatibleType> friend class ManagedPointer;
			template<class _CompatibleType> friend class WeakManagedPointer;

		public:
			operator bool()
			{
				return (m_pTarget != NULL);
			}

			operator bool() const
			{
				return (m_pTarget != NULL);
			}

			//! Retains the object and returns an unmanaged pointer
			/*! As we have declared restrictions for unmanaged pointers (if a function received an unmanaged
				pointer, it should release it after usage, or, actually, initialize a managed one and forget
				the unmanaged), we do not allow direct casting from managed to unmanaged pointer. However, if
				an unmanaged pointer is required, it can be obtaied by this method.
				\returns This method returns a retained unmanaged pointer to an object (or NULL). Be sure to
						 release this pointer by calling Release() method, or initialize a managed pointer
						 using it.
			*/
			_ServiceType* RetainAndGet() const
			{
				if (m_pTarget)
					m_pTarget->Retain();
				return m_pTarget;
			}

			const _ServiceType &operator*() const
			{
				return *m_pTarget;
			}

			_ServiceType* operator->() const
			{
				return m_pTarget;
			}

			bool operator != (const ConstManagedPointer &right)
			{
				return m_pTarget != right.m_pTarget;
			}

			bool operator == (const ConstManagedPointer &right)
			{
				return m_pTarget == right.m_pTarget;
			}
		};


		//! Represents a managed pointer field
		/*! This template class is used to implement a managed pointer to a service that is stored as
			a field of the client class. For about <b>Service</b> and <b>Client</b> definitions, see the
			BazisLib::ObjectManager documentation.
			The basic idea for service references is to allow automatic reference counting and circular
			reference chain detection. Such reference establishes a link between the object that contains
			the reference (source) and the target. The source then becomes a client for the target. This means,
			that the target will never be deleted until the source is deleted. This also means that the target
			cannot become a client for the source. The ServiceObject::RegisterClient() method performs
			such check in the debug build. As this check requires additional time to be performed, it is not
			done in release version.

			\param _ServiceType Specifies the service type to reference.
			\remarks <b>Never use this class directly in declarations.</b> Instead, use the following macros:
				<ul>
					<li>Use DECLARE_REFERENCE() macro to declare a reference.</li>
					<li>Use INIT_REFERENCE() macro to initialize a reference from a constructor.</li>
					<li>Use ZERO_REFERENCE() macro to initialize an empty (zeroed) reference.</li>
					<li>Use COPY_REFERENCE() macro to copy a reference inside a copy constructor.</li>
				</ul>
		*/

		template <class _ServiceType> class ParentServiceReference : public ConstManagedPointer<_ServiceType>
		{
		private:
			ManagedObjectBase *m_pObject;
			typedef ConstManagedPointer<_ServiceType> _Base;

			template <class _ManagedType> friend class ManagedPointer;

			using _Base::m_pTarget;

		private:
			//! Copy constructor is not allowed for parent service reference
			/*! When a parent service reference is being copied, it means that the object containing the
				parent service reference is being copied. In that case, the new reference should contain
				an updated object pointer. This can be ensured by using a constructor that contains both
				old reference and new object parameters.
			*/
			ParentServiceReference(const ParentServiceReference &ref)
			{
				ASSERT(FALSE);
			}

			//! Ensures that the object is a part of a heap-allocated managed object.
			void EnsureCorrectAllocation()
			{
#ifdef _DEBUG
				ASSERT(!((INT_PTR)this % sizeof(unsigned)));	//Ensure proper buffer alignment
				//WARNING! If (somehow) this class gets a vtable, this[0] should be changed to this[1]!
				if (((unsigned *)this)[0] != ManagedObjectBase::HEAP_ALLOCATION_SIGNATURE)
				{
					//Oops! You tried to create a parent service reference somewhere out of a managed object.
					//See class reference for details.
					ASSERT(!"Class references should be the fields of managed objects!");
				}
#endif
			}

		private:
			//Implicit conversion is prohibited. To retrieve a unmanaged pointer, call the RetainAndGet() method
			operator _ServiceType *()
			{
				ASSERT(FALSE);
				return NULL;
			}

			//! Sets a new value for a parent service reference
			/*! This method is internally used by the assignment operator
				\param pObj Object to assign
				\param Release If this parameter is <b>true</b>, the pointer will be released once
							   after client registration. It is set to <b>true</b> only when an operator
							   received an unmanaged pointer from user.
			*/
			ParentServiceReference &Assign(_ServiceType *pObj, bool Release)
			{
				if (m_pTarget)
					m_pTarget->RemoveClient(m_pObject);
				m_pTarget = pObj;
				if (m_pTarget)
				{
					m_pTarget->RegisterClient(m_pObject);
					if (Release)
						m_pTarget->Release();
				}
				return *this;
			}

		public:
			ParentServiceReference(ManagedObjectBase *pObject, _ServiceType *pTarget)
			  //Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
			  //we initialize our members.
			{
				EnsureCorrectAllocation();
				m_pObject = pObject;
				m_pTarget = pTarget;
				ASSERT(m_pObject);
				if (m_pTarget)
				{
					m_pTarget->RegisterClient(m_pObject);
					//Once we registered within the service, we may simply release our unmanaged pointer
					m_pTarget->Release();
				}
			}

			ParentServiceReference(ManagedObjectBase *pObject, const ConstManagedPointer<_ServiceType> &pTarget)
			  //Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
			  //we initialize our members.
			{
				EnsureCorrectAllocation();
				m_pObject = pObject;
				m_pTarget = pTarget.m_pTarget;
				ASSERT(m_pObject);
				if (m_pTarget)
					m_pTarget->RegisterClient(m_pObject);
			}


			~ParentServiceReference()
			{
				if (m_pTarget)
					m_pTarget->RemoveClient(m_pObject);
			}

			ParentServiceReference &operator=(_ServiceType *pObj)
			{
				return Assign(pObj, true);
			}

			ParentServiceReference &operator=(const ConstManagedPointer<_ServiceType> &ref)
			{
				return Assign(ref.m_pTarget, false);
			}

			ParentServiceReference &operator=(const ParentServiceReference<_ServiceType> &ref)
			{
				return Assign(ref.m_pTarget, false);
			}

			_ServiceType* operator->() const
			{
				return m_pTarget;
			}

			_ServiceType &operator*() const
			{
				return *m_pTarget;
			}

			operator bool()
			{
				return (m_pTarget != NULL);
			}

			operator bool() const
			{
				return (m_pTarget != NULL);
			}
		};

		//! Used to declare managed-to-managed references
		#define AUTO_REFERENCE(type) BazisLib::ObjectManager::ParentServiceReference<type>

		//! Declares a managed field reference. See \ref objmgr_fieldp "this article" for details & usage example.
		#define DECLARE_REFERENCE(type, name) AUTO_REFERENCE(type) name
		//! Initializes a managed field reference. Should be used from a member initialization list. See \ref objmgr_fieldp "this article" for details & usage example.
		#define INIT_REFERENCE(member, value) member(this, value)
		//! Initializes a managed field reference with NULL pointer. Should be used from a member initialization list. See \ref objmgr_fieldp "this article" for details & usage example.
		#define ZERO_REFERENCE(member)		  member(this, NULL)
		//! Copies a managed field reference. Should be used from a member initialization list. See \ref objmgr_fieldp "this article" for details & usage example.
		#define COPY_REFERENCE(member, source) member(this, source.member)

		/*! To declare an auto interface, use the following syntax:
			<pre>class AIxxx : AUTO_INTERFACE { ... }</pre>
		*/
		#define AUTO_INTERFACE	public virtual BazisLib::ObjectManager::ServiceObject

		/*! To declare an auto object, use the following syntax:
			<pre>class AIxxx : AUTO_OBJECT, ... { ... }</pre>
			If an auto object implements one or more auto interfaces,
			the usage of AUTO_OBJECT is optionsl.
		*/
		#define AUTO_OBJECT		public virtual BazisLib::ObjectManager::ServiceObject

		/*! Use this macro to declare an auto interface that copies an existing unmanaged interface.
			The syntax is the following: <pre>
class Ixxx
{
...
};

MAKE_AUTO_INTERFACE(Ixxx);
			</pre>
		*/
		#define MAKE_AUTO_INTERFACE(iface) class A ## iface : public iface, AUTO_INTERFACE {}

//! Represents a managed pointer to an object.
		/*! This class represents a managed pointer to an object capable of dealing with the reference count.
			\remarks Avoid declaring ManagedPointer fields in managed objects. Use the DECLARE_REFERENCE() instead.
					 Note that the debug build of the framework detects such problems.
		*/
		template <class _ManagedType> class ManagedPointer : public ConstManagedPointer<_ManagedType>
		{
		private:
			template<class _ServiceType> friend class ParentServiceReference;
			typedef ConstManagedPointer<_ManagedType> _Base;
			using _Base::m_pTarget;

			//! Ensures that the object is NOT a part of a heap-allocated managed object.
			void EnsureCorrectAllocation()
			{
#ifdef _DEBUG
				ASSERT(!((INT_PTR)this % sizeof(unsigned)));	//Ensure proper buffer alignment
				//WARNING! If (somehow) this class gets a vtable, this[0] should be changed to this[1]!
				if (((unsigned *)this)[0] == ManagedObjectBase::HEAP_ALLOCATION_SIGNATURE)
				{
					//Oops! You tried to create a parent service reference somewhere out of a managed object.
					//See class reference for details.
					ASSERT(!"Class references should be the fields of managed objects!");
				}
#endif
			}

		private:
			operator _ManagedType *()
			{
				return m_pTarget;
			}

			//! Sets a new value for a parent service reference
			/*! This method is internally used by the assignment operator
				\param pObj Object to assign
				\param Release If this parameter is <b>true</b>, the pointed object will not be retained.
							   It is set to <b>true</b> only when an operator received an unmanaged pointer from user.
			*/
			ManagedPointer<_ManagedType> &Assign(_ManagedType *pObj, bool Release)
			{
				if (m_pTarget)
					m_pTarget->Release();
				m_pTarget = pObj;
				if (m_pTarget)
				{
					if (!Release)
						m_pTarget->Retain();
				}
				return *this;
			}

		public:
			ManagedPointer(_ManagedType *pTarget = NULL)
			  //Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
			  //we initialize our members.
			{
				EnsureCorrectAllocation();
				m_pTarget = pTarget;
			}

			ManagedPointer(const ManagedPointer &ref)
			  //Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
			  //we initialize our members.
			{
				m_pTarget = ref.m_pTarget;
				EnsureCorrectAllocation();
				if (m_pTarget)
					m_pTarget->Retain();
			}


			ManagedPointer(const ConstManagedPointer<_ManagedType> &ref)
			  //Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
			  //we initialize our members.
			{
				m_pTarget = ref.m_pTarget;
				EnsureCorrectAllocation();
				if (m_pTarget)
					m_pTarget->Retain();
			}

			template<class _CompatibleType> friend class ManagedPointer;

			template<class _CompatibleType> ManagedPointer(const ManagedPointer<_CompatibleType> &ref)
				//Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
				//we initialize our members.
			{
				m_pTarget = static_cast<_ManagedType *>(ref.m_pTarget);
				EnsureCorrectAllocation();
				if (m_pTarget)
					m_pTarget->Retain();
			}


			template<class _CompatibleType> ManagedPointer(const ConstManagedPointer<_CompatibleType> &ref)
				//Avoid using initializer list here, as we need to call EnsureCorrectAllocation() BEFORE
				//we initialize our members.
			{
				m_pTarget = static_cast<_ManagedType *>(ref.m_pTarget);
				EnsureCorrectAllocation();
				if (m_pTarget)
					m_pTarget->Retain();
			}

			~ManagedPointer()
			{
				if (m_pTarget)
					m_pTarget->Release();
			}

			ManagedPointer<_ManagedType> &operator=(_ManagedType *pObj)
			{
				return Assign(pObj, true);
			}

			//Both ManagedPointer- and ConstManagedPointer-related operators are required.
			//Otherwise direct copying will be used by compiler
			ManagedPointer<_ManagedType> &operator=(const ManagedPointer &pObj)
			{
				return Assign(pObj.m_pTarget, false);
			}

			ManagedPointer<_ManagedType> &operator=(const ConstManagedPointer<_ManagedType> &pObj)
			{
				return Assign(pObj.m_pTarget, false);
			}

			_ManagedType* operator->() const
			{
				return m_pTarget;
			}

			operator bool()
			{
				return (m_pTarget != NULL);
			}

			operator bool() const
			{
				return (m_pTarget != NULL);
			}

			bool operator ==(const ManagedPointer &right) const
			{
				return m_pTarget == right.m_pTarget;
			}

			bool operator !=(const ManagedPointer &right) const
			{
				return m_pTarget != right.m_pTarget;
			}

			bool operator <(const ManagedPointer &right) const
			{
				return m_pTarget < right.m_pTarget;
			}
		};

		template <class _ManagedType> class WeakManagedPointer
		{
		private:
			ManagedObjectWithWeakPointerSupport::WeakPointerExtension *m_pExtension;
			_ManagedType *m_pTarget;

		public:
			WeakManagedPointer()
			{
				m_pExtension = NULL;
				m_pTarget = NULL;
			}

			WeakManagedPointer(_ManagedType *pObj)
				: m_pExtension(NULL)
				, m_pTarget(NULL)
			{
				assign(pObj);
				if (pObj)
					pObj->Release();
			}

			~WeakManagedPointer()
			{
				if (m_pExtension)
					m_pExtension->Release();
			}

			WeakManagedPointer(const ConstManagedPointer<_ManagedType> &ptr)
				: m_pExtension(NULL)
				, m_pTarget(NULL)
			{
				assign(ptr.m_pTarget);
			}

			WeakManagedPointer(const WeakManagedPointer &_right)
			{
				m_pExtension = _right.m_pExtension;
				m_pTarget = _right.m_pTarget;
				if (m_pExtension)
					m_pExtension->Retain();
			}

			WeakManagedPointer &operator=(const ConstManagedPointer<_ManagedType> &ptr)
			{
				assign(ptr.m_pTarget);
				return *this;
			}

			WeakManagedPointer &operator=(const WeakManagedPointer<_ManagedType> &ptr)
			{
				assign(ptr.m_pTarget);
				return *this;
			}

			WeakManagedPointer &operator=(_ManagedType *pObj)
			{
				assign(pObj);
				if (pObj)
					pObj->Release();
				return *this;
			}

			operator bool()
			{
				return m_pExtension && *m_pExtension;
			}

		private:
			void assign(_ManagedType *pObj)
			{
				if (m_pExtension)
					m_pExtension->Release();
				if (pObj == NULL)
				{
					m_pExtension = NULL;
					m_pTarget = NULL;
				}
				else
				{
					m_pExtension = pObj->GetWeakPointerExtension();
					if (m_pExtension)
						m_pExtension->Retain();
					m_pTarget = pObj;
				}
			}

		public:
			operator ManagedPointer<_ManagedType>()
			{
				if (!m_pExtension || !m_pTarget)
					return NULL;
				return ManagedPointer<_ManagedType>(m_pExtension->ReferenceAndGetObject(m_pTarget));
			}

		};

		//!Provides type casting for managed pointers
		/*! Use the managed_cast template function to cast from one managed pointer type to another.
			Example: <pre>
class A
{
	...
};

class B : public A
{
	...
};

...
{
	ManagedPointer<A> ptr = new A();
	ManagedPointer<B> ptr2 = managed_cast<B>(ptr);
}
</pre>
		*/
		template<class _DestType, class _SourceType> static inline ManagedPointer<_DestType> managed_cast(const ConstManagedPointer<_SourceType> &ptr)
		{
			return ManagedPointer<_DestType>(static_cast<_DestType*>(ptr.RetainAndGet()));
		}
	
		template <class _Interface> inline ManagedPointer<_Interface> ManagedObjectBase::QueryInterface(const GUID *pGUID, unsigned version)
		{
			_Interface *pIface = NULL;
			if(!ExecuteSystemCommand(bzsQueryInterface, false, (void *)pGUID, (void *)version, (void *)&pIface).Successful())
			{
				delete pIface;
				pIface = NULL;
			}
			return pIface;
		}

	}

	using namespace ObjectManager;
}