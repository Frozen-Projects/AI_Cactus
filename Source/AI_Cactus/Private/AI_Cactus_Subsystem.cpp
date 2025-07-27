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

	try
	{
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

	catch (const std::exception& Exception)
	{
		UE_LOG(LogTemp, Error, TEXT("Exception occurred while initializing Cactus: %s"), UTF8_TO_TCHAR(Exception.what()));
		return false;
	}
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

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, Input, MaxTokens]()
		{
			const std::string inputStr = TCHAR_TO_UTF8(*Input);
			const std::chrono::steady_clock::time_point Start_Time = std::chrono::high_resolution_clock::now();

			this->Cactus_Context->generated_text.clear();
			this->Cactus_Context->params.prompt = inputStr;
			this->Cactus_Context->params.n_predict = MaxTokens;

			if (!this->Cactus_Context->initSampling())
			{
				AsyncTask(ENamedThreads::GameThread, [DelegateCactus]()
					{
						DelegateCactus.ExecuteIfBound(false, TEXT("Failed to initialize sampling parameters."));
					}
				);

				return;
			}

			this->Cactus_Context->beginCompletion();
			this->Cactus_Context->loadPrompt();

			bool First_Token = true;
			std::chrono::high_resolution_clock::time_point First_Token_Time;
			int NumTokens = 0;

			while (this->Cactus_Context->has_next_token && !this->Cactus_Context->is_interrupted)
			{
				cactus::completion_token_output Token_Output = this->Cactus_Context->doCompletion();
				
				if (Token_Output.tok == -1)
				{
					break;
				}

				if (First_Token)
				{
					First_Token_Time = std::chrono::high_resolution_clock::now();
					First_Token = false;
				}

				NumTokens++;
			}

			const std::chrono::steady_clock::time_point End_Time = std::chrono::high_resolution_clock::now();
			
			this->Cactus_Context->endCompletion();

			const std::chrono::milliseconds Total_Time = std::chrono::duration_cast<std::chrono::milliseconds>(End_Time - Start_Time);
			const std::chrono::milliseconds Time_To_First_Token = First_Token ? std::chrono::milliseconds(0) : std::chrono::duration_cast<std::chrono::milliseconds>(First_Token_Time - Start_Time);
			const FString Result = FString(UTF8_TO_TCHAR(this->Cactus_Context->generated_text.c_str()));

			AsyncTask(ENamedThreads::GameThread, [DelegateCactus, Result]()
				{
					DelegateCactus.ExecuteIfBound(true, Result);
				}
			);
		}
	);
}