#pragma once
#include "cmndef.h"

namespace BazisLib
{
	namespace Win32
	{
		typedef bool(*copyfunc)(void *src, const  void *dst, unsigned size, void *pContext);

		namespace 
		{
			inline bool _simplecpy(void *src, const  void *dst, unsigned size, void *pContext)
			{
				memcpy(src, dst, size);
				return true;
			}

			typedef void *(*ptr_LocateImportsSection)(void *pBase, void *pContext, int *pSize);
		}

		//! Allows patching import table of both local and remote processes
		/*! This class is used to patch image import table after the image is
			loaded to memory. It allows analyzing PE headers, finding the
			import table pointers, and modifying them.
		*/
		template <ptr_LocateImportsSection _LocateImportsSectionFunc = NULL, copyfunc rdfunc = _simplecpy, copyfunc wrfunc = _simplecpy> class ImportTablePatcher
		{
		private:
			char *m_pAddr;
			void *m_pContext;

			typedef struct _IMAGE_DATA_DIRECTORY {
				DWORD   RVA;
				DWORD   Size;
			} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;


		public:
			class ImportTable
			{
			private:
				char *m_pImageAddr;
				char *m_pImportAddr;
				void *m_pContext;

			protected:
				struct LibraryEntry
				{
					unsigned rvaImportLookupTable;
					unsigned DateTime;
					unsigned ForwarderChain;
					unsigned rvaName;
					unsigned rvaImportAddrTable;
				};

			public:
				typedef char *LibraryRecord;

			protected:
				void **LookupAddressEntry(LibraryRecord rec, const char *pNameFunction)
				{
					if (!rec || !pNameFunction)
						return NULL;
					LibraryEntry entry;
					memset(&entry, 0, sizeof(entry));
					rdfunc(&entry, rec, sizeof(LibraryEntry), m_pContext);
					if (!entry.rvaImportAddrTable || !entry.rvaName || !entry.rvaImportLookupTable)
						return NULL;
					unsigned ptr;
					char *pLookupTable = m_pImageAddr + entry.rvaImportLookupTable;
					for (int i = 0; ;i++)
					{
						ptr = 0;
						rdfunc(&ptr, pLookupTable + i * 4, 4, m_pContext);
						if (!ptr)
							break;
						if (ptr & 0x80000000)
							continue;
						char *pName = m_pImageAddr + ptr + 2;
						if (!_stricmp(pName, pNameFunction))
							return (void **)((m_pImageAddr + entry.rvaImportAddrTable) + i * 4);
					}
					return NULL;
				}

			public:
				typedef char *LibraryRecord;

				ImportTable(char *pImageAddr = NULL, char *pImportAddr = NULL, void *pContext = NULL) :
				  m_pImageAddr(pImageAddr),
				  m_pImportAddr(pImportAddr),
				  m_pContext(pContext)
				{
				}

				bool Valid()
				{
					return m_pImageAddr && m_pImportAddr;
				}

				LibraryRecord FindLibraryRecord(const char *pLibraryName)
				{
					if (!pLibraryName)
						return NULL;
					LibraryEntry entry;
					for (int i = 0;;i++)
					{
						memset(&entry, 0, sizeof(entry));
						rdfunc(&entry, m_pImportAddr + i * sizeof(LibraryEntry), sizeof(LibraryEntry), m_pContext);
						if (!entry.rvaImportAddrTable || !entry.rvaName || !entry.rvaImportLookupTable)
							break;
						char *pName = m_pImageAddr + entry.rvaName;
						if (!_stricmp(pName, pLibraryName))
							return m_pImportAddr + i * sizeof(LibraryEntry);
					}
					return NULL;
				}

				void *GetFunctionAddress(LibraryRecord rec, const char *pNameFunction)
				{
					void **pp = LookupAddressEntry(rec, pNameFunction);
					if (!pp)
						return NULL;
					void *pRet = NULL;
					rdfunc(&pRet, pp, sizeof(void *), m_pContext);
					return pRet;
				}

				bool SetFunctionAddress(LibraryRecord rec, const char *pNameFunction, void *pAddr)
				{
					void **pp = LookupAddressEntry(rec, pNameFunction);
					if (!pp)
						return false;
					wrfunc(pp, &pAddr, sizeof(void *), m_pContext);
					return true;
				}

			};

		protected:
		#pragma region Object search routines
			char *LocatePEHeader()
			{
				char sign[4];
				unsigned ptr = 0;
				rdfunc(sign, m_pAddr, 2, m_pContext);
				if ((sign[0] != 'M') || (sign[1] != 'Z'))
					return NULL;
				//Read the MZ-to-PE pointer value (offset of the real PE header)
				rdfunc(&ptr, m_pAddr + 0x3C, 4, m_pContext);
				if (!ptr)
					return NULL;
				rdfunc(sign, m_pAddr + ptr, 4, m_pContext);
				if ((sign[0] != 'P') || (sign[1] != 'E') || sign[2] || sign[3])
					return NULL;
				return m_pAddr + ptr;
			}

			char *LocateSectionList()
			{
				short sptr = 0;
				char *pPE = LocatePEHeader();
				if (!pPE)
					return NULL;
				rdfunc(&sptr, pPE + 4 + 16, 2, m_pContext);
				if (!sptr)
					return NULL;
				return pPE + 4 + 20 + sptr;
			}

			char *LocateObjectDirectory(int *pSectionCount)
			{
				if (!pSectionCount)
					return NULL;
				int cnt = 0;
				char *pPE = LocatePEHeader();
				if (!pPE)
					return NULL;
				char *pAdvHdr = pPE + 4 + 20;
				rdfunc(&cnt, pAdvHdr + 92, 4, m_pContext);
				if (!cnt)
					return NULL;
				*pSectionCount = cnt;
				return pAdvHdr + 96;
			}

			ImportTable LocateImportsSection(int *pSize = NULL)
			{
				int objectCount = 0;
				char *pObjDir = LocateObjectDirectory(&objectCount);
				if (!pObjDir)
					return ImportTable();
				IMAGE_DATA_DIRECTORY dir = {0,0};
				rdfunc(&dir, pObjDir + 1 * sizeof(IMAGE_DATA_DIRECTORY), sizeof(IMAGE_DATA_DIRECTORY), m_pContext);
				if (!dir.RVA || !dir.Size)
					return ImportTable();
				if (pSize)
					*pSize = dir.Size;
				return ImportTable(m_pAddr, m_pAddr + dir.RVA, m_pContext);
			}

		#pragma endregion
		public:
			ImportTablePatcher(void *pAddr, void *pContext) :
			  m_pAddr((char *)pAddr),
			  m_pContext(pContext)
			{
			}

			LPVOID QueryImportedFunction(const char *pszLibName, const char *pszFuncName)
			{
				if (!pszLibName || !pszFuncName)
					return NULL;
				ImportTable imp = (!_LocateImportsSectionFunc) ? LocateImportsSection() : ImportTable(m_pAddr, (char *)_LocateImportsSectionFunc(m_pAddr, m_pContext, NULL), m_pContext);
				if (!imp.Valid())
					return NULL;
				ImportTable::LibraryRecord rec = imp.FindLibraryRecord(pszLibName);
				if (!rec)
					return NULL;
				return imp.GetFunctionAddress(rec, pszFuncName);
			}

			LPVOID PatchImportedFunction(const char *pszLibName, const char *pszFuncName, LPVOID pNewPointer)
			{
				if (!pszLibName || !pszFuncName || !pNewPointer)
					return NULL;
				ImportTable imp = (!_LocateImportsSectionFunc) ? LocateImportsSection() : ImportTable(m_pAddr, (char *)_LocateImportsSectionFunc(m_pAddr, m_pContext, NULL), m_pContext);
				if (!imp.Valid())
					return NULL;
				ImportTable::LibraryRecord rec = imp.FindLibraryRecord(pszLibName);
				if (!rec)
					return NULL;
				void *pFunc = imp.GetFunctionAddress(rec, pszFuncName);
				if (!pFunc)
					return NULL;
				if (!imp.SetFunctionAddress(rec, pszFuncName, pNewPointer))
					return NULL;
				return pFunc;
			}
		};
	}
}