/*++

Module Name:

    MyMiniEncrypt.c

Abstract:

    This is the main module of the MyMiniEncrypt miniFilter driver.

Environment:

    Kernel mode

--*/
#include "MyMiniEncrypt.h"
#include "Utils.h"

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

ULONG gTraceFlags = 0;

//��������ƫ��
ULONG  ProcessNameOffset = 0;

//�ӽ�������
CHAR key[KEY_MAX_LEN] = { "zheshimima" };

//ȫ�ֿ���
BOOLEAN IS_SYSTEM_OPEN = FALSE;

//The NPAGED_LOOKASIDE_LIST structure is an opaque structure that describes a lookaside list 
//of fixed - size buffers allocated from nonpaged pool.The system creates new entries and destroys 
//unused entries on the list as necessary.
//For fixed - size buffers, using a lookaside list is quicker than allocating memory directly.

NPAGED_LOOKASIDE_LIST Pre2PostContextList;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MyMiniEncryptUnload)
#pragma alloc_text(PAGE, MyMiniEncryptInstanceQueryTeardown)
#pragma alloc_text(PAGE, MyMiniEncryptInstanceSetup)
#pragma alloc_text(PAGE, MyMiniEncryptInstanceTeardownStart)
#pragma alloc_text(PAGE, MyMiniEncryptInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      MyPreCreate,
	  MyPostCreate },

	  { IRP_MJ_READ,
      0,
      MyPreRead,
      MyPostRead },

	  { IRP_MJ_WRITE,
	  0,
	  MyPreWrite,
	  MyPostWrite },

	  {IRP_MJ_CLEANUP,
	  0,
	  MyPreClose,
	  MyPostClose },

	  { IRP_MJ_QUERY_INFORMATION,
	  0,
	  MyPreQueryInformation,
	  MyPostQueryInformation },

	  { IRP_MJ_SET_INFORMATION,
	  0,
	  MyPreSetInformation,
	  MyPostSetInformation },

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    MyMiniEncryptUnload,                           //  MiniFilterUnload

    MyMiniEncryptInstanceSetup,                    //  InstanceSetup
    MyMiniEncryptInstanceQueryTeardown,            //  InstanceQueryTeardown
    MyMiniEncryptInstanceTeardownStart,            //  InstanceTeardownStart
    MyMiniEncryptInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
MyMiniEncryptInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    KdPrint(("[MyMiniEncrypt]:MyMiniEncryptInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}


NTSTATUS
MyMiniEncryptInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    KdPrint(("MyMiniEncrypt!MyMiniEncryptInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
MyMiniEncryptInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    KdPrint(("MyMiniEncrypt!MyMiniEncryptInstanceTeardownStart: Entered\n") );
}


VOID
MyMiniEncryptInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    KdPrint(("MyMiniEncrypt!MyMiniEncryptInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

	KdPrint(("[MyMiniEncrypt]:DriverEntry()."));

	//��ȡ��������ƫ��
	ProcessNameOffset = GetProcessNameOffset();

	//��ʼ��NPAGED_LOOKASIDE_LIST
	ExInitializeNPagedLookasideList(&Pre2PostContextList,
		NULL,
		NULL,
		0,
		sizeof(PRE_2_POST_CONTEXT),
		PRE_2_POST_TAG,
		0);

    //ע����������ص�����
    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //�����ļ�������
        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}

NTSTATUS
MyMiniEncryptUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

	KdPrint(("[MyMiniEncrypt]:MyMiniEncryptUnload()."));

    FltUnregisterFilter( gFilterHandle );

	ExDeleteNPagedLookasideList(&Pre2PostContextList);

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
MyMiniEncryptPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    KdPrint(("MyMiniEncrypt!MyMiniEncryptPreOperation: Entered\n") );

    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    if (MyMiniEncryptDoRequestOperationStatus( Data )) {

        status = FltRequestOperationStatusCallback( Data,
                                                    MyMiniEncryptOperationStatusCallback,
                                                    (PVOID)(++OperationStatusCtx) );
        if (!NT_SUCCESS(status)) {

			KdPrint(("MyMiniEncrypt!MyMiniEncryptPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
                           status) );
        }
    }

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MyMiniEncryptPostOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	KdPrint(("MyMiniEncrypt!MyMiniEncryptPostOperation: Entered\n"));

	return FLT_POSTOP_FINISHED_PROCESSING;
}


VOID
MyMiniEncryptOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );

    KdPrint(("MyMiniEncrypt!MyMiniEncryptOperationStatusCallback: Entered\n") );

	KdPrint(("MyMiniEncrypt!MyMiniEncryptOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)));
}


FLT_PREOP_CALLBACK_STATUS
MyMiniEncryptPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    KdPrint(("MyMiniEncrypt!MyMiniEncryptPreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
MyMiniEncryptDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}

/*************************************************************************
�Զ���ص�����
*************************************************************************/

FLT_PREOP_CALLBACK_STATUS
MyPreCreate(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	//ɶҲ����

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MyPostCreate(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	NTSTATUS status;

	PSTREAM_HANDLE_CONTEXT pCtx = NULL;
	STREAM_HANDLE_CONTEXT tempCtx;

	UNREFERENCED_PARAMETER(CompletionContext);

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}

	__try
	{
		//��ʼ������ʱ������
		tempCtx.isEncyptFile = IS_NOT_ENCRYPT_FILE;
		tempCtx.isEncypted = IS_NOT_ENCRYPTED;
//		temFileInfo.keyWord = NULL;

		//��ȡ�ļ��ļ�����Ϣ
		status = MyGetFileEncryptInfoToCtx(Data, FltObjects, &tempCtx);// , key_word_header);
		if (!NT_SUCCESS(status))
		{
			return retValue;
		}

		//������Ǽ������ͣ�ֱ�ӷŹ�
		if (tempCtx.isEncyptFile != IS_ENCRYPT_FILE)
		{
			return retValue;
		}

		//�������
		cfFileCacheClear(FltObjects->FileObject);

		//��鿪��
		if (!IS_SYSTEM_OPEN)
		{
			return retValue;
		}

		status = FltGetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, &pCtx);

		//��һ�δ��ļ�û���������ģ���Ҫ����
		if (!NT_SUCCESS(status))
		{
			//��ȡ�������ĵ�ַ
			status = FltAllocateContext(FltObjects->Filter,
				FLT_STREAMHANDLE_CONTEXT,
				sizeof(STREAM_HANDLE_CONTEXT),
				NonPagedPool, &pCtx);

			if (!NT_SUCCESS(status)) 
			{
				return retValue;
			}

			PFLT_CONTEXT oldCtx;
			//�����ļ������������
			status = FltSetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS, pCtx, &oldCtx);

			if (oldCtx != NULL)
			{
				pCtx = (PSTREAM_HANDLE_CONTEXT)oldCtx;
			}

			if (!NT_SUCCESS(status))
			{
				return retValue;
			}
		}

		//���ơ����ctx
		pCtx->isEncypted = tempCtx.isEncypted;
		pCtx->isEncyptFile = tempCtx.isEncyptFile;
//		ctx->keyWord = temFileInfo.keyWord;

		if (pCtx->isEncypted == IS_NOT_ENCRYPTED)
		{
			//����ǻ��ܽ��̣��򿪾ͼ���
			//��ȡ��������
			PCHAR procName = GetCurrentProcessName(ProcessNameOffset);

			//����ǻ��ܽ���������ļ�
			if (strncmp(procName, "notepad.exe", strlen(procName)) == 0)
			//if (IsSecretProcess(ctx->keyWord, procName))
			{
				//�����ļ�
				status = EncryptFile(Data, FltObjects, key);
				if (NT_SUCCESS(status))
				{
					pCtx->isEncypted = IS_ENCRYPTED;
					//DbgPrint("encrypt a file");
				}
				else
				{
					//DbgPrint("encrypt a file fail");
				}
				//����ļ�����
				//cfFileCacheClear(FltObjects->FileObject);
			}
		}

		//��ȡ�ļ�����Ϣ���ctx�����ݸ���������
		status = FltQueryInformationFile(FltObjects->Instance,
			Data->Iopb->TargetFileObject,
			&(pCtx->fileInfo),
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation, NULL);

		//�������
		cfFileCacheClear(FltObjects->FileObject);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("a exception happened in postCreate"));

		if (pCtx != NULL)
		{
			FltReleaseContext(pCtx);
		}
	}

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
MyPreRead(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	NTSTATUS status;
	PVOID newBuf = NULL;
	PMDL newMdl = NULL;
	PPRE_2_POST_CONTEXT p2pCtx = NULL;
	ULONG readLen = iopb->Parameters.Read.Length;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}

	__try
	{
		//��鿪��
		if (IS_SYSTEM_OPEN == FALSE)
		{
			return retValue;
		}

		//��ȡStreamContext
		PSTREAM_HANDLE_CONTEXT pCtx;

		status = FltGetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT *)&pCtx);

		if (!NT_SUCCESS(status))
		{
			return retValue;
		}

		BOOLEAN canDecode = FALSE;

		//����ļ��Ѿ�����
		if (pCtx->isEncypted == IS_ENCRYPTED)
		{
			//�������Ƿ�Ϊ���ܽ���
			//��ȡ��������
			PCHAR procName = GetCurrentProcessName(ProcessNameOffset);
			////����ǻ��ܽ��������
			if (strncmp(procName, "notepad.exe", strlen(procName)) == 0)
				//if (IsSecretProcess(ctx->keyWord, procName))
			{
				//����������Ϣ����������
				p2pCtx = (PPRE_2_POST_CONTEXT)ExAllocateFromNPagedLookasideList(&Pre2PostContextList);
				if (p2pCtx == NULL) {
					goto clean_res;
				}
				*CompletionContext = p2pCtx;

				PFILE_STANDARD_INFORMATION fileInfo = &(pCtx->fileInfo);

				//��ȡ�ļ�����
				LONGLONG offset = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart);

				//�����дƫ�Ƴ����������ļ�β
				if (offset < 0)
				{
					DbgPrint("[MyMiniEncrypt]MyPreRead��END OF FILE");
					iopb->Parameters.Read.ByteOffset.QuadPart = fileInfo->EndOfFile.QuadPart + 1;

					return FLT_PREOP_SUCCESS_NO_CALLBACK;
				}
				//У�Զ�д����
				offset = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart + iopb->Parameters.Read.Length - 1);
				//��д���ȳ����ļ�������β��
				if (offset < 0)
				{
					//DbgPrint("reset read file length");
					iopb->Parameters.Read.Length = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart) + 1;
					FltSetCallbackDataDirty(Data);
					readLen = iopb->Parameters.Read.Length;
				}
				p2pCtx->IS_ENCODED = FALSE;

				if (iopb->IrpFlags&(IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE))
				{
					canDecode = TRUE;
				}
				//canDecode=TRUE;
				retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}
			//else//�ǻ��ܽ���ֱ������
			//{
			//	return FLT_PREOP_SUCCESS_NO_CALLBACK;
			//}
		}
		//�ǻ����ĵ�ֱ������
		else
		{
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}

		//����������ļ����ļ���������ܻ����������滻ԭ����irp������
		if (canDecode)
		{
			DbgPrint("[MyMiniEncrypt]MyPreRead��read file in canDecode");
			if (readLen == 0)
			{
				goto clean_res;
			}

			//������ȡβ��ƫ�ƵĴ���
			//��ѯ�ļ�β��ƫ��
			//��ȡ�ļ���Ϣ
			//�����»�����
			newBuf = ExAllocatePoolWithTag(NonPagedPool,readLen,BUFFER_SWAP_TAG);

			//�������ʧ�ܣ�����
			if (newBuf == NULL) 
			{
				goto clean_res;
			}

			//����MDL
			if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
			{
				newMdl = IoAllocateMdl(newBuf,readLen,FALSE,FALSE,NULL);

				if (newMdl == NULL) 
				{
					goto clean_res;
				}

				MmBuildMdlForNonPagedPool(newMdl);
			}

			//����������
			iopb->Parameters.Read.ReadBuffer = newBuf;
			iopb->Parameters.Read.MdlAddress = newMdl;
			FltSetCallbackDataDirty(Data);		//??

			//�����»�������������
			p2pCtx->SwappedBuffer = newBuf;

			p2pCtx->IS_ENCODED = TRUE;
			retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

clean_res:
			//������ǳɹ������ͷ���Դ
			if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK)
			{
				if (newBuf != NULL) 
				{
					ExFreePool(newBuf);
				}

				if (newMdl != NULL) 
				{
					IoFreeMdl(newMdl);
				}

				if (p2pCtx != NULL)
				{
					ExFreeToNPagedLookasideList(&Pre2PostContextList,p2pCtx);
				}
			}
		}
		//����ǲ�������ȡ�ļ����ļ������Լ�ʵ�ֶ�ȡ���ݣ�����postread�н��ܣ�ֱ�ӽ���
		else 
		{
			DbgPrint("readFile in nodecode");
			retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

			//�Լ�ʵ�ֶ�
			PVOID origBuf = NULL;
			FltLockUserBuffer(Data);

			//��ȡԭ���Ļ����������ȴ�MDL��ȡ
			if (iopb->Parameters.Read.MdlAddress != NULL) 
			{
				//DbgPrint("get buffer from MDL");
				origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,NormalPagePriority);
				if (origBuf == NULL)
				{
					//DbgPrint("memony is err in my read");
					Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					Data->IoStatus.Information = 0;
					return FLT_PREOP_COMPLETE;
				}
			}
			//�建����ֱ����
			else   
			{
				origBuf = iopb->Parameters.Read.ReadBuffer;
			}

			if (origBuf != NULL)
			{
				status = FltReadFile(FltObjects->Instance,
					FltObjects->FileObject,
					&(iopb->Parameters.Read.ByteOffset),
					iopb->Parameters.Read.Length,
					origBuf,
					FLTFL_IO_OPERATION_NON_CACHED,
					&(Data->IoStatus.Information)
					, NULL, NULL);
				Data->IoStatus.Status = status;
				//DbgPrint("my read is complete");
				//����
				DecodeData(origBuf, origBuf, iopb->Parameters.Read.ByteOffset.QuadPart, Data->IoStatus.Information, key);
				return FLT_PREOP_COMPLETE;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("exception had happened");
	}

	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
MyPostRead(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	PVOID origBuf = NULL;
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	NTSTATUS status;
	PPRE_2_POST_CONTEXT p2pCtx = (PPRE_2_POST_CONTEXT)CompletionContext;
	BOOLEAN cleanupAllocatedBuffer = TRUE;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}

	//�����ļ�
	__try
	{
		//��ȡStreamContext
		PSTREAM_HANDLE_CONTEXT pCtx;

		status = FltGetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT *)&pCtx);

		if (!NT_SUCCESS(status))
		{
			return retValue;
		}

		BOOLEAN canDecode = TRUE;

		//����Ǽ����ļ�
		if (p2pCtx->IS_ENCODED)
		{
			//DbgPrint("process  %s is reading file",procName);
			//DbgPrint("MyPostRead");	
			//if (p2pCtx == NULL)
			//{
			//	goto clean_res;
			//}

			//���ܽ�������������
			DecodeData(p2pCtx->SwappedBuffer, p2pCtx->SwappedBuffer, iopb->Parameters.Read.ByteOffset.QuadPart, Data->IoStatus.Information, key);

			//��ȡԭ���Ļ����������ȴ�MDL��ȡ
			if (iopb->Parameters.Read.MdlAddress != NULL) 
			{
				//DbgPrint("get buffer from MDL");
				origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
					NormalPagePriority);
				if (origBuf == NULL)
				{
					origBuf = iopb->Parameters.Read.ReadBuffer;
				}
			}
			//�建����ֱ����
			else  
			if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||	FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) 
			{
				origBuf = iopb->Parameters.Read.ReadBuffer;
			}
			//�ȴ���ȫ�� ����
			//else 
			//{
			//	if (FltDoCompletionProcessingWhenSafe(Data,
			//		FltObjects,
			//		CompletionContext,
			//		Flags,
			//		SwapPostReadBuffersWhenSafe,
			//		&retValue)) 
			//	{
			//		cleanupAllocatedBuffer = FALSE;
			//	}
			//	else 
			//	{
			//		Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
			//		Data->IoStatus.Information = 0;
			//	}
			//	goto clean_res;
			//}

			if (origBuf != NULL)
			{
				//�������û����������ݵ�ԭ����
				RtlCopyMemory(origBuf,
					p2pCtx->SwappedBuffer,
					Data->IoStatus.Information);

				FltSetCallbackDataDirty(Data);
			}

			//������дƫ��
			PFILE_STANDARD_INFORMATION fileInfo = &(pCtx->fileInfo);
			//��ȡ�ļ�����
			LONGLONG offset;
			//У�Զ�д����
			offset = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart + iopb->Parameters.Read.Length - 1);
			//��д���ȳ����ļ�������β��
			if (offset<0)
			{
				//DbgPrint("reset read file length at post read");
				Data->IoStatus.Information = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart) + 1;
				//Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart=fileInfo->EndOfFile.QuadPart+1;
			}
			//����

//clean_res:
			//�������
			if (cleanupAllocatedBuffer) 
			{
				ExFreePool(p2pCtx->SwappedBuffer);
				ExFreeToNPagedLookasideList(&Pre2PostContextList,p2pCtx);
			}
		}
		else
		{
			//������дƫ��
			PFILE_STANDARD_INFORMATION fileInfo = &(pCtx->fileInfo);
			//��ȡ�ļ�����
			LONGLONG offset;
			//У�Զ�д����
			offset = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart + iopb->Parameters.Read.Length - 1);
			//��д���ȳ����ļ�������β��
			if (offset<0)
			{
				//DbgPrint("reset read file length at post read");
				Data->IoStatus.Information = (fileInfo->EndOfFile.QuadPart - ENCRYPT_MARK_LEN) - (iopb->Parameters.Read.ByteOffset.QuadPart) + 1;
				//Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart=fileInfo->EndOfFile.QuadPart+1;
			}
			FltSetCallbackDataDirty(Data);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("EXCEPTION HAPPENED IN POST READ");
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
MyPreWrite(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PVOID newBuf = NULL;
	PMDL newMdl = NULL;
	PPRE_2_POST_CONTEXT p2pCtx;
	PVOID origBuf;
	NTSTATUS status;
	ULONG writeLen;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}

	//����Ҫ��������ܣ��Ѿ���ΪCLEANUP������
	__try
	{
		return retValue;
		//��鿪��
		if (IS_SYSTEM_OPEN == FALSE)
		{
			return retValue;
		}

		//��ȡStreamContext
		PSTREAM_HANDLE_CONTEXT ctx;

		status = FltGetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT *)&ctx);

		if (!NT_SUCCESS(status))
		{
			//DbgPrint("can not get ctx in pre write");
			return retValue;
		}

		BOOLEAN canEncrypt = FALSE;
		BOOLEAN noCache = FALSE;

		if (ctx->isEncypted == IS_ENCRYPTED)
		{
			//�������Ƿ�Ϊ���ܽ���
			//��ȡ��������
			PCHAR procName = GetCurrentProcessName(ProcessNameOffset);
			//if (IsSecretProcess(ctx->keyWord, procName))
			if (strncmp(procName, "notepad.exe", strlen(procName)) == 0)
			{
				//�ǻ���д�ż���
				//��Ϊ�ǻ���д�Ż�д�뵽����
				if (iopb->IrpFlags&(IRP_NOCACHE))
				{
					noCache = FALSE;
					canEncrypt = TRUE;
					writeLen = iopb->Parameters.Write.Length += ENCRYPT_MARK_LEN;
					DbgPrint("write data use encrypt");
				}
				else
				{
					noCache = FALSE;
					canEncrypt = TRUE;
					writeLen = iopb->Parameters.Write.Length;

					return retValue;
				}
			}

		}

		//������Լ����ļ�
		if (canEncrypt)
		{
			//DbgPrint("process name is %s writing file",procName);
			//д��0�ֽڲ�����

			//FltLockUserBuffer( Data );
			//��MDL��ȡԭ������
			if (iopb->Parameters.Write.MdlAddress != NULL)
			{
				origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Write.MdlAddress,
					NormalPagePriority);
				if (origBuf == NULL)
				{
					Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					Data->IoStatus.Information = 0;
					DbgPrint("get mdl address err");
					goto clean_res;
				}
			}
			else
			{
				origBuf = iopb->Parameters.Write.WriteBuffer;
			}

			if (origBuf == NULL)
			{
				DbgPrint("can not get orign buf in pre write");
				goto clean_res;
			}

			//�����»�����
			newBuf = ExAllocatePoolWithTag(NonPagedPool,
				writeLen,
				BUFFER_SWAP_TAG);

			if (newBuf == NULL) {
				DbgPrint("can not get buf in pre write");
				goto clean_res;
			}

			//��ȡMDL
			if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
			{
				newMdl = IoAllocateMdl(newBuf,writeLen,	FALSE,FALSE,NULL);

				if (newMdl == NULL) {
					DbgPrint("can not get mdl in pre write");
					goto clean_res;
				}

				MmBuildMdlForNonPagedPool(newMdl);
			}

			//�������ݵ��»�����
			RtlCopyMemory(newBuf,
				origBuf,
				iopb->Parameters.Write.Length);

			EncryptData(newBuf, newBuf, iopb->Parameters.Write.ByteOffset.QuadPart, iopb->Parameters.Write.Length, key);

			//����Ƿǻ���ͼ���
			if (noCache)
			{
				//�����»�����
				EncryptData(newBuf, newBuf, iopb->Parameters.Write.ByteOffset.QuadPart, iopb->Parameters.Write.Length, key);
				WriteEncryptTrail(newBuf, iopb->Parameters.Write.Length);
			}

			p2pCtx = (PPRE_2_POST_CONTEXT)ExAllocateFromNPagedLookasideList(&Pre2PostContextList);

			if (p2pCtx == NULL) {
				DbgPrint("p2p get err");
				goto clean_res;
			}

			//�滻����
			iopb->Parameters.Write.WriteBuffer = newBuf;
			iopb->Parameters.Write.MdlAddress = newMdl;
			if (noCache)
			{
				iopb->Parameters.Write.Length += ENCRYPT_MARK_LEN;
			}

			FltSetCallbackDataDirty(Data);
			ctx->isEncypted = IS_ENCRYPTED;

			//��¼�»�������ַ�������ͷ�
			p2pCtx->SwappedBuffer = newBuf;
			*CompletionContext = p2pCtx;

			//
			retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

			//�뿪
clean_res:
			//�ͷŻ���
			if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) 
			{
				if (newBuf != NULL) 
				{
					ExFreePool(newBuf);
				}

				if (newMdl != NULL) 
				{
					IoFreeMdl(newMdl);
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("exception happened in MyPreWrite");
	}

	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
MyPostWrite(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	PPRE_2_POST_CONTEXT p2pCtx = (PPRE_2_POST_CONTEXT)CompletionContext;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	//�ͷŻ�����
	ExFreePool(p2pCtx->SwappedBuffer);
	ExFreeToNPagedLookasideList(&Pre2PostContextList,p2pCtx);

	Data->IoStatus.Information = Data->Iopb->Parameters.Write.Length;

	FltSetCallbackDataDirty(Data);

	return FLT_POSTOP_FINISHED_PROCESSING;

}

FLT_PREOP_CALLBACK_STATUS
MyPreClose(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}

	//���ܴ��رյ��ļ�
	PSTREAM_HANDLE_CONTEXT ctx;
	NTSTATUS status = FltGetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT *)&ctx);
	if (NT_SUCCESS(status))
	{

		//�������

		//��ʼ������ʱ������
		STREAM_HANDLE_CONTEXT temCtx;
		temCtx.isEncyptFile = IS_NOT_ENCRYPT_FILE;
		temCtx.isEncypted = IS_NOT_ENCRYPTED;
//		temCtx.keyWord = NULL;


		//��ȡ�ļ��ļ�����Ϣ
		status = MyGetFileEncryptInfoToCtx(Data, FltObjects, &temCtx);// , key_word_header);
		if (!NT_SUCCESS(status))
		{
			return retValue;
		}


		//������Ǽ������ͣ�ֱ�ӷŹ�
		if (temCtx.isEncyptFile != IS_ENCRYPT_FILE)
		{
			return retValue;
		}
		//cfFileCacheClear(FltObjects->FileObject);

		if (temCtx.isEncypted == IS_NOT_ENCRYPTED)
		{
			//����ǻ��ܽ��̣��򿪾ͼ���
			//��ȡ��������
			PCHAR procName = GetCurrentProcessName(ProcessNameOffset);
			//����ǻ��ܽ���������ļ�
			if (strncmp(procName, "notepad.exe", strlen(procName)) == 0)
//			if (IsSecretProcess(temCtx.keyWord, procName))
			{
				DbgPrint("cleanup  PROCESS name is %s", procName);
				//�����ļ�
				status = EncryptFile(Data, FltObjects, key);
				if (NT_SUCCESS(status))
				{

					ctx->isEncypted = IS_ENCRYPTED;
					DbgPrint("encrypt a file");
				}
				else
				{
					DbgPrint("encrypt a file fail");
				}
			}
		}
	}
	//cfFileCacheClear(FltObjects->FileObject);

	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
MyPostClose(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
MyPreQueryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MyPostQueryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	STREAM_HANDLE_CONTEXT ctx;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}
	//��ȡ�ļ�������Ϣ
	__try
	{
		//��鿪��
		if (!IS_SYSTEM_OPEN)
		{
			return retValue;
		}


		//��ȡStreamContext

		NTSTATUS status = MyGetFileEncryptInfoToCtx(Data, FltObjects, &ctx);// , key_word_header);


		if (NT_SUCCESS(status))
		{
			//DbgPrint("get file info in post info");
			//�Ǽ����������Ѿ�������
			if (ctx.isEncypted == IS_ENCRYPTED)
			{

				//�������Ƿ�Ϊ���ܽ���
				//��ȡ��������
				PCHAR procName = GetCurrentProcessName(ProcessNameOffset);

				if (strncmp(procName, "notepad.exe", strlen(procName)) == 0)
//				if (!IsSecretProcess(ctx.keyWord, procName))
				{
					return retValue;
				}


				//�޸���Ϣ���ļ�����
				PVOID buff = iopb->Parameters.QueryFileInformation.InfoBuffer;
				//ѡ�������������

				switch (iopb->Parameters.QueryFileInformation.FileInformationClass)
				{

				case FileStandardInformation:
				{
												//DbgPrint("QueryFileStandardInformation");
												PFILE_STANDARD_INFORMATION info = (PFILE_STANDARD_INFORMATION)buff;
												info->AllocationSize.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
												info->EndOfFile.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
												break;
				}
				case FileAllInformation:
				{
										   //DbgPrint("QueryFileAllInformation");
										   PFILE_ALL_INFORMATION info = (PFILE_ALL_INFORMATION)buff;
										   if (Data->IoStatus.Information >=
											   sizeof(FILE_BASIC_INFORMATION)+
											   sizeof(FILE_STANDARD_INFORMATION))
										   {
											   info->StandardInformation.AllocationSize.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
											   info->StandardInformation.EndOfFile.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;

											   if (Data->IoStatus.Information >=
												   sizeof(FILE_BASIC_INFORMATION)+
												   sizeof(FILE_STANDARD_INFORMATION)+
												   sizeof(FILE_EA_INFORMATION)+
												   sizeof(FILE_ACCESS_INFORMATION)+
												   sizeof(FILE_POSITION_INFORMATION))
											   {
												   info->PositionInformation.CurrentByteOffset.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
											   }
										   }
										   break;
				}
				case FileAllocationInformation:
				{
												  //DbgPrint("QueryFileAllocationInformation");
												  PFILE_ALLOCATION_INFORMATION info = (PFILE_ALLOCATION_INFORMATION)buff;
												  info->AllocationSize.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
												  break;
				}
				case FileValidDataLengthInformation:
				{
													   //DbgPrint("QueryFileValidDataLengthInformation");
													   PFILE_VALID_DATA_LENGTH_INFORMATION info = (PFILE_VALID_DATA_LENGTH_INFORMATION)buff;
													   info->ValidDataLength.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
													   break;
				}
				case FileEndOfFileInformation:
				{
												 //DbgPrint("QueryFileEndOfFileInformation");
												 PFILE_END_OF_FILE_INFORMATION info = (PFILE_END_OF_FILE_INFORMATION)buff;
												 info->EndOfFile.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
												 break;
				}
				case FilePositionInformation:
				{
												//DbgPrint("QueryFilePositionInformation");
												PFILE_POSITION_INFORMATION info = (PFILE_POSITION_INFORMATION)buff;
												info->CurrentByteOffset.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
												break;
				}
				case FileStreamInformation:
				{
											  //DbgPrint("QueryFileStreamInformation");
											  PFILE_STREAM_INFORMATION info = (PFILE_STREAM_INFORMATION)buff;
											  info->StreamAllocationSize.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
											  info->StreamSize.QuadPart -= ENCRYPT_FILE_CONTENT_OFFSET;
											  break;
				}
				default:
				{
						   //DbgPrint("DEFAULT");
						   //DbgPrint("query FileInformationClass is %d",iopb->Parameters.QueryFileInformation.FileInformationClass);
						   break;
				}
				}
				FltSetCallbackDataDirty(Data);
			}
		}
		else
		{
			//DbgPrint("get file info fail in post file");
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("err happen in post info");
	}

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
MyPreSetInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	STREAM_HANDLE_CONTEXT ctx;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}
	//��ȡ�ļ�������Ϣ
	__try
	{
		//��鿪��
		if (!IS_SYSTEM_OPEN)
		{
			return retValue;
		}

		//��ȡStreamContext
		NTSTATUS status = MyGetFileEncryptInfoToCtx(Data, FltObjects, &ctx);// , key_word_header);

		if (NT_SUCCESS(status))
		{
			//�Ǽ����������Ѿ�������
			if (ctx.isEncypted == IS_ENCRYPTED)
			{

				//�������Ƿ�Ϊ���ܽ���
				//��ȡ��������
				PCHAR procName = GetCurrentProcessName(ProcessNameOffset);

				if (strncmp(procName, "notepad.exe", strlen(procName)) == 0)
//				if (!IsSecretProcess(ctx.keyWord, procName))
				{
					return retValue;
				}


				//�޸���Ϣ���ļ�����
				PVOID buff = iopb->Parameters.SetFileInformation.InfoBuffer;
				//�޸�����ƫ��
				switch (iopb->Parameters.SetFileInformation.FileInformationClass)
				{

				case FileStandardInformation:
				{
												//DbgPrint("setFileStandardInformation");
												PFILE_STANDARD_INFORMATION info = (PFILE_STANDARD_INFORMATION)buff;
												info->AllocationSize.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
												info->EndOfFile.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
												break;
				}
				case FileAllInformation:
				{
										   //DbgPrint("setFileAllInformation");
										   PFILE_ALL_INFORMATION info = (PFILE_ALL_INFORMATION)buff;
										   if (Data->IoStatus.Information >=
											   sizeof(FILE_BASIC_INFORMATION)+
											   sizeof(FILE_STANDARD_INFORMATION))
										   {
											   info->StandardInformation.AllocationSize.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
											   info->StandardInformation.EndOfFile.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;

											   if (Data->IoStatus.Information >=
												   sizeof(FILE_BASIC_INFORMATION)+
												   sizeof(FILE_STANDARD_INFORMATION)+
												   sizeof(FILE_EA_INFORMATION)+
												   sizeof(FILE_ACCESS_INFORMATION)+
												   sizeof(FILE_POSITION_INFORMATION))
											   {
												   info->PositionInformation.CurrentByteOffset.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
											   }
										   }
										   break;
				}
				case FileAllocationInformation:
				{
												  //DbgPrint("setFileAllocationInformation");
												  PFILE_ALLOCATION_INFORMATION info = (PFILE_ALLOCATION_INFORMATION)buff;
												  info->AllocationSize.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
												  break;
				}
				case FileValidDataLengthInformation:
				{
													   //DbgPrint("setFileValidDataLengthInformation");
													   PFILE_VALID_DATA_LENGTH_INFORMATION info = (PFILE_VALID_DATA_LENGTH_INFORMATION)buff;
													   info->ValidDataLength.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
													   break;
				}
				case FileEndOfFileInformation:
				{
												 //DbgPrint("setFileEndOfFileInformation");
												 PFILE_END_OF_FILE_INFORMATION info = (PFILE_END_OF_FILE_INFORMATION)buff;
												 info->EndOfFile.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
												 break;
				}
				case FilePositionInformation:
				{
												//DbgPrint("setFilePositionInformation");
												PFILE_POSITION_INFORMATION info = (PFILE_POSITION_INFORMATION)buff;
												info->CurrentByteOffset.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
												break;
				}
				case FileStreamInformation:
				{
											  //DbgPrint("setFileStreamInformation");
											  PFILE_STREAM_INFORMATION info = (PFILE_STREAM_INFORMATION)buff;
											  info->StreamAllocationSize.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
											  info->StreamSize.QuadPart += ENCRYPT_FILE_CONTENT_OFFSET;
											  break;

				}
				default:
				{
						   //DbgPrint("DEFAULT");
						   //DbgPrint("setFileInformationClass is %d",iopb->Parameters.QueryFileInformation.FileInformationClass);
						   break;
				}
				}
			}

		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}

	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
MyPostSetInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}
	return retValue;
}
