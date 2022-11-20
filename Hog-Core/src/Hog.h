#pragma once

// For use by Hog applications

#include "Hog/Core/Base.h"

#include "Hog/Core/Application.h"
#include "Hog/Core/Layer.h"
#include "Hog/Core/Log.h"
#include "Hog/Core/Assert.h"

#include "Hog/Core/Timestep.h"

#include "Hog/Core/Input.h"
#include "Hog/Core/KeyCodes.h"
#include "Hog/Core/MouseCodes.h"

#include <Hog/Utils/Loader.h>

#include "Hog/ImGui/ImGuiLayer.h"
#include "Hog/Core/CVars.h"

// ---Renderer------------------------

#include "Hog/Renderer/Types.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Resource/Pipeline.h"
#include "Hog/Renderer/Resource/Shader.h"
#include "Hog/Renderer/Resource/Buffer.h"
#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Material.h"
#include "Hog/Renderer/Renderer.h"
#include "Hog/Renderer/RenderGraph.h"
#include "Hog/Renderer/Resource/Image.h"
#include "Hog/Renderer/Resource/Texture.h"
#include "Hog/Renderer/EditorCamera.h"
#include "Hog/Renderer/Light.h"
#include "Hog/Renderer/Resource/AccelerationStructure.h"

// -----------------------------------