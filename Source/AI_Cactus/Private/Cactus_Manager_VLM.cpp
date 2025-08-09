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

std::string ACactus_Manager_VLM::JsonMaker(const FString& Question) const
{
	// We used nlohmann because non-UE developers can understand it easily.

	nlohmann::json JsonObject = json::array(
		{
			{
				{ "role", "user" },
				{ "content", json::array(
					{
						{
							{ "type", "text" },
							{ "text", TCHAR_TO_UTF8(*Question)}
						}
					})
				}
			}
		});

	const std::string JsonMessage = JsonObject.dump();
	UE_LOG(LogTemp, Log, TEXT("Conversation JSON String:\n%s"), UTF8_TO_TCHAR(JsonMessage.c_str()));

	return JsonMessage;

	/*
	* Sample JSON Structure:
	[
		{
			"role": "user",
			"content": [
				{
					"type": "text",
					"text": "What do you see in this image ?"
				}
			]
		}
	]
	*/
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

		this->Cactus_Context = MakeShared<cactus_context, ESPMode::ThreadSafe>();

		if (!this->Cactus_Context->loadModel(this->Cactus_Params))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Cactus model from path: %s"), *this->Path_Model);
			return false;
		}

		if (!this->Path_MMProj.IsEmpty())
		{
			if (!this->Cactus_Context->initMultimodal(TCHAR_TO_UTF8(*this->Path_MMProj), VLM_Params.bUseGPUForMMProj))
			{
				UE_LOG(LogTemp, Error, TEXT("Model doesn't support multi-modal !"));
			}
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
	if (In_Path_Model.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Model path is empty !"));
		return false;
	}

	FString TempPath = In_Path_Model;
	FPaths::MakePlatformFilename(TempPath);
	this->Path_Model = TempPath;

	if (!In_Path_MMProj.IsEmpty())
	{
		FString Temp_MMProj = In_Path_MMProj;
		FPaths::MakePlatformFilename(Temp_MMProj);
		this->Path_MMProj = Temp_MMProj;
	}

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

void ACactus_Manager_VLM::Response_Image_Path(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, FString FilePath, const FString& Question, int32 MaxTokens)
{
	if (!Cactus_Context.IsValid())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Cactus Context is not valid !"), -1, -1, -1);
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

	FPaths::MakeStandardFilename(FilePath);

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, DelegateCounter, FilePath, Question, MaxTokens, World]()
		{
			FString TempPath = FilePath;
			FPaths::MakePlatformFilename(TempPath);
			const std::string PathString = TCHAR_TO_UTF8(*TempPath);

			std::vector<std::string> ImagePaths;

			if (FPaths::FileExists(FilePath))
			{
				ImagePaths.push_back(PathString);
			}

			/*
			* Process the image data and prepare the prompt for the model.
			*/

			std::string JsonMessage;
			std::string FormattedPrompt;

			try
			{
				JsonMessage = this->JsonMaker(Question);
				FormattedPrompt = this->Cactus_Context->getFormattedChat(JsonMessage, "");
			}

			catch (const std::exception& Exception)
			{
				FormattedPrompt = TCHAR_TO_UTF8(*Question);
				UE_LOG(LogTemp, Error, TEXT("Exception occurred while formatting chat. We used fallback method: %s"), UTF8_TO_TCHAR(Exception.what()));
			}

			this->Cactus_Context->params.prompt = FormattedPrompt;
			this->Cactus_Context->params.n_predict = MaxTokens;

			// Init sampling
			if (!this->Cactus_Context->initSampling())
			{
				AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, World]()
					{
						World->GetTimerManager().ClearTimer(this->Handle_Counter);
						DelegateCactus.ExecuteIfBound(false, TEXT("Failed to initialize sampling!"), -1, -1, -1);
					}
				);

				return;
			}

			// Rewind context for correct image+text generation
			this->Cactus_Context->rewind();

			// Start completion
			this->Cactus_Context->generated_text.clear();
			this->Cactus_Context->beginCompletion();
			this->Cactus_Context->loadPrompt(ImagePaths);

			const std::chrono::steady_clock::time_point StartTime = std::chrono::high_resolution_clock::now();
			bool FirstToken = true;
			std::chrono::high_resolution_clock::time_point FirstTokenTime;
			int NumTokens = 0;

			while (this->Cactus_Context->has_next_token && !this->Cactus_Context->is_interrupted)
			{
				const cactus::completion_token_output Token_Output = this->Cactus_Context->doCompletion();

				if (Token_Output.tok == -1)
				{
					break;
				}

				if (FirstToken)
				{
					FirstTokenTime = std::chrono::high_resolution_clock::now();
					FirstToken = false;
				}

				NumTokens++;
			}

			this->Cactus_Context->endCompletion();

			const std::chrono::milliseconds TotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - StartTime);
			const std::chrono::milliseconds TTFT = FirstToken ? std::chrono::milliseconds(0) : std::chrono::duration_cast<std::chrono::milliseconds>(FirstTokenTime - StartTime);

			const FString Result = UTF8_TO_TCHAR(this->Cactus_Context->generated_text.c_str());

			AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, World, Result, TotalTime, TTFT, NumTokens]()
				{
					while (!IsValid(World))
					{
						UE_LOG(LogTemp, Warning, TEXT("World is not valid, waiting for a valid world..."));
					}

					World->GetTimerManager().ClearTimer(this->Handle_Counter);
					DelegateCactus.ExecuteIfBound(true, Result, TotalTime.count() / 1000.0, TTFT.count() / 1000.0, NumTokens);
				}
			);
		}
	);
}

void ACactus_Manager_VLM::Conversation_Image_Path(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, FString FilePath, const FString& Question, const FString& Assistant_Marker, int32 MaxTokens)
{
	if (!Cactus_Context.IsValid())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Cactus Context is not valid !"), -1, -1, -1);
		return;
	}

	if (Question.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Question text is empty !"), -1, -1, -1);
		return;
	}

	if (Assistant_Marker.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Assistant_Marker is empty !"), -1, -1, -1);
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
		}
	);

	World->GetTimerManager().SetTimer(this->Handle_Counter, this->Delegate_Counter, 1.0f, true);

	FPaths::MakeStandardFilename(FilePath);

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, FilePath, Question, MaxTokens, Assistant_Marker, World]()
		{
			FString TempPath = FilePath;
			FPaths::MakePlatformFilename(TempPath);
			const std::string PathString = TCHAR_TO_UTF8(*TempPath);

			std::vector<std::string> ImagePaths;

			if (FPaths::FileExists(FilePath))
			{
				ImagePaths.push_back(PathString);
			}

			std::string JsonMessage;
			std::string Prompt;

			try
			{
				JsonMessage = this->JsonMaker(Question);

				if (this->Cactus_Context->embd.empty())
				{
					Prompt = this->Cactus_Context->getFormattedChat(JsonMessage, "");
				}

				else
				{
					const std::string UserPart = this->Cactus_Context->getFormattedChat(JsonMessage, "");
					const size_t AssistantStart = UserPart.find(TCHAR_TO_UTF8(*Assistant_Marker));
					
					if (AssistantStart != std::string::npos)
					{
						Prompt = UserPart.substr(0, AssistantStart) + TCHAR_TO_UTF8(*Assistant_Marker) + "\n";
					}

					else
					{
						Prompt = UserPart;
					}
				}
			}

			catch (const std::exception& Exception)
			{
				Prompt = TCHAR_TO_UTF8(*Question);
				UE_LOG(LogTemp, Error, TEXT("Exception occurred while formatting chat. Fallback is used. %s"), UTF8_TO_TCHAR(Exception.what()));
			}

			this->Cactus_Context->generated_text.clear();
			this->Cactus_Context->params.prompt = Prompt;
			this->Cactus_Context->params.n_predict = MaxTokens;

			if (!this->Cactus_Context->initSampling())
			{
				AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, World]()
					{
						World->GetTimerManager().ClearTimer(this->Handle_Counter);
						DelegateCactus.ExecuteIfBound(false, TEXT("Failed to initialize sampling!"), -1, -1, -1);
					}
				);

				return;
			}

			this->Cactus_Context->beginCompletion();
			this->Cactus_Context->loadPrompt(ImagePaths);

			const auto StartTime = std::chrono::high_resolution_clock::now();
			bool FirstToken = true;
			std::chrono::high_resolution_clock::time_point FirstTokenTime;
			int NumTokens = 0;

			while (this->Cactus_Context->has_next_token && !this->Cactus_Context->is_interrupted)
			{
				const cactus::completion_token_output Token = this->Cactus_Context->doCompletion();
				if (Token.tok == -1) break;

				if (FirstToken)
				{
					FirstTokenTime = std::chrono::high_resolution_clock::now();
					FirstToken = false;
				}

				++NumTokens;
			}

			this->Cactus_Context->endCompletion();

			const auto TotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - StartTime);
			const auto TTFT = FirstToken ? std::chrono::milliseconds(0) : std::chrono::duration_cast<std::chrono::milliseconds>(FirstTokenTime - StartTime);

			const FString Result = UTF8_TO_TCHAR(this->Cactus_Context->generated_text.c_str());

			AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, World, Result, TotalTime, TTFT, NumTokens]()
				{
					World->GetTimerManager().ClearTimer(this->Handle_Counter);
					DelegateCactus.ExecuteIfBound(true, Result, TotalTime.count() / 1000.0, TTFT.count() / 1000.0, NumTokens);
				}
			);
		}
	);
}

void ACactus_Manager_VLM::Response_Image_Buffer(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, TArray<uint8> ImageData, FVector2D ImageSize, const FString& Question, int32 MaxTokens)
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

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, DelegateCounter, ImageData, ImageSize, Question, MaxTokens, World]()
		{
			/*
			* TODO:
			* We need buffer import feature for Cactus.
			* Process the image data and prepare the prompt for the model.
			*/

			std::string JsonMessage;
			std::string FormattedPrompt;

			try
			{
				JsonMessage = this->JsonMaker(Question);
				FormattedPrompt = this->Cactus_Context->getFormattedChat(JsonMessage, "");
			}

			catch (const std::exception& Exception)
			{
				FormattedPrompt = TCHAR_TO_UTF8(*Question);
				UE_LOG(LogTemp, Error, TEXT("Exception occurred while formatting chat. We used fallback method: %s"), UTF8_TO_TCHAR(Exception.what()));
			}

			this->Cactus_Context->params.prompt = FormattedPrompt;
			this->Cactus_Context->params.n_predict = MaxTokens;

			// Init sampling
			if (!this->Cactus_Context->initSampling())
			{
				AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, World]()
					{
						World->GetTimerManager().ClearTimer(this->Handle_Counter);
						DelegateCactus.ExecuteIfBound(false, TEXT("Failed to initialize sampling!"), -1, -1, -1);
					}
				);

				return;
			}

			// Rewind context for correct image+text generation
			this->Cactus_Context->rewind();

			// Start completion
			this->Cactus_Context->generated_text.clear();
			this->Cactus_Context->beginCompletion();
			//this->Cactus_Context->loadPrompt();

			const std::chrono::steady_clock::time_point StartTime = std::chrono::high_resolution_clock::now();
			bool FirstToken = true;
			std::chrono::high_resolution_clock::time_point FirstTokenTime;
			int NumTokens = 0;

			while (this->Cactus_Context->has_next_token && !this->Cactus_Context->is_interrupted)
			{
				const cactus::completion_token_output Token_Output = this->Cactus_Context->doCompletion();

				if (Token_Output.tok == -1)
				{
					break;
				}

				if (FirstToken)
				{
					FirstTokenTime = std::chrono::high_resolution_clock::now();
					FirstToken = false;
				}

				NumTokens++;
			}

			this->Cactus_Context->endCompletion();

			const std::chrono::milliseconds TotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - StartTime);
			const std::chrono::milliseconds TTFT = FirstToken ? std::chrono::milliseconds(0) : std::chrono::duration_cast<std::chrono::milliseconds>(FirstTokenTime - StartTime);

			const FString Result = UTF8_TO_TCHAR(this->Cactus_Context->generated_text.c_str());

			/*
			* Cleanup up virtual file.
			*/

			AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, World, Result, TotalTime, TTFT, NumTokens]()
				{
					while (!IsValid(World))
					{
						UE_LOG(LogTemp, Warning, TEXT("World is not valid, waiting for a valid world..."));
					}

					World->GetTimerManager().ClearTimer(this->Handle_Counter);
					DelegateCactus.ExecuteIfBound(true, Result, TotalTime.count() / 1000.0, TTFT.count() / 1000.0, NumTokens);
				}
			);
		}
	);
}

void ACactus_Manager_VLM::ExportConversation(FDelegateCactusSave DelegateSave, const FString& SavePath)
{
	if (!this->Cactus_Context.IsValid())
	{
		DelegateSave.ExecuteIfBound(false);
		return;
	}

	if (SavePath.IsEmpty() || !FPaths::DirectoryExists(FPaths::GetPath(SavePath)))
	{
		DelegateSave.ExecuteIfBound(false);
		return;
	}

	const size_t TokenSize = this->Cactus_Context->embd.size();

	if (TokenSize <= 0)
	{
		DelegateSave.ExecuteIfBound(false);
		return;
	}

	UCactusConversationSave* SaveObject = Cast<UCactusConversationSave>(UGameplayStatics::CreateSaveGameObject(UCactusConversationSave::StaticClass()));

	if (!IsValid(SaveObject))
	{
		DelegateSave.ExecuteIfBound(false);
		return;
	}

	SaveObject->EmbdTokens.SetNumUninitialized(TokenSize);
	FMemory::Memcpy(SaveObject->EmbdTokens.GetData(), Cactus_Context->embd.data(), TokenSize * sizeof(int32));

	AsyncTask(ENamedThreads::AnyNormalThreadNormalTask, [DelegateSave, SaveObject, SavePath]()
		{
			TArray<uint8> SaveData;
			FMemoryWriter MemoryWriter(SaveData, true);
			FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, false);
			SaveObject->Serialize(Archive);

			if (FFileHelper::SaveArrayToFile(SaveData, *SavePath))
			{
				AsyncTask(ENamedThreads::GameThread, [DelegateSave]()
					{
						DelegateSave.ExecuteIfBound(true);
					}
				);

				return;
			}

			else
			{
				AsyncTask(ENamedThreads::GameThread, [DelegateSave]()
					{
						DelegateSave.ExecuteIfBound(false);
					}
				);

				return;
			}
		}
	);
}

void ACactus_Manager_VLM::ImportConversation(FDelegateCactusSave DelegateLoad, const FString& FilePath, const FString& Assistant_Marker)
{
	if (!this->Cactus_Context.IsValid())
	{
		DelegateLoad.ExecuteIfBound(false);
		return;
	}

	FString TempPath = FilePath;
	FPaths::NormalizeFilename(TempPath);

	if (TempPath.IsEmpty() || !FPaths::FileExists(TempPath))
	{
		DelegateLoad.ExecuteIfBound(false);
		return;
	}

	if (Assistant_Marker.IsEmpty())
	{
		DelegateLoad.ExecuteIfBound(false);
		return;
	}

	TSubclassOf<UCactusConversationSave> SaveGameClass = UCactusConversationSave::StaticClass();
	UCactusConversationSave* LoadedGame = NewObject<UCactusConversationSave>(GetTransientPackage(), SaveGameClass);

	if (!IsValid(LoadedGame))
	{
		DelegateLoad.ExecuteIfBound(false);
		return;
	}

	AsyncTask(ENamedThreads::AnyNormalThreadNormalTask, [this, DelegateLoad, TempPath, LoadedGame]()
		{
			TArray<uint8> LoadData;
			if (!FFileHelper::LoadFileToArray(LoadData, *TempPath))
			{
				AsyncTask(ENamedThreads::GameThread, [DelegateLoad]()
					{
						DelegateLoad.ExecuteIfBound(false);
					}
				);

				return;
			}

			FMemoryReader MemoryReader(LoadData, true);
			FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
			LoadedGame->Serialize(Archive);

			const size_t TokenSize = LoadedGame->EmbdTokens.Num();
			this->Cactus_Context->embd.resize(TokenSize);
			FMemory::Memcpy(Cactus_Context->embd.data(), LoadedGame->EmbdTokens.GetData(), TokenSize * sizeof(int32));

			AsyncTask(ENamedThreads::GameThread, [DelegateLoad]()
				{
					DelegateLoad.ExecuteIfBound(true);
				}
			);
		}
	);
}