#include "ComputeExample.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <imgui/imgui.h>

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui/imgui_internal.h"

static auto& context = GraphicsContext::Get();
constexpr uint32_t BufferElements = 32;

ComputeExample::ComputeExample()
	: Layer("ComputeExample")
{

}

void ComputeExample::OnAttach()
{
	HG_PROFILE_FUNCTION()

	GraphicsContext::Initialize();

	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.Queue), &(context.QueueFamilyIndex), 1, nullptr);

	m_ComputeBuffer = Buffer::Create(BufferDescription::Defaults::ReadbackStorageBuffer, BufferElements * sizeof(uint32_t));

	uint32_t n = 0;
	std::vector<uint32_t> tempBuffer(BufferElements);
	std::generate(tempBuffer.begin(), tempBuffer.end(), [&n] { return n++; });
	
	m_ComputeBuffer->SetData(tempBuffer.data(), tempBuffer.size() * sizeof(uint32_t));

	RenderGraph graph;
	uint32_t bufferElements = 32;

	auto fib = graph.AddStage(nullptr, { "Fibonacci stage", Shader::Create("Headless.compute"), RendererStageType::ForwardCompute,
		{
			{"values", ResourceType::Storage, ShaderType::Defaults::Compute, m_ComputeBuffer, 0, 0},
			{ "BUFFER_ELEMENTS", ResourceType::Constant, ShaderType::Defaults::Compute, 0, sizeof(uint32_t), &bufferElements}
		},
		{bufferElements, 1, 1}
	});

	Renderer::Initialize(graph);

	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 10000.0f);

	std::vector<uint32_t> computeBuffer(BufferElements);
	std::memcpy(computeBuffer.data(), (void*)(*m_ComputeBuffer), BufferElements * sizeof(uint32_t));

	HG_INFO("Before fibonacci stage");
	for (int i = 0; i < computeBuffer.size(); i++)
	{
		HG_TRACE("computeBuffer[{0}] = {1}", i, computeBuffer[i]);
	}
}

void ComputeExample::OnDetach()
{
	HG_PROFILE_FUNCTION()

	GraphicsContext::WaitIdle();

	std::vector<uint32_t> computeBuffer(BufferElements);
	HG_INFO("After fibonacci stage");
	std::memcpy(computeBuffer.data(), (void*)(*m_ComputeBuffer), BufferElements * sizeof(uint32_t));
	for (int i = 0; i < computeBuffer.size(); i++)
	{
		HG_TRACE("computeBuffer[{0}] = {1}", i, computeBuffer[i]);
	}

	Renderer::Deinitialize();
	MaterialLibrary::Deinitialize();
	TextureLibrary::Deinitialize();

	m_ComputeBuffer.reset();

	GraphicsContext::Deinitialize();
}

void ComputeExample::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION()

	m_EditorCamera.OnUpdate(ts);

	Application::Get().Close();
}

static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("X", buttonSize))
		values.x = resetValue;
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Y", buttonSize))
		values.y = resetValue;
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Z", buttonSize))
		values.z = resetValue;
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();
}

void ComputeExample::OnImGuiRender()
{
	HG_PROFILE_FUNCTION();

	ImGui::ShowDemoWindow();
	/*Timestep ts = Application::Get().GetTimestep();

	ImGui::Begin("Scene", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
	{
		ImGui::BeginChild("Object List Child", ImVec2(ImGui::GetContentRegionAvail().x, 300), false, ImGuiWindowFlags_None);
		{
			ImGui::Text("Object List");

			for (int i = 0; i < m_Objects.size(); ++i)
			{
				Ref<RendererObject> object = m_Objects[i];

				if (!m_SelectedObject && i == 0)
				{
					m_SelectedObject = object;
				}

				ImGuiTreeNodeFlags flags = ((m_SelectedObject == object) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
				ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)i, flags, object->GetName().c_str());
				if (ImGui::IsItemClicked())
				{
					m_SelectedObject = object;
				}
				ImGui::TreePop();
			}
		}
		ImGui::EndChild();

		ImGui::BeginChild("Properties Child", ImVec2(ImGui::GetContentRegionAvail().x, 480), false, ImGuiWindowFlags_None);
		{
			ImGui::Text("Properties");

			if (m_SelectedObject)
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					ImGui::Separator();
					bool open = ImGui::TreeNodeEx((void*)typeid(1254135).hash_code(), treeNodeFlags, "Translate");
					ImGui::PopStyleVar();

					if (open)
					{
						glm::vec3 scale;
						glm::quat rotationQuat;
						glm::vec3 translation;
						glm::vec3 skew;
						glm::vec4 perspective;
						glm::decompose(m_SelectedObject->GetTransform(), scale, rotationQuat, translation, skew, perspective);

						DrawVec3Control("Translation", translation);

						glm::vec3 rotation = glm::eulerAngles(rotationQuat);
						DrawVec3Control("Rotation", rotation);

						DrawVec3Control("Scale", scale, 1.0f);

						m_SelectedObject->SetTransform(glm::translate(glm::mat4{ 1.0f }, translation)
							* glm::toMat4(glm::quat(rotation))
							* glm::scale(glm::mat4{ 1.0f }, scale));

						ImGui::TreePop();
					}
				}

				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					ImGui::Separator();
					bool open = ImGui::TreeNodeEx((void*)typeid(23123123123).hash_code(), treeNodeFlags, "Material");
					ImGui::PopStyleVar();

					if (open)
					{
						auto& material = m_SelectedObject->GetMaterial()->GetMaterialData();

						ImGui::DragFloat("Index Of Refraction", &material.IOR, 0.01f, 0.001f, 10.0f);
						ImGui::DragFloat("Dissolve", &material.Dissolve, 0.01f, 0.001f, 10.0f);
						const char* items[] = { "Color on and Ambient off",
							"Color on and Ambient on",
							"Highlight on",
							"Reflection on and Ray trace on",
							"Transparency: Glass on, Reflection: Ray trace on",
							"Reflection: Fresnel on and Ray trace on",
							"Transparency: Refraction on, Reflection: Fresnel off and Ray trace on",
							"Transparency: Refraction on, Reflection: Fresnel on and Ray trace on",
							"Reflection on and Ray trace off",
							"Transparency: Glass on, Reflection: Ray trace off",
							"Casts shadows onto invisible surfaces" };
						ImGui::Combo("Illumination Model", &material.IlluminationModel, items, std::size(items), 3);
						ImGui::ColorEdit3("Transmittance Filter Color", glm::value_ptr(material.TransmittanceFilter));


						ImGui::DragFloat("Ambient Strength", &material.AmbientStrength);
						ImGui::ColorEdit3("Ambient Color", glm::value_ptr(material.AmbientColor));

						ImGui::ColorEdit3("Diffuse Color", glm::value_ptr(material.DiffuseColor));

						ImGui::DragFloat("Specularity", &material.Specularity, 1.0f, 0.0f, 1000.0f);
						ImGui::ColorEdit3("Specular Color", glm::value_ptr(material.SpecularColor));

						ImGui::DragFloat("Emissive Strength", &material.EmissiveStrength);
						ImGui::ColorEdit3("Emissive Color", glm::value_ptr(material.EmissiveColor));

						ImGui::TreePop();
					}
				}
			}
		}
		ImGui::EndChild();

		ImGui::End();
	}

	ImGui::Begin("Stats", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
	{
		if (ImGui::CollapsingHeader("Statistics"))
		{
			auto stats = Renderer::GetStats();
			ImGui::Text("Renderer Stats:");
			ImGui::Text("Frame Time (ms): %f", ts.GetMilliseconds());
		}

		if (ImGui::CollapsingHeader("Settings"))
		{
			CVarSystem::Get()->DrawImguiEditor();
		}

		ImGui::End();
	}*/
}

void ComputeExample::OnEvent(Event& e)
{
	m_EditorCamera.OnEvent(e);

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<FrameBufferResizeEvent>(HG_BIND_EVENT_FN(ComputeExample::OnResized));
}

bool ComputeExample::OnResized(FrameBufferResizeEvent& e)
{
	GraphicsContext::RecreateSwapChain();

	m_EditorCamera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());

	return false;
}