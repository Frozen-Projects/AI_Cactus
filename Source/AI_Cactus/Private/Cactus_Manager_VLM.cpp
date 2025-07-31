// Fill out your copyright notice in the Description page of Project Settings.

#include "Cactus_Manager_VLM.h"

// Sets default values
ACactus_Manager_VLM::ACactus_Manager_VLM()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned.
void ACactus_Manager_VLM::BeginPlay()
{
	Super::BeginPlay();
}

// Called when the game starts or when destroyed.
void ACactus_Manager_VLM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (this->Cactus_Context.IsValid())
	{
		Cactus_Context.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame.
void ACactus_Manager_VLM::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ACactus_Manager_VLM::Init_Cactus(int32 NumberThreads, const FString& AntiPrompt)
{
	if (Cactus_Context.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cactus Context is already initialized !"));
		return false;
	}

	if (AntiPrompt.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AntiPrompt is empty !"));
		return false;
	}

	if (NumberThreads < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Number of threads must be at least 1 !"));
		return false;
	}

	if (this->Path_Model.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Model path is not set !"));
		return false;
	}

	try
	{
		this->Cactus_Params.model.path = TCHAR_TO_UTF8(*this->Path_Model);
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

		this->Cactus_Params.antiprompt.push_back(TCHAR_TO_UTF8(*AntiPrompt));

		this->Cactus_Context = MakeShared<cactus_context, ESPMode::ThreadSafe>();

		if (!this->Cactus_Context->loadModel(this->Cactus_Params))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Cactus model from path: %s"), *this->Path_Model);
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

bool ACactus_Manager_VLM::SetModelPath(const FString& In_Path_Model, const FString& In_Path_MMProj)
{
	if (In_Path_MMProj.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("MMProj path is empty !"));
		return false;
	}

	if (In_Path_MMProj != "MMPROJ_DISABLED")
	{
		FString Temp_MMProj = In_Path_MMProj;
		FPaths::MakePlatformFilename(Temp_MMProj);
		this->Path_MMProj = Temp_MMProj;
	}

	if (In_Path_Model.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Model path is empty !"));
		return false;
	}

	FString TempPath = In_Path_Model;
	FPaths::MakePlatformFilename(TempPath);

	this->Path_Model = TempPath;
	return true;
}

FString ACactus_Manager_VLM::GetModelPath() const
{
	return this->Path_Model;
}

FString ACactus_Manager_VLM::GetMMProjPath() const
{
	return this->Path_MMProj;
}