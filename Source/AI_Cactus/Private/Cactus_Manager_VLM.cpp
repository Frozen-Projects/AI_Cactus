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

	try
	{
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