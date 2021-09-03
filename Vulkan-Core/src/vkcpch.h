#pragma once

#include "VulkanCore/Core/PlatformDetection.h"

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "VulkanCore/Core/Base.h"

#include "VulkanCore/Core/Log.h"

#include "VulkanCore/Debug/Instrumentor.h"

#ifdef VKC_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
