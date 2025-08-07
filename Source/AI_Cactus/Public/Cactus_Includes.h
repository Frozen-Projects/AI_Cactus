#pragma once

#include "Kismet/GameplayStatics.h"

#include "JsonObjectWrapper.h"
#include "JsonUtilities.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

#include "GameFramework/SaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

THIRD_PARTY_INCLUDES_START
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#include "Windows/AllowWindowsPlatformTypes.h"
#include <memoryapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <algorithm>

#include "cactus.h"
#include "tools/mtmd/mtmd.h"
#include "json.hpp"
THIRD_PARTY_INCLUDES_END