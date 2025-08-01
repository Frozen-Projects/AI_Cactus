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

bool ACactus_Manager_VLM::Init_Cactus(FCactusModelParams_VLM VLM_Params)
{
	if (Cactus_Context.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cactus Context is already initialized !"));
		return false;
	}

	if (!VLM_Params.bIsNumbersOkay())
	{
		UE_LOG(LogTemp, Warning, TEXT("Numeric values should bigger than 0 !"));
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
		this->Cactus_Params.n_ctx = VLM_Params.ContextSize;
		this->Cactus_Params.n_batch = VLM_Params.BatchSize;
		this->Cactus_Params.n_gpu_layers = VLM_Params.GPULayers;
		this->Cactus_Params.cpuparams.n_threads = VLM_Params.CPUThreads;

		if (!this->Cactus_Context->loadModel(this->Cactus_Params))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Cactus model from path: %s"), *this->Path_Model);
			return false;
		}

		if (!this->Path_MMProj.IsEmpty() && this->Path_MMProj != "MMPROJ_DISABLED")
		{
			this->Cactus_Context->initMultimodal(TCHAR_TO_UTF8(*this->Path_MMProj), VLM_Params.bUseGPUForMMProj);
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

void ACactus_Manager_VLM::GenerateResponseToImage(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, TArray<uint8> ImageData, FVector2D ImageSize, const FString& Question)
{
	if (!Cactus_Context.IsValid())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Cactus Context is not valid !"), -1, -1, -1);
		return;
	}

	if (ImageData.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Image data is empty !"), -1, -1, -1);
		return;
	}

	if (ImageSize.X <= 0 || ImageSize.Y <= 0)
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Image size shouldn't smaller 1 !"), -1, -1, -1);
		return;
	}

	if (Question.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Question text is empty !"), -1, -1, -1);
		return;
	}

	UWorld* World = GEngine->GetCurrentPlayWorld();

	if (!IsValid(World))
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("World is not valid !"), -1, -1, -1);
		return;
	}

	const FDateTime Counter_Start = FDateTime::Now();
	this->Delegate_Counter = FTimerDelegate::CreateLambda([DelegateCounter, Counter_Start]()
		{
			const FTimespan Duration = FDateTime::Now() - Counter_Start;
			DelegateCounter.ExecuteIfBound(FMath::TruncToInt32(Duration.GetTotalSeconds()));
		});

	World->GetTimerManager().SetTimer(this->Handle_Counter, this->Delegate_Counter, 1.0f, true);

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, DelegateCounter, ImageData, ImageSize, Question, World]()
		{

			AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, DelegateCounter, World]()
				{
					World->GetTimerManager().ClearTimer(this->Handle_Counter);
				}
			);
		}
	);
}