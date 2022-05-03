#pragma once

#include "Hog/Core/PlatformDetection.h"

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

#include "vulkan/vulkan.h"

#include "Hog/Core/Base.h"

#include "Hog/Core/Log.h"

#include "Hog/Debug/Instrumentor.h"

#ifdef HG_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
