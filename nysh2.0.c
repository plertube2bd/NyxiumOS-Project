#define BUF_SIZE 128
#define SBU_MAX_OPTIONS 8

/** NyxiumBootloader Shell 2.0
     Copyleft (>) Nyxium Team
     an UEFI OS
     Update :: ls , cat , cd
**/ 

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SimpleFileSystem.h>  // EFI_FILE_PROTOCOL, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
#include <Guid/FileInfo.h>              // EFI_FILE_INFO, gEfiFileInfoGuid

typedef struct _System_Binary_Utility SBU;
EFI_STATUS EFIAPI SBU_InitializeLib(SBU *This);

// ----- 전방 선언 (Prototypes) -----
// 컴파일러에게 함수의 존재를 미리 알립니다.
EFI_STATUS EFIAPI SBU_ReadLine(SBU *This, CHAR16 *Buffer, UINTN BufferSize);
EFI_STATUS EFIAPI SBU_HandleRebootCommand(SBU *This, CHAR16 *Input);
EFI_STATUS EFIAPI SBU_HandleShutdownCommand(SBU *This);
BOOLEAN    EFIAPI SBU_DispatchCommand(SBU *This, CHAR16 *Input);
EFI_STATUS EFIAPI SBU_InitializeLib(SBU *This);

// ----- 옵션 구조체 -----
typedef struct {
    CHAR16 Opt;
    CHAR16 *LongOpt;
    CHAR16 *Value;
} SBU_PARSED_OPTION;

typedef struct {
    UINTN Count;
    SBU_PARSED_OPTION Options[SBU_MAX_OPTIONS];
} SBU_PARSED_OPTIONS;

CHAR16 *ShowVersion() {
    Print(L"Nyxium BootLoader Vsersion 2.0 ( Third Edition )\r\nCopyLeft (>) Nyxium Team - Temi\r\n");
    return L"2.0";
}

// ----- 옵션 찾기 -----
CHAR16 *SBU_FindOptionStart(CHAR16 *Str, CHAR16 OptChar)
{
    if (!Str) return NULL;
    while (*Str == L' ') Str++;
    if (*Str == OptChar) return Str;
    while (*Str) {
        if (*Str == OptChar && *(Str - 1) == L' ')
            return Str;
        Str++;
    }
    return NULL;
}

// ----- 옵션 파싱 -----
EFI_STATUS SBU_OptionParsing(CHAR16 *Input, SBU_PARSED_OPTIONS *Parsed)
{
    if (!Input || !Parsed) return EFI_INVALID_PARAMETER;
    ZeroMem(Parsed, sizeof(*Parsed));
    UINTN i = 0;
    CHAR16 *Ptr = SBU_FindOptionStart(Input, L'-');

    while (Ptr && i < SBU_MAX_OPTIONS) {
        if (*(Ptr + 1) == L'-') {
            // long option
            CHAR16 *End = Ptr + 2;
            while (*End && *End != L' ') End++;
            UINTN Len = End - (Ptr + 2);
            Parsed->Options[i].LongOpt = AllocatePool((Len + 1) * sizeof(CHAR16));
            if (!Parsed->Options[i].LongOpt) return EFI_OUT_OF_RESOURCES;
            CopyMem(Parsed->Options[i].LongOpt, Ptr + 2, Len * sizeof(CHAR16));
            Parsed->Options[i].LongOpt[Len] = L'\0';
        } else {
            // short option
            Parsed->Options[i].Opt = *(Ptr + 1);
        }
        i++;
        Parsed->Count = i;
        Ptr = SBU_FindOptionStart(Ptr + 1, L'-');
    }

    return EFI_SUCCESS;
}

// ----- SBU 구조체 -----
struct _System_Binary_Utility {
    EFI_STATUS (EFIAPI *ReadLine)(SBU *This, CHAR16 *Buffer, UINTN BufferSize);
    EFI_STATUS (EFIAPI *HandleRebootCommand)(SBU *This, CHAR16 *Input);
    EFI_STATUS (EFIAPI *HandleShutdownCommand)(SBU *This);
    BOOLEAN    (EFIAPI *DispatchCommand)(SBU *This, CHAR16 *Input);
    EFI_STATUS (EFIAPI *InitializeLib)(SBU *This);
};

// ----- BOOT FILE MANAGER -----
typedef struct _BOOT_FILE_MANAGER BFm;
struct _BOOT_FILE_MANAGER {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    
    EFI_STATUE (*ProtocolHeader)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **FsProtocol, EFI_FILE_PROTOCOL **RootHandle, EFI_STATUS *Status );
    EFI_STATUS (*MakeFatFile)(IN BFm *This, IN CHAR16 *FileName);
    EFI_STATUS (*ChangeDirectory)
    
    EFI_STATUS (*ReadFatFile)(BFm *This, CHAR16 *FilePath, VOID **OutBuffer, UINTN *OutSize);
    EFI_STATUS (*ReadFatFolder)(BFm *This, CHAR16 *Path, CHAR16 *Buffer);
    EFI_STATUS (*Init)(BFm *This);
};

EFI_STATUS BFm_ChangeDirectory(
    IN BFm *This,
    IN CHAR16 *dir
) {
    CHAR16 *sub = dir + 3;
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *Dir;
    if (sub[0] == L'\\') {
        Status = This->Fs->OpenVolume(This->Fs, &Root);
        if (EFI_ERROR(Status)) return Status;
        Status = Root->Open(Root, 8Dir, dir, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
          
        DIRECTORY = sub;
    } else {
        
        
        

EFI_STATUS BFm_ProtocolHeader(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **FsProtocol, EFI_FILE_PROTOCOL **RootHandle, EFI_STATUS *Status) {
    *Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)FsProtocol);
    
    if (EFI_ERROR(*Status)) {
        Print(L"Cannot find Protocol.\r\n");
        return *Status;
    }

    *Status = (*FsProtocol)->OpenVolume((*FsProtocol), RootHandle);

    if (EFI_ERROR(*Status)) {
        Print(L"Unknown Error occur during open the volume.\r\n");
        return *Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS BFm_MakeFatFile(IN BFm *This, IN CHAR16 *FileName) {
    EFI_FILE_PROTOCOL *RootHandle = NULL;  //root directory handle
    EFI_STATUS Status;

    BFSU_ProtocolHeader(&Fs, &RootHandle, &Status);
    
    EFI_FILE_PROTOCOL *FileHandle = NULL;  //current file handle
    Status = RootHandle->Open(RootHandle, &FileHandle, FileName, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(Status)) {
        Print(L"Cannot Create File : ");
        if (Status == EFI_NO_MEDIA) {
            Print(L"NO MEDIA\r\n");
        }
        else if (Status == EFI_VOLUME_CORRUPTED) {
            Print(L"VOLUME CORRUPTED\r\n");
        }
        else if (Status == EFI_DEVICE_ERROR) {
            Print(L"DEVICE ERROR\r\n");
        }
        else if (Status == EFI_ACCESS_DENIED) {
            Print(L"ACCESS DENIED\r\n");
        }
        else if (Status == EFI_VOLUME_FULL) {
            Print(L"VOLUME FULL\r\n");
        }
        else if (Status == EFI_OUT_OF_RESOURCES) {
            Print(L"OUT OF RESOURCE\r\n");
        }
        else if (Status == EFI_UNSUPPORTED) {
            Print(L"UNSUPPORT\r\n");
        }
        else if (Status == EFI_INVALID_PARAMETER) {
            Print(L"INVAILD_PARAMETER\r\n");
        }
        else {
            Print(L"I don't know. but something is Error.\r\n");
        }
        RootHandle->Close(RootHandle);
        return Status;
    }

    Print(L"Make File %s in %s\r\n", FileName, This->CurrentDirectoryPath);
    FileHandle->Close(FileHandle);
    RootHandle->Close(RootHandle);
    return EFI_SUCCESS;
}

// ----- 파일 읽기 -----
EFI_STATUS BFm_ReadFatFile(BFm *This, CHAR16 *FilePath, VOID **OutBuffer, UINTN *OutSize)
{
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    EFI_FILE_PROTOCOL *Root = NULL, *File = NULL;
    EFI_FILE_INFO *Info = NULL;
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0, InfoSize = 0;
    VOID *Buffer = NULL;

    if (!FilePath || !OutBuffer || !OutSize) return EFI_INVALID_PARAMETER;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status)) return Status;

    for (UINTN i = 0; i < HandleCount; i++) {
        Status = gBS->HandleProtocol(Handles[i], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
        if (EFI_ERROR(Status)) continue;
        Status = Fs->OpenVolume(Fs, &Root);
        if (EFI_ERROR(Status)) continue;
        Status = Root->Open(Root, &File, FilePath, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) { Root->Close(Root); continue; }

        // 파일 정보 조회
        Status = File->GetInfo(File, &gEfiFileInfoGuid, &InfoSize, NULL);
        if (Status != EFI_BUFFER_TOO_SMALL) goto Cleanup;

        Info = AllocatePool(InfoSize);
        if (!Info) goto Cleanup;

        Status = File->GetInfo(File, &gEfiFileInfoGuid, &InfoSize, Info);
        if (EFI_ERROR(Status)) goto Cleanup;

        if (Info->FileSize == 0 || Info->FileSize > MAX_UINTN) goto Cleanup;

        Buffer = AllocatePool((UINTN)Info->FileSize);
        if (!Buffer) goto Cleanup;

        File->SetPosition(File, 0);
        *OutSize = (UINTN)Info->FileSize;
        Status = File->Read(File, OutSize, Buffer);
        if (EFI_ERROR(Status)) goto Cleanup;

        *OutBuffer = Buffer;
        Status = EFI_SUCCESS;
        goto Done;
    }

    Cleanup:
        if (Buffer) {
            FreePool(Buffer);
            Status = EFI_NOT_FOUND;
        }
    Done:
        if (Info) FreePool(Info);
        if (Handles) FreePool(Handles);
        return Status;
}

// ----- BFm 초기화 -----
EFI_STATUS BFm_Init(BFm *This)
{
    EFI_STATUS Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID **)&This->Fs);
    if (EFI_ERROR(Status)) return Status;

    This->ReadFatFile = BFm_ReadFatFile;
    This->Init = BFm_Init;
    return EFI_SUCCESS;
}

// ----- ReadLine -----
EFI_STATUS EFIAPI SBU_ReadLine(SBU *This, CHAR16 *Buffer, UINTN BufferSize)
{
    EFI_INPUT_KEY Key;
    UINTN Pos = 0, Cursor = 0, Index;
    while (1) {
        gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index);
        if (EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &Key))) continue;

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            Buffer[Pos] = L'\0';
            Print(L"\r\n");
            return EFI_SUCCESS;
        } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
            if (Cursor > 0 && Pos > 0) {
                Cursor--; Pos--;
                Buffer[Pos] = L'\0';
                Print(L"\b \b");
            }
        } else if (Pos < BufferSize - 1) {
            Buffer[Pos++] = Key.UnicodeChar;
            Buffer[Pos] = L'\0';
            Print(L"%c", Key.UnicodeChar);
            Cursor++;
        }
    }
}

// ----- UEFI Entry -----
CONST CHAR16 PROMPT[] = L"] ";
CHAR16 DIRECTORY[4096] = L"\\";

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
) {
    Print(L"Initializing Library...\r\n");

    BFm Bfm;
    SBU Shell;

    // 파일 시스템 초기화
    if (EFI_ERROR(BFm_Init(&Bfm))) {
        Print(L"Failed to init file system.\r\n");
    } else {
        // 부팅 직후 readme.txt 읽기
        VOID *FileBuffer = NULL;
        UINTN FileSize = 0;
        if (EFI_ERROR(BFm_ReadFatFile(&Bfm, L"\\readme.txt", &FileBuffer, &FileSize))) {
            Print(L"readme.txt not found.\r\n");
        } else {
            Print(L"--- readme.txt ---\r\n");
            // 유니코드 문자열로 가정하고 출력
            Print((CHAR16 *)FileBuffer);
            Print(L"\r\n--- end ---\r\n");
            FreePool(FileBuffer);  // 메모리 해제
        }
    }

    // SBU 초기화
    SBU_InitializeLib(&Shell);
    Print(L"Nyxium Shell has loaded.\r\n");

    while (1) {
        Print(PROMPT);

        CHAR16 Input[BUF_SIZE];
        Shell.ReadLine(&Shell, Input, BUF_SIZE);

        if (!Shell.DispatchCommand(&Shell, Input))
            break;
    }

    return EFI_SUCCESS;
}

// ----- 초기화 -----
EFI_STATUS EFIAPI SBU_InitializeLib(SBU *This)
{
    This->ReadLine = SBU_ReadLine;
    This->HandleShutdownCommand = SBU_HandleShutdownCommand;
    This->HandleRebootCommand   = SBU_HandleRebootCommand;
    This->DispatchCommand       = SBU_DispatchCommand;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI SBU_HandleRebootCommand(
    SBU *This,
    CHAR16 *Input
) {
    SBU_PARSED_OPTIONS Parsed;
    BOOLEAN Cold = FALSE;
    BOOLEAN Warm = FALSE;

    // 1. 옵션 파싱
    if (EFI_ERROR(SBU_OptionParsing(Input + 6, &Parsed))) {
        Print(L"Failed to parse options.\r\n");
        return EFI_INVALID_PARAMETER;
    }

    // 2. 파싱 결과 확인
    for (UINTN i = 0; i < Parsed.Count; i++) {
        SBU_PARSED_OPTION *Opt = &Parsed.Options[i];

        // short options
        if (Opt->Opt) {
            switch (Opt->Opt) {
                case L'c':
                    Cold = TRUE;
                    break;
                case L'w':
                    Warm = TRUE;
                    break;
                default:
                    Print(L"Unknown short option '-%c'\r\n", Opt->Opt);
            }
        }
        // long options
        else if (Opt->LongOpt) {
            if (StrCmp(Opt->LongOpt, L"cold") == 0)
                Cold = TRUE;
            else if (StrCmp(Opt->LongOpt, L"warm") == 0)
                Warm = TRUE;
            else
                Print(L"Unknown long option '--%s'\r\n", Opt->LongOpt);
        }
    }

    // 3. 실제 리부트 수행
    if (Warm) {
        gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
    } else if (Cold) {
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    } else {
        Print(L"'reboot' command needs an option. type help\r\n");
        Print(L"'-w' or '--warm' = warm reboot\r\n");
        Print(L"'-c' or '--cold' = cold reboot\r\n");
    }

    return EFI_SUCCESS;
}
EFI_STATUS EFIAPI SBU_HandleShutdownCommand(SBU *This) {
    gRT->ResetSystem(
        EfiResetShutdown,
        EFI_SUCCESS,
        0,
        NULL
    );
    return EFI_SUCCESS;
}


BOOLEAN EFIAPI
SBU_DispatchCommand(
    SBU *This,
    CHAR16 *Input
) {
    BFm *Bfm;
    if (StrCmp(Input, L"exit") == 0) {
        return FALSE; // 쉘 종료

    } else if (StrCmp(Input, L"shutdown") == 0) {
        This->HandleShutdownCommand(This);

    } else if (
        StrnCmp(Input, L"reboot", 6) == 0 &&
        (Input[6] == L'\0' || Input[6] == L' ')
    ) {
        This->HandleRebootCommand(This, Input);
    } else if ( Input[0] == L'\0' )
    else if ( StrCmp(Input, "version") == 0 ) ShowVersion();
    else if ( StrnCmp(Input, L"cd", 2) == 0 && (Input[2] == L'\0' || Input[2] == L' ')  
    else {
        Print(L"Unknown command. type 'help'\r\n");
    }

    return TRUE;
}