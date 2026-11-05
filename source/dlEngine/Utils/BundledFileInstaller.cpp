#include "dlpch.h"
#include "BundledFileInstaller.h"
#include "Utf8.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>

namespace Daedalus
{
	namespace
	{
		struct TemporaryDirectory
		{
			std::filesystem::path path;
			~TemporaryDirectory()
			{
				std::error_code ignored;
				if (!path.empty()) std::filesystem::remove_all(path, ignored);
			}
		};
	}

	bool BundledFileInstaller::InstallMissing(const std::filesystem::path& root,
		const std::string& relative_path, const Writer& writer, std::string& error)
	{
		error.clear();
		const auto fail = [&error, &relative_path](const std::string& reason)
		{
			error = "Cannot provision " + relative_path + ": " + reason;
			return false;
		};
		if (relative_path.empty() || relative_path.find_first_of("\\:\r\n") != std::string::npos ||
			relative_path.find('\0') != std::string::npos || relative_path.find("//") != std::string::npos)
			return fail("invalid path");
		const auto relative = Utf8ToPath(relative_path);
		if (relative.is_absolute() || relative.has_root_path()) return fail("absolute path");
		for (const auto& part : relative)
			if (part.empty() || part == "." || part == "..") return fail("unsafe path component");
		if (Utf8ToString(relative.generic_u8string()) != relative_path)
			return fail("noncanonical path");
		const auto first = *relative.begin();
		if (relative != "path.ini" && !((first == "asset" || first == "script" || first == "config") && relative.has_parent_path()))
			return fail("unsupported runtime location");

		std::error_code ec;
		const auto base = std::filesystem::canonical(root, ec);
		if (ec) return fail(ec.message());
		auto destination = base;
		for (auto part = relative.begin(); part != relative.end(); ++part)
		{
			destination /= *part;
			const bool last = std::next(part) == relative.end();
			const auto status = std::filesystem::symlink_status(destination, ec);
			if (ec == std::errc::no_such_file_or_directory) ec.clear();
			if (ec) return fail(ec.message());
			if (std::filesystem::is_symlink(status)) return fail("symlink in destination");
			if (std::filesystem::exists(status))
			{
				if (last) return std::filesystem::is_regular_file(status) || fail("destination is not a regular file");
				if (!std::filesystem::is_directory(status)) return fail("parent is not a directory");
			}
			else if (!last)
			{
				std::filesystem::create_directory(destination, ec);
				if (ec) return fail(ec.message());
			}
		}

		static std::atomic<uint64_t> s_sequence{ 0 };
		TemporaryDirectory temporary;
		for (int attempt = 0; attempt < 32; ++attempt)
		{
			const auto candidate = base / (".apk-install-" +
				std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(s_sequence++));
			if (std::filesystem::create_directory(candidate, ec)) { temporary.path = candidate; break; }
			if (ec) return fail(ec.message());
		}
		if (temporary.path.empty()) return fail("cannot create staging directory");
		const auto staged_file = temporary.path / "file";
		std::ofstream output(staged_file, std::ios::binary | std::ios::trunc);
		if (!output || !writer(output)) return fail("bundled read/write failed");
		output.close();
		if (!output) return fail("bundled write failed");

		// Android app storage forbids hard links; re-check before an atomic rename of the complete file.
		const auto status = std::filesystem::symlink_status(destination, ec);
		if (ec == std::errc::no_such_file_or_directory) ec.clear();
		if (ec) return fail(ec.message());
		if (std::filesystem::exists(status))
			return std::filesystem::is_regular_file(status) || fail("destination is not a regular file");
		std::filesystem::rename(staged_file, destination, ec);
		if (ec) return fail(ec.message());
		return true;
	}
}
