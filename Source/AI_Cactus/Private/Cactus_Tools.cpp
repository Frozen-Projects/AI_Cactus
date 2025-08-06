#include "Cactus_Tools.h"

static std::string fnv_hash(const uint8_t* data, size_t len) 
{
    const uint64_t fnv_prime = 0x100000001b3ULL;
    uint64_t hash = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= fnv_prime;
    }
    return std::to_string(hash);
}

FVirtualFileMapping::FVirtualFileMapping()
{

}

FVirtualFileMapping::~FVirtualFileMapping()
{
	this->Cleanup();
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

SIZE_T FVirtualFileMapping::GetSize() const
{
    return MappedSize;
}

bool FVirtualFileMapping::CreateVirtualImageFile(const TArray<uint8>& ImageBuffer, FVector2D ImageRes, const FString& InPath)
{
    if (ImageBuffer.IsEmpty())
    {
        return false;
    }

    if (ImageRes.X <= 0 || ImageRes.Y <= 0)
    {
        return false;
    }

    if (InPath.IsEmpty())
    {
        return false;
    }

    const wchar_t* Path = *InPath;
	const size_t BufferSize = ImageBuffer.Num();
    this->MappingHandle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(BufferSize), Path);

    if (!this->MappingHandle)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create file mapping: %s"), *InPath);
        
        this->Cleanup();
        return false;
    }

    this->MappedMemory = MapViewOfFile(this->MappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, BufferSize);

    if (!this->MappedMemory)
    {
        std::stringstream ErrorStream;
        ErrorStream << "MapViewOfFile failed: " << GetLastError();
        UE_LOG(LogTemp, Error, TEXT("%s"), *FString(ErrorStream.str().c_str()));
		
        this->Cleanup();
        return false;
    }

    FMemory::Memcpy(this->MappedMemory, ImageBuffer.GetData(), BufferSize);

    return true;
}