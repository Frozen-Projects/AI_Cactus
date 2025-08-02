#include "Cactus_Tools.h"

std::vector<uint8_t> cactus_context_vlm::Convert_Array(TArray<uint8_t> ImageData)
{
	if (ImageData.IsEmpty())
	{
		return std::vector<uint8_t>();
	}

	const size_t NumBytes = ImageData.Num();
	std::vector<uint8_t> TempBuffer;
	TempBuffer.resize(NumBytes);
	FMemory::Memcpy(TempBuffer.data(), ImageData.GetData(), NumBytes);

	return TempBuffer;
}

std::vector<uint8_t> cactus_context_vlm::BGRA_To_RGB(const std::vector<uint8_t>& ImageData)
{
	if (ImageData.empty())
	{
		return std::vector<uint8_t>();
	}

	const size_t NumBytes = ImageData.size();

	std::vector<uint8_t> TempBuffer;
	TempBuffer.resize(NumBytes);

	for (size_t i = 0, j = 0; i < ImageData.size(); i += 4, j += 3)
	{
		TempBuffer[j] = ImageData[i + 2];		// R
		TempBuffer[j + 1] = ImageData[i + 1];	// G
		TempBuffer[j + 2] = ImageData[i + 0];	// B
	}

	return TempBuffer;
}

bool cactus_context_vlm::Load_Image_Buffer(const std::vector<uint8_t>& Buffer, uint32_t Width, uint32_t Height, bool bIsDataBGRA)
{
	if (Buffer.size() == 0)
	{
		return false;
	}

	if (Width <= 0 || Height <= 0)
	{
		return false;
	}

	if (Buffer.size() != Width * Height * 3)
	{
		return false;
	}

	std::vector<uint8_t> ProcessedBuffer;

	if (bIsDataBGRA)
	{
		ProcessedBuffer = BGRA_To_RGB(Buffer);

		if (ProcessedBuffer.empty())
		{
			return false;
		}
	}

	else
	{
		ProcessedBuffer = Buffer;
	}

	mtmd_bitmap* Bitmap = mtmd_bitmap_init(Width, Height, reinterpret_cast<const unsigned char*>(ProcessedBuffer.data()));

	if (!Bitmap)
	{
		return false;
	}

	std::vector<const mtmd_bitmap*> media_bitmaps = { Bitmap };

	mtmd_input_chunks* chunks = mtmd_input_chunks_init();
	
	mtmd_input_text input_text;
	memset(&input_text, 0, sizeof(mtmd_input_text));
	input_text.text = params.prompt.c_str();
	input_text.add_special = true;
	input_text.parse_special = true;

	const int Result = mtmd_tokenize(mtmd_wrapper->mtmd_ctx, chunks, &input_text, media_bitmaps.data(), media_bitmaps.size());

	mtmd_bitmap_free(Bitmap);

	return Result == 0;
}