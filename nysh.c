#define BUF_SIZE 128

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

typedef struct _System_Binary_Utility SBU;
struct _System_Binary_Utility {
    EFI_STATUS (*ReadLine)(
        SBU *This,
        CHAR16 *Buffer,
        UINTN BufferSize
    );
    EFI_STATUS (*HandleRebootCommand)(
        SBU *This,
        CHAR16 *Input
    );
    EFI_STATUS (*HandleShutdownCommand)(
        SBU *This
    );
    BOOLEAN (*DispatchCommand)(
        SBU *This,
        CHAR16 *Input
    );
    EFI_STATUS (*InitializeLib)(
        SBU *This
    );
};
    

typedef struct _BOOT_FILE_MANAGER BFm;
// 간단하게 파일과 폴더(디렉토리)를 읽는 매니저
struct _BOOT_FILE_MANAGER {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;

    EFI_STATUS (*ReadFatFile)(
        BFm *This,
        IN  CHAR16 *FilePath,
        OUT VOID  **OutBuffer,
        OUT UINTN *OutSize
    );
    // FAT32 파일 내용을 Buffer에 저장하는 함수
    EFI_STATUS (*ReadFatFolder)(
        BFm *This,
        IN CHAR16 *Path
    );
    // FAT32 폴더 내용을 출력하는 함수

    EFI_STATUS (*Init)(
        BFm *This
    );
    // BOOT FILE MANAGER을 시작하는 함수
};

EFI_STATUS
BFm_ReadFatFile(
    BFm *This, 
    IN  CHAR16 *FilePath, // 읽을 파일 경로
    OUT VOID  **OutBuffer, // 파일 내용을 넣을곳
    OUT UINTN  *OutSize // 어느크기만큼 유니코드로 넣을지 결정
) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    EFI_FILE_PROTOCOL *Root = NULL;
    EFI_FILE_PROTOCOL *File = NULL;
    EFI_FILE_INFO *Info = NULL;
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;
    UINTN InfoSize = 0;
    VOID *Buffer = NULL;

    if (!FilePath || !OutBuffer || !OutSize)
        return EFI_INVALID_PARAMETER;

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
    );
    if (EFI_ERROR(Status))
        return Status;

    for (UINTN i = 0; i < HandleCount; i++) {

        Status = gBS->HandleProtocol(
            Handles[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&Fs
        );
        if (EFI_ERROR(Status))
            continue;

        Status = Fs->OpenVolume(Fs, &Root);
        if (EFI_ERROR(Status))
            continue;

        Status = Root->Open(
            Root,
            &File,
            FilePath,
            EFI_FILE_MODE_READ,
            0
        );
        if (EFI_ERROR(Status)) {
            Root->Close(Root);
            continue;
        }

        /* 파일 정보 크기 조회 */
        Status = File->GetInfo(
            File,
            &gEfiFileInfoGuid,
            &InfoSize,
            NULL
        );
        if (Status != EFI_BUFFER_TOO_SMALL)
            goto Cleanup;

        Status = AllocatePool(EfiBootServicesData, InfoSize, (VOID **)&Info);
        if (EFI_ERROR(Status))
            goto Cleanup;

        Status = File->GetInfo(
            File,
            &gEfiFileInfoGuid,
            &InfoSize,
            Info
        );
        if (EFI_ERROR(Status))
            goto Cleanup;

        if (Info->FileSize == 0)
            goto Cleanup;

        if (Info->FileSize > MAX_UINTN)
            goto Cleanup;

        Status = AllocatePool(
            EfiBootServicesData,
            (UINTN)Info->FileSize,
            &Buffer
        );
        if (EFI_ERROR(Status))
            goto Cleanup;

        File->SetPosition(File, 0);
        *OutSize = (UINTN)Info->FileSize;
        Status = File->Read(File, OutSize, Buffer);
        if (EFI_ERROR(Status))
            goto Cleanup;

        *OutBuffer = Buffer;
        Status = EFI_SUCCESS;
        goto Done;

Cleanup:
    if (Buffer) FreePool(Buffer);
    if (Info)   FreePool(Info);
    if (File)   File->Close(File);
    if (Root)   Root->Close(Root);

    Status = EFI_NOT_FOUND;

Done:
    if (Handles)
        FreePool(Handles);
    return Status;
}

EFI_STATUS
BFm_Init(BFm *This)
{
    EFI_STATUS Status;

    Status = gBS->LocateProtocol(
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        (VOID **)&This->Fs
    );
    if (EFI_ERROR(Status))
        return Status;

    This->ReadFatFile = BFm_ReadFatFile;
    This->Init = BFm_Init;
    return EFI_SUCCESS;
}

CONST CHAR16 PROMPT[] = L"] ";
// 프롬프트 쉘 문자열

EFI_STATUS EFIAPI SBU_ReadLine(
    SBU *This,
    CHAR16 *Buffer,
    UINTN BufferSize
) {
    EFI_INPUT_KEY Key;
    EFI_STATUS Status;
    UINTN Index;
    UINTN Pos = 0;
    UINTN Cursor = 0;

    UINTN Row = gST->ConOut->Mode->CursorRow;
    UINTN BaseCol = gST->ConOut->Mode->CursorColumn;

    while (1) {
        gBS->WaitForEvent(
            1,
            &gST->ConIn->WaitForKey,
            &Index
        );

        Status = gST->ConIn->ReadKeyStroke(
            gST->ConIn,
            &Key
        );

        if (EFI_ERROR(Status))
            continue;

        if (Key.UnicodeChar == 0) {
            if (Key.ScanCode == SCAN_LEFT && Cursor > 0) {
                Cursor--;
                gST->ConOut->SetCursorPosition(
                    gST->ConOut,
                    BaseCol + Cursor,
                    Row
                );
            } else if (Key.ScanCode == SCAN_RIGHT && Cursor < Pos) {
                Cursor++;
                gST->ConOut->SetCursorPosition(
                    gST->ConOut,
                    BaseCol + Cursor,
                    Row
                );
            }
            continue;
        }

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            Buffer[Pos] = L'\0';
            Print(L"\r\n");
            return EFI_SUCCESS;

        } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
            if (Cursor > 0 && Pos > 0) {
                Cursor--;
                Pos--;
                Buffer[Pos] = L'\0';
                Print(L"\b \b");
            }

        } else if (Key.UnicodeChar == 0x15) { // Ctrl+U
            while (Pos > 0) {
                Pos--;
                Print(L"\b \b");
            }
            Cursor = 0;
            Buffer[0] = L'\0';

        } else if (Pos < BufferSize - 1) {
            Buffer[Pos++] = Key.UnicodeChar;
            Buffer[Pos] = L'\0';
            Print(L"%c", Key.UnicodeChar);
            Cursor++;
        }
    }
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
) {
    
    BFm Bfm;
    SBU Shell;

    BFm_Init(&Bfm);
    SBU_InitializeLib(&Shell);
    Print(L"Nyxium Shell has loaded.\r\n");

    while (1) {
        Print(PROMPT);

        CHAR16 Input[BUF_SIZE];
        Shell.ReadLine(&Shell, Input, BUF_SIZE);

        if (!Shell.DispatchCommand(&Shell, Input)) {
            break;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS SBU_HandleRebootCommand(
    SBU *This,
    CHAR16 *Input
) {
    BOOLEAN Cold = FALSE;
    BOOLEAN Warm = FALSE;
    BOOLEAN Idk  = FALSE;

    CHAR16 *Ptr = Input + 6; // "reboot" 이후

    while (*Ptr != L'\0') {
        while (*Ptr == L' ') Ptr++;

        if (*Ptr == L'-') {
            switch (*(Ptr + 1)) {
                case L'c':
                    Cold = TRUE;
                    break;
                case L'w':
                    Warm = TRUE;
                    break;
                default:
                    Idk = TRUE;
            }
            Ptr += 2;
        } else {
            Ptr++;
        }
    }

    if (Warm) {
        gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
    } else if (Cold) {
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    } else if (Idk) {
        Print(L"Unknown option for 'reboot'. Did you mean '-w' or '-c'?\r\n");
        Print(L"'-w' = warm reboot, '-c' = cold reboot\r\n");
    } else {
        Print(L"'reboot' command needs an option. type help\r\n");
    }

    return EFI_SUCCESS;
}

EFI_STATUS
SBU_HandleShutdownCommand(SBU *This) {
    gRT->ResetSystem(
        EfiResetShutdown,
        EFI_SUCCESS,
        0,
        NULL
    );
    return EFI_SUCCESS;
}

BOOLEAN
SBU_DispatchCommand(
    SBU *This,
    CHAR16 *Input
) {
    if (StrCmp(Input, L"exit") == 0) {
        return FALSE; // 쉘 종료

    } else if (StrCmp(Input, L"shutdown") == 0) {
        This->HandleShutdownCommand(This);

    } else if (
        StrnCmp(Input, L"reboot", 6) == 0 &&
        (Input[6] == L'\0' || Input[6] == L' ')
    ) {
        This->HandleRebootCommand(This, Input);

    } else {
        Print(L"Unknown command. type 'help'\r\n");
    }

    return TRUE;
}

EFI_STATUS
SBU_InitializeLib(SBU *This)
{
    This->ReadLine = SBU_ReadLine;
    This->HandleShutdownCommand = SBU_HandleShutdownCommand;
    This->HandleRebootCommand   = SBU_HandleRebootCommand;
    This->DispatchCommand       = SBU_DispatchCommand;
    return EFI_SUCCESS;
}
