#include "Cactus_Tools.h"

UCactusVirtualFile::UCactusVirtualFile()
{

}

UCactusVirtualFile::~UCactusVirtualFile()
{
	this->Cleanup();
}

std::string UCactusVirtualFile::GetErrorString(DWORD ErrorCode)
{
    char* msgBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ErrorCode, 0, (LPSTR)&msgBuffer, 0, nullptr);

    std::string message(msgBuffer, size);
    LocalFree(msgBuffer);
    return message;
}

void UCactusVirtualFile::Cleanup()
{
    if (MappedMemory)
    {
        UnmapViewOfFile(MappedMemory);
        MappedMemory = nullptr;
    }

    if (MappingHandle)
    {
        CloseHandle(MappingHandle);
        MappingHandle = nullptr;
    }

    MappedSize = 0;
}

void* UCactusVirtualFile::GetData() const
{
    return MappedMemory;
}

size_t UCactusVirtualFile::GetSize() const
{
    return MappedSize;
}

FString UCactusVirtualFile::GetFilePath()
{
    return this->VirtualFilePath;
}

bool UCactusVirtualFile::CreateVirtualFile(const TArray<uint8>& Buffer, FVector2D ImageRes, FName FileName)
{
    if (Buffer.IsEmpty())
    {
        return false;
    }

    if (ImageRes.X <= 0 || ImageRes.Y <= 0)
    {
        return false;
    }

	FString NameString = FileName.ToString();

    if (NameString.IsEmpty())
    {
        return false;
    }

	const FString TempPath = "Local\\" + NameString;

    const wchar_t* PathChar = *TempPath;
	const size_t BufferSize = Buffer.Num();
	//UE_LOG(LogTemp, Warning, TEXT("BufferSize :%d"), static_cast<int32>(BufferSize));

    this->MappingHandle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(BufferSize), PathChar);

    if (!this->MappingHandle)
    {
		FString ErrorTitle = "Failed to create file mapping at: " + TempPath + " : ";
        
        std::stringstream ErrorStream;
        ErrorStream << TCHAR_TO_UTF8(*ErrorTitle) << this->GetErrorString(GetLastError());
        UE_LOG(LogTemp, Error, TEXT("%s"), *FString(ErrorStream.str().c_str()));
        
        this->Cleanup();
        return false;
    }

    this->MappedMemory = MapViewOfFile(this->MappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, BufferSize);

    if (!this->MappedMemory)
    {
        std::stringstream ErrorStream;
        ErrorStream << "MapViewOfFile failed: " << this->GetErrorString(GetLastError());
        UE_LOG(LogTemp, Error, TEXT("%s"), *FString(ErrorStream.str().c_str()));
		
        this->Cleanup();
        return false;
    }

    FMemory::Memcpy(this->MappedMemory, Buffer.GetData(), BufferSize);
	this->MappedSize = BufferSize;
	this->VirtualFilePath = TempPath;

    return true;
}