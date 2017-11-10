#ifndef __MINIFILTER_UTILS__
#define	__MINIFILTER_UTILS__

#include "MyMiniEncrypt.h"

//��ȡ������ƫ��
ULONG GetProcessNameOffset(VOID);

//��ȡ��������
PCHAR GetCurrentProcessName(ULONG ProcessNameOffset);

//��ȡ�ļ�������Ϣ                               
NTSTATUS MyGetFileEncryptInfoToCtx(__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__inout PSTREAM_HANDLE_CONTEXT ctx);
//	__in PTYPE_KEY_WORD keyWord);

BOOLEAN IsInEncryptList(PUNICODE_STRING file_name);


//����ļ���������                               
void cfFileCacheClear(PFILE_OBJECT pFileObject);

NTSTATUS
EncryptFile(__inout PFLT_CALLBACK_DATA Data,__in PCFLT_RELATED_OBJECTS FltObjects,__in PCHAR key);

void EncryptData(__in PVOID buff, __in PVOID outbuff, __in LONGLONG offset, __in ULONG len, PCHAR key);

void DecodeData(__in PVOID buff, __in PVOID outbuff, __in LONGLONG offset, __in ULONG len, PCHAR key);


#endif