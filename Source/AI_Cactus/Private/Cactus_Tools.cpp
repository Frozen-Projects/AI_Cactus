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
    const size_t NumPixels = NumBytes / 4; // BGRA = 4 bytes per pixel
    
    std::vector<uint8_t> TempBuffer;
    TempBuffer.resize(NumPixels * 3); // RGB = 3 bytes per pixel

	for (size_t i = 0, j = 0; i < NumBytes; i += 4, j += 3)
	{
		TempBuffer[j] = ImageData[i + 2];		// R
		TempBuffer[j + 1] = ImageData[i + 1];	// G
		TempBuffer[j + 2] = ImageData[i + 0];	// B
	}

	UE_LOG(LogTemp, Log, TEXT("Converted BGRA to RGB: %zu bytes"), TempBuffer.size());
	return TempBuffer;
}

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

void cactus_context_vlm::loadPrompt(const std::vector<uint8_t>& BufferRGB, int32 Width, int32 Height)
{
    if (!isMultimodalEnabled()) {
        throw std::runtime_error("Multimodal is not enabled but image buffer is provided");
    }

    if (BufferRGB.empty() || Width <= 0 || Height <= 0) {
        throw std::runtime_error("Invalid image buffer or dimensions");
    }

    // ✅ Check RGB buffer size
    size_t expectedSize = static_cast<size_t>(Width) * static_cast<size_t>(Height) * 3;
    if (BufferRGB.size() != expectedSize) {
        throw std::runtime_error("RGB buffer size mismatch: expected " + std::to_string(expectedSize) +
            ", got " + std::to_string(BufferRGB.size()));
    }

    // ✅ Check that <image> marker exists in prompt
    std::string default_media_marker = mtmd_default_marker();  // Call the function
    if (params.prompt.find(default_media_marker) == std::string::npos) {
        throw std::runtime_error("Prompt missing image marker: " + default_media_marker);
    }

    // Create bitmap from RGB buffer
    mtmd::bitmap bmp(mtmd_helper_bitmap_init_from_buf(BufferRGB.data(), BufferRGB.size()));
    if (!bmp.ptr) {
        throw std::runtime_error("Failed to load image from RGB buffer");
    }

    // Assign ID based on hash
    std::string hash = fnv_hash(bmp.data(), bmp.n_bytes());
    bmp.set_id(hash.c_str());

    mtmd::bitmaps bitmaps;
    bitmaps.entries.push_back(std::move(bmp));

    // Ensure prompt contains <image> marker
    std::string full_prompt = params.prompt;
    
    if (full_prompt.find(default_media_marker) == std::string::npos) {
        full_prompt += " ";
        full_prompt += default_media_marker;
    }

    // Tokenize with image
    auto bitmaps_c_ptr = bitmaps.c_ptr();
    mtmd_input_chunks* chunks = mtmd_input_chunks_init();
    if (!chunks) {
        throw std::runtime_error("Failed to initialize input chunks");
    }

    mtmd_input_text input_text;
    input_text.text = full_prompt.c_str();
    input_text.add_special = true;
    input_text.parse_special = true;

    int32_t res = mtmd_tokenize(mtmd_wrapper->mtmd_ctx, chunks, &input_text, bitmaps_c_ptr.data(), bitmaps_c_ptr.size());
    if (res != 0) {
        mtmd_input_chunks_free(chunks);
        throw std::runtime_error("Failed to tokenize text and image buffer");
    }

    // Gather tokens
    size_t num_chunks = mtmd_input_chunks_size(chunks);
    embd.clear();
    size_t total_tokens = 0;

    for (size_t i = 0; i < num_chunks; i++) {
        const mtmd_input_chunk* chunk = mtmd_input_chunks_get(chunks, i);
        mtmd_input_chunk_type chunk_type = mtmd_input_chunk_get_type(chunk);

        if (chunk_type == MTMD_INPUT_CHUNK_TYPE_TEXT) {
            size_t n_tokens;
            const llama_token* tokens = mtmd_input_chunk_get_tokens_text(chunk, &n_tokens);
            embd.insert(embd.end(), tokens, tokens + n_tokens);
            total_tokens += n_tokens;
        }
        else if (chunk_type == MTMD_INPUT_CHUNK_TYPE_IMAGE) {
            size_t n_pos = mtmd_input_chunk_get_n_pos(chunk);
            embd.insert(embd.end(), n_pos, LLAMA_TOKEN_NULL);
            total_tokens += n_pos;
        }
    }

    num_prompt_tokens = total_tokens;
    if (params.n_keep < 0) params.n_keep = (int)num_prompt_tokens;
    params.n_keep = std::min(n_ctx > 4 ? n_ctx - 4 : 0, params.n_keep);
    params.n_keep = std::max(0, params.n_keep);

    mtmd_input_chunks_free(chunks);
}