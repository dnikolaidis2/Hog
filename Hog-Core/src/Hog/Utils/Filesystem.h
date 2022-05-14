#pragma once

#include "Hog/Core/Log.h"

#include <string>
#include <filesystem>
#include <fstream>

namespace Hog
{
	inline static std::vector<uint32_t> ReadBinaryFile(const std::string& filepath)
	{
		std::vector<uint32_t> data;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in.is_open())
		{
			in.seekg(0, std::ios::end);
			auto size = in.tellg();
			in.seekg(0, std::ios::beg);

			data.resize(size / sizeof(uint32_t));
			in.read((char*)data.data(), size);

			return data;
		}

		return data;
	}

	inline static bool WriteBinaryFile(const std::string& filepath, const std::vector<uint32_t>& data)
	{
		std::ofstream out(filepath, std::ios::out | std::ios::binary);
		if (out.is_open())
		{
			out.write((char*)data.data(), data.size() * sizeof(uint32_t));
			out.flush();
			out.close();

			return true;
		}

		return false;
	}

	inline static std::string ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				HG_CORE_ERROR("Could not read from file '{0}'", filepath);
			}
		}
		else
		{
			HG_CORE_ERROR("Could not open file '{0}'", filepath);
		}

		return result;
	}

	inline static std::string ReadFile(const std::filesystem::path& path)
	{
		std::string result;
		std::ifstream in(path.c_str(), std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				HG_CORE_ERROR("Could not read from file '{0}'", path);
			}
		}
		else
		{
			HG_CORE_ERROR("Could not open file '{0}'", path);
		}

		return result;
	}
}