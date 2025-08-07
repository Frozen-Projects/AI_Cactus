#include "Cactus_Tools.h"

FVirtualFileMapping::FVirtualFileMapping()
{

}

FVirtualFileMapping::~FVirtualFileMapping()
{
	this->Cleanup();
}

std::string FVirtualFileMapping::GetErrorString(DWORD ErrorCode)
{
    char* msgBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ErrorCode, 0, (LPSTR)&msgBuffer, 0, nullptr);

    std::string message(msgBuffer, size);
    LocalFree(msgBuffer);
    return message;
}

void FVirtualFileMapping::Cleanup()
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

void* FVirtualFileMapping::GetData() const
{
    return MappedMemory;
}

size_t FVirtualFileMapping::GetSize() const
{
    return MappedSize;
}

bool FVirtualFileMapping::CreateVirtualImageFile(const TArray<uint8>& ImageBuffer, FVector2D ImageRes, FName FileName)
{
    if (ImageBuffer.IsEmpty())
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

	const FString VirtualPath = "Local\\" + NameString;

    const wchar_t* Path = *VirtualPath;
	const size_t BufferSize = ImageBuffer.Num();
	//UE_LOG(LogTemp, Warning, TEXT("BufferSize :%d"), static_cast<int32>(BufferSize));

    this->MappingHandle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(BufferSize), Path);

    if (!this->MappingHandle)
    {
		FString ErrorTitle = "Failed to create file mapping at: " + VirtualPath + " : ";
        
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

    FMemory::Memcpy(this->MappedMemory, ImageBuffer.GetData(), BufferSize);
	this->MappedSize = BufferSize;

    return true;
}

UCactusImage::UCactusImage()
{
	this->CactusImageMapping = new FVirtualFileMapping();
}

void UCactusImage::BeginDestroy()
{
    if (this->CactusImageMapping)
    {
        delete this->CactusImageMapping;
        this->CactusImageMapping = nullptr;
    }

    Super::BeginDestroy();
}

TArray<uint8> UCactusImage::GetVirtualData()
{
    if (!this->CactusImageMapping)
    {
        return TArray<uint8>();
    }

    if (!this->CactusImageMapping->GetData())
    {
        return TArray<uint8>();
    }

    TArray<uint8> Result;
    SIZE_T Size = this->CactusImageMapping->GetSize();
    Result.SetNumUninitialized(Size);
    FMemory::Memcpy(Result.GetData(), this->CactusImageMapping->GetData(), Size);

    return Result;
}