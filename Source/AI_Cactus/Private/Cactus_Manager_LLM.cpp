// Fill out your copyright notice in the Description page of Project Settings.

#include "Cactus_Manager_LLM.h"

// Sets default values.
ACactus_Manager_LLM::ACactus_Manager_LLM()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned.
void ACactus_Manager_LLM::BeginPlay()
{
	Super::BeginPlay();
}

// Called when the game starts or when destroyed.
void ACactus_Manager_LLM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (this->Cactus_Context.IsValid())
	{
		this->Cactus_Context.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame.
void ACactus_Manager_LLM::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ACactus_Manager_LLM::Init_Cactus(FCactusModelParams_LLM LLM_Params)
{
	if (Cactus_Context.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cactus Context is already initialized !"));
		return false;
	}

	if (LLM_Params.AntiPrompt.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AntiPrompt is empty !"));
		return false;
	}

	if (!LLM_Params.bIsNumbersOkay())
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
		this->Cactus_Params.n_ctx = LLM_Params.ContextSize;
		this->Cactus_Params.n_batch = LLM_Params.BatchSize;
		this->Cactus_Params.n_gpu_layers = LLM_Params.GPULayers;
		this->Cactus_Params.cpuparams.n_threads = LLM_Params.CPUThreads;

		this->Cactus_Params.n_cache_reuse = LLM_Params.CacheReuse;
		this->Cactus_Params.n_keep = LLM_Params.KeepTokens;

		this->Cactus_Params.sampling.temp = LLM_Params.Temperature;
		this->Cactus_Params.sampling.top_k = LLM_Params.TopK;
		this->Cactus_Params.sampling.top_p = LLM_Params.TopP;
		this->Cactus_Params.sampling.penalty_repeat = LLM_Params.RepeatPenalty;

		this->Cactus_Params.antiprompt.push_back(TCHAR_TO_UTF8(*LLM_Params.AntiPrompt));

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

bool ACactus_Manager_LLM::SetModelPath(const FString& In_Path)
{
	if (In_Path.IsEmpty())
	{
		return false;
	}

	FString TempPath = In_Path;
	FPaths::MakePlatformFilename(TempPath);

	this->Path_Model = TempPath;
	return true;
}

FString ACactus_Manager_LLM::GetModelPath() const
{
	return this->Path_Model;
}

void ACactus_Manager_LLM::GenerateText(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, FString Input, int32 MaxTokens)
{
	if (!Cactus_Context.IsValid())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Cactus Context is not valid !"), -1, -1, -1);
		return;
	}

	if (Input.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Input text is empty !"), -1, -1, -1);
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
	
	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, DelegateCounter, Input, MaxTokens, World]()
		{
			const std::chrono::high_resolution_clock::time_point Start_Time = std::chrono::high_resolution_clock::now();
			const std::string PromptStr = TCHAR_TO_UTF8(*Input);

			this->Cactus_Context->generated_text.clear();
			this->Cactus_Context->params.prompt = PromptStr;
			this->Cactus_Context->params.n_predict = MaxTokens;

			if (!this->Cactus_Context->initSampling())
			{
				AsyncTask(ENamedThreads::GameThread, [DelegateCactus]()
					{
						DelegateCactus.ExecuteIfBound(false, TEXT("Failed to initialize sampling parameters !"), -1, -1, -1);
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
				const cactus::completion_token_output Token_Output = this->Cactus_Context->doCompletion();
				
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

			this->Cactus_Context->endCompletion();
			
			const FString Result = UTF8_TO_TCHAR(this->Cactus_Context->generated_text.c_str());
			
			// Time to first token
			const std::chrono::milliseconds TTFT = First_Token ? std::chrono::milliseconds(0) : std::chrono::duration_cast<std::chrono::milliseconds>(First_Token_Time - Start_Time);
			const std::chrono::milliseconds Total_Time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - Start_Time);
			
			const double TT_Seconds = std::chrono::duration_cast<std::chrono::duration<double>>(Total_Time).count();
			const double TTFT_Seconds = std::chrono::duration_cast<std::chrono::duration<double>>(TTFT).count();

			AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, Result, TT_Seconds, TTFT_Seconds, NumTokens, World]()
				{				
					World->GetTimerManager().ClearTimer(this->Handle_Counter);
					DelegateCactus.ExecuteIfBound(true, Result, TT_Seconds, TTFT_Seconds, NumTokens);
				}
			);
		}
	);
}

void ACactus_Manager_LLM::RunConversation(FDelegateCactus DelegateCactus, FDelegateCactusCounter DelegateCounter, FString Input, int32 MaxTokens, const FString& Assistant_Marker)
{
	if (!Cactus_Context.IsValid())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Cactus Context is not valid !"), -1, -1, -1);
		return;
	}

	if (Input.IsEmpty())
	{
		DelegateCactus.ExecuteIfBound(false, TEXT("Input text is empty !"), -1, -1, -1);
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
	});

	World->GetTimerManager().SetTimer(this->Handle_Counter, this->Delegate_Counter, 1.0f, true);

	AsyncTask(ENamedThreads::AnyNormalThreadHiPriTask, [this, DelegateCactus, DelegateCounter, Input, MaxTokens, Assistant_Marker, World]()
		{
			TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			JsonObject->SetStringField("role", "user");
			JsonObject->SetStringField("content", Input);
			
			TArray<TSharedPtr<FJsonValue>> JsonArray;
			JsonArray.Add(MakeShared<FJsonValueObject>(JsonObject));

			FString OutputString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonArray, Writer);
			UE_LOG(LogTemp, Log, TEXT("Conversation JSON String:\n%s"), *OutputString);
			
			const std::string RawString = TCHAR_TO_UTF8(*OutputString);

			std::string Prompt;

			if (this->Cactus_Context->embd.empty()) 
			{
				Prompt = this->Cactus_Context->getFormattedChat(RawString, "");
			}

			else 
			{
				const std::string user_part = this->Cactus_Context->getFormattedChat(RawString, "");
				const size_t assistant_start = user_part.find(TCHAR_TO_UTF8(*Assistant_Marker));
				
				if (assistant_start != std::string::npos)
				{
					Prompt = user_part.substr(0, assistant_start) + TCHAR_TO_UTF8(*Assistant_Marker) + "\n";
				}

				else 
				{
					Prompt = user_part;
				}
			}

			this->Cactus_Context->generated_text.clear();
			this->Cactus_Context->params.prompt = Prompt;
			this->Cactus_Context->params.n_predict = MaxTokens;

			if (!this->Cactus_Context->initSampling())
			{
				AsyncTask(ENamedThreads::GameThread, [DelegateCactus]()
					{
						DelegateCactus.ExecuteIfBound(false, TEXT("Failed to initialize sampling parameters !"), -1, -1, -1);
					}
				);

				return;
			}

			this->Cactus_Context->beginCompletion();
			this->Cactus_Context->loadPrompt();

			const std::chrono::steady_clock::time_point Start_Time = std::chrono::high_resolution_clock::now();
			bool First_Token = true;
			std::chrono::high_resolution_clock::time_point First_Token_Time;
			int NumTokens = 0;

			while (this->Cactus_Context->has_next_token && !this->Cactus_Context->is_interrupted)
			{
				const cactus::completion_token_output Token_Output = this->Cactus_Context->doCompletion();
				
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

			this->Cactus_Context->endCompletion();

			const FString Result = UTF8_TO_TCHAR(this->Cactus_Context->generated_text.c_str());

			const std::chrono::milliseconds Total_Time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - Start_Time);
			const std::chrono::milliseconds TTFT = First_Token ? std::chrono::milliseconds(0) : std::chrono::duration_cast<std::chrono::milliseconds>(First_Token_Time - Start_Time);

			const double TT_Seconds = std::chrono::duration_cast<std::chrono::duration<double>>(Total_Time).count();
			const double TTFT_Seconds = std::chrono::duration_cast<std::chrono::duration<double>>(TTFT).count();

			AsyncTask(ENamedThreads::GameThread, [this, DelegateCactus, Result, TT_Seconds, TTFT_Seconds, NumTokens, World]()
				{
					World->GetTimerManager().ClearTimer(this->Handle_Counter);
					DelegateCactus.ExecuteIfBound(true, Result, TT_Seconds, TTFT_Seconds, NumTokens);
				}
			);
		}
	);
}

bool ACactus_Manager_LLM::ClearConversation()
{
	if (!this->Cactus_Context.IsValid())
	{
		return false;
	}

	this->Cactus_Context->embd.clear();
	this->Cactus_Context->generated_text.clear();

	return true;
}

void ACactus_Manager_LLM::ExportConversation(FDelegateCactusSave DelegateSave, const FString& SavePath)
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

void ACactus_Manager_LLM::ImportConversation(FDelegateCactusSave DelegateLoad, const FString& FilePath, const FString& Assistant_Marker)
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