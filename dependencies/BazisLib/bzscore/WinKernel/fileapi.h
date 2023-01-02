#pragma once

#pragma region Imported FS-related functions
#ifndef _NTIFS_INCLUDED_
extern "C"
NTSTATUS 
  ZwQueryDirectoryFile(
    IN HANDLE  FileHandle,
    IN HANDLE  Event  OPTIONAL,
    IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
    IN PVOID  ApcContext  OPTIONAL,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    OUT PVOID  FileInformation,
    IN ULONG  Length,
    IN FILE_INFORMATION_CLASS  FileInformationClass,
    IN BOOLEAN  ReturnSingleEntry,
    IN PUNICODE_STRING  FileName  OPTIONAL,
    IN BOOLEAN  RestartScan
    );

extern "C" 
NTSYSAPI
NTSTATUS 
NTAPI
ZwDeviceIoControlFile(IN HANDLE  FileHandle,
					  IN HANDLE  Event,
					  IN PIO_APC_ROUTINE  ApcRoutine,
					  IN PVOID  ApcContext,
					  OUT PIO_STATUS_BLOCK  IoStatusBlock,
					  IN ULONG  IoControlCode,
					  IN PVOID  InputBuffer,
					  IN ULONG  InputBufferLength,
					  OUT PVOID  OutputBuffer,
					  IN ULONG  OutputBufferLength
					  );

typedef struct _FILE_RENAME_INFORMATION {
	BOOLEAN ReplaceIfExists;
	HANDLE RootDirectory;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
  ULONG  NextEntryOffset;
  ULONG  FileIndex;
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  LARGE_INTEGER  EndOfFile;
  LARGE_INTEGER  AllocationSize;
  ULONG  FileAttributes;
  ULONG  FileNameLength;
  ULONG  EaSize;
  WCHAR  FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;
#endif
#pragma endregion