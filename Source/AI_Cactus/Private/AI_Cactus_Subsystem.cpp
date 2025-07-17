// Fill out your copyright notice in the Description page of Project Settings.

#include "AI_Cactus_Subsystem.h"

void UCactusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCactusSubsystem::Deinitialize()
{
	if (Cactus_Context.IsValid())
	{
		//Cactus_Context->releaseMultimodal();
		Cactus_Context.Reset();
	}

	Super::Deinitialize();
}

bool UCactusSubsystem::Init_Cactus(int32 NumberThreads)
{
	if (Cactus_Context.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cactus Context is already initialized."));
		return false;
	}

	if (NumberThreads < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Number of threads must be at least 1."));
		return false;
	}

	if (this->Model_Path.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Model path is not set."));
		return false;
	}

	this->Cactus_Params.model.path = TCHAR_TO_UTF8(*this->Model_Path);
	this->Cactus_Params.n_ctx = 4096;
	this->Cactus_Params.n_batch = 512;
	this->Cactus_Params.n_gpu_layers = 99;
	this->Cactus_Params.cpuparams.n_threads = NumberThreads;

	this->Cactus_Params.n_cache_reuse = 256;
	this->Cactus_Params.n_keep = 32;

	this->Cactus_Params.sampling.temp = 0.7f;
	this->Cactus_Params.sampling.top_k = 40;
	this->Cactus_Params.sampling.top_p = 0.9f;
	this->Cactus_Params.sampling.penalty_repeat = 1.1f;

	this->Cactus_Params.antiprompt.push_back("<|im_end|>");

	this->Cactus_Context = MakeShared<cactus_context, ESPMode::ThreadSafe>();

	if (!this->Cactus_Context->loadModel(this->Cactus_Params))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Cactus model from path: %s"), *this->Model_Path);
		return false;
	}

	return true;
}

bool UCactusSubsystem::SetModelPath(const FString& Path)
{
	if (Path.IsEmpty())
	{
		return false;
	}

	this->Model_Path = Path;
	return true;
}

FString UCactusSubsystem::GetModelPath() const
{
	return this->Model_Path;
}

void UCactusSubsystem::GenerateText(FDelegateCactus DelegateCactus, FString Input, int32 MaxTokens)
{
	if (!Cactus_Context.IsValid())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Cactus context is not initialized."));
		return;
	}

	if (Input.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Input text is empty."));
		return;
	}

	std::string inputStr = TCHAR_TO_UTF8(*Input);
	std::string outputStr;

	this->Cactus_Context->generated_text.clear();
	this->Cactus_Context->params.prompt = inputStr;
	this->Cactus_Context->params.n_predict = MaxTokens;

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, Input, MaxTokens]()
		{

		}
	);
}