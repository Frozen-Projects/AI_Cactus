#include "Cactus_Tools.h"

std::vector<uint8_t> cactus_context_vlm::Convert_Array(TArray<uint8_t> ImageData)
{
	if (ImageData.Num() == 0)
	{
		return std::vector<uint8_t>();
	}

	const size_t NumBytes = ImageData.Num();
	std::vector<uint8_t> TempBuffer;
	TempBuffer.reserve(NumBytes);
	FMemory::Memcpy(TempBuffer.data(), ImageData.GetData(), NumBytes);

	return TempBuffer;
}

std::vector<uint8_t> cactus_context_vlm::BGRA_To_RGBA(const std::vector<uint8_t>& ImageData)
{
	const size_t NumBytes = ImageData.size();

	std::vector<uint8_t> TempBuffer;
	TempBuffer.reserve(NumBytes);

	for (int i = 0; i < NumBytes; i += 4)
	{
		uint8_t B = ImageData[i];
		uint8_t G = ImageData[i + 1];
		uint8_t R = ImageData[i + 2];
		uint8_t A = ImageData[i + 3];

		TempBuffer.push_back(R);
		TempBuffer.push_back(G);
		TempBuffer.push_back(B);
		TempBuffer.push_back(A);
	}

	return TempBuffer;
}

bool cactus_context_vlm::Load_Image_Buffer(const std::vector<uint8_t>& Buffer, uint32_t Width, uint32_t Height, bool bUseAlpha, bool bIsBGRA)
{
	if (Buffer.size() == 0)
	{
		return false;
	}

	if (Width <= 0 || Height <= 0)
	{
		return false;
	}

	if (Buffer.size() != Width * Height * (bUseAlpha ? 4 : 3))
	{
		return false;
	}

	std::vector<uint8_t> ProcessedBuffer;

	if (bIsBGRA)
	{
		ProcessedBuffer = BGRA_To_RGBA(Buffer);

		if (ProcessedBuffer.empty())
		{
			return false;
		}
	}

	else
	{
		ProcessedBuffer = Buffer;
	}

	return false;
}