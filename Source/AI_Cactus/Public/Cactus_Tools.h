#pragma once
#include "Cactus_Includes.h"
#include "Cactus_Tools.generated.h"

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_FiveParams(FDelegateCactus, bool, bIsSuccessfull, FString, Out_Result, double, Out_TT, double, Out_TTF, int32, Out_Tokens);

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_OneParam(FDelegateCactusCounter, int32, Out_Counter);

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_DELEGATE_OneParam(FDelegateCactusSave, bool, bIsSuccessfull);

UCLASS()
class AI_CACTUS_API UCactusConversationSave : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> EmbdTokens;

};

USTRUCT(BlueprintType)
struct AI_CACTUS_API FCactusModelParams_LLM
{
	GENERATED_BODY()

public:

    // ==== Context & batching ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 ContextSize = 4096;  // Equivalent to params.n_ctx

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 BatchSize = 512;  // Equivalent to params.n_batch

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 GPULayers = 99;  // Equivalent to params.n_gpu_layers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 CPUThreads = 4;  // Equivalent to params.cpuparams.n_threads

    // ==== Cache & token control ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 CacheReuse = 256;  // Equivalent to params.n_cache_reuse

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 KeepTokens = 32;  // Equivalent to params.n_keep

    // ==== Sampling settings ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    float Temperature = 0.7f;  // Equivalent to params.sampling.temp

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    int32 TopK = 40;  // Equivalent to params.sampling.top_k

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    float TopP = 0.9f;  // Equivalent to params.sampling.top_p

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    float RepeatPenalty = 1.1f;  // Equivalent to params.sampling.penalty_repeat

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
	FString AntiPrompt = "<|im_end|>";  // Equivalent to params.anti_prompt

    bool bIsNumbersOkay() const
    {
        return ContextSize > 0 && BatchSize > 0 && GPULayers > 0 && CPUThreads > 0 && CacheReuse >= 0 && KeepTokens >= 0 && Temperature >= 0.0f && TopK >= 0 && TopP >= 0.0f && RepeatPenalty >= 1.0f;
	}
};

USTRUCT(BlueprintType)
struct AI_CACTUS_API FCactusModelParams_VLM
{
    GENERATED_BODY()

    // ==== Runtime Settings ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cactus|VLM")
    int32 ContextSize = 2048;   // Equivalent to params.n_ctx

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cactus|VLM")
    int32 BatchSize = 32;       // Equivalent to params.n_batch

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cactus|VLM")
    int32 GPULayers = 99;       // Equivalent to params.n_gpu_layers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cactus|VLM")
    int32 CPUThreads = 4;       // Equivalent to params.cpuparams.n_threads

    // ==== Multimodal Settings ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cactus|VLM")
    bool bUseGPUForMMProj = true;  // Use GPU for mmproj acceleration

    // ==== Token & Output Control ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cactus|VLM")
    int32 MaxTokens = 50;   // Equivalent to params.n_predict for VLM responses

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Cactus")
    FString AntiPrompt = "<|im_end|>";  // Equivalent to params.anti_prompt

    bool bIsNumbersOkay() const
    {
        return ContextSize > 0 && BatchSize > 0 && GPULayers > 0 && CPUThreads > 0 && MaxTokens > 0;
	}
};

struct cactus_context_vlm : public cactus::cactus_context
{
    virtual std::vector<uint8_t> Convert_Array(TArray<uint8_t> ImageData);
    virtual std::vector<uint8_t> BGRA_To_RGBA(const std::vector<uint8_t>& ImageData);
    virtual bool Load_Image_Buffer(const std::vector<uint8_t>& Buffer, uint32_t Width, uint32_t Height, bool bUseAlpha, bool bIsBGRA);
};