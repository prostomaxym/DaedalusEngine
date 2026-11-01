#include "dlpch.h"
#include "AssetSourceReader.h"
#include "VirtualFileSystem.h"
#include "Utf8.h"
#include <fstream>
#include <set>
#include <sstream>

namespace Daedalus
{
	bool AssetSourceReader::ReadBytes(const std::filesystem::path& path, std::string& data, std::string& error)
	{
		data.clear();
		error.clear();
		std::ifstream input(path, std::ios::binary);
		if (!input) { error = "Cannot open asset source"; return false; }
		std::ostringstream output;
		output << input.rdbuf();
		if (input.bad() || output.bad()) { error = "Cannot read asset source"; return false; }
		data = output.str();
		return true;
	}

	namespace
	{
		bool ReadShader(const VirtualFileSystem& files, const std::string& path, std::set<std::string>& stack, std::string& data, std::string& error)
		{
			const auto source = files.Resolve(path);
			const auto key = source ? Utf8ToString(source->generic_u8string()) : std::string();
			if (!source || stack.size() >= 32 || !stack.insert(key).second)
			{
				error = "Invalid shader include path, cycle or depth limit: " + path;
				return false;
			}
			std::string text;
			if (!AssetSourceReader::ReadBytes(*source, text, error)) return false;
			std::istringstream input(text);
			std::string line;
			while (std::getline(input, line))
			{
				if (!line.empty() && line.back() == '\r') line.pop_back();
				const auto first = line.find_first_not_of(" \t");
				if (first != std::string::npos && line.compare(first, 8, "#include") == 0)
				{
					const auto begin = line.find_first_not_of(" \t", first + 8);
					const auto end = begin == std::string::npos ? std::string::npos : line.find('"', begin + 1);
					const auto trailing = end == std::string::npos ? std::string::npos : line.find_first_not_of(" \t", end + 1);
					if (begin == std::string::npos || line[begin] != '"' || end == std::string::npos || end == begin + 1 ||
						(trailing != std::string::npos && line.compare(trailing, 2, "//") != 0))
					{
						error = "Invalid shader include directive: " + path;
						return false;
					}
					if (!ReadShader(files, "shaders:/" + line.substr(begin + 1, end - begin - 1), stack, data, error)) return false;
				}
				else data += line + '\n';
			}
			stack.erase(key);
			return true;
		}
	}

	bool AssetSourceReader::ReadShaderSource(const VirtualFileSystem& files, const std::string& virtual_path, std::string& data, std::string& error)
	{
		data.clear();
		error.clear();
		std::set<std::string> stack;
		if (ReadShader(files, virtual_path, stack, data, error)) return true;
		data.clear();
		return false;
	}
}
