#include "AndroidBootstrap.h"
#include "AndroidRuntime.h"
#include "AndroidWindow.h"
#include "Core/Application.h"
#include "Debug/Log.h"
#include "Utils/BundledFileInstaller.h"
#include "Utils/WorkingDirectory.h"
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <filesystem>
#include <fstream>
#include <array>
#include <memory>
#include <sstream>

namespace Daedalus
{
	namespace
	{
		using Asset = std::unique_ptr<AAsset, decltype(&AAsset_close)>;

		bool PrepareBundledRuntime(AAssetManager* manager, const std::filesystem::path& root)
		{
			Asset manifest(AAssetManager_open(manager, "runtime-files.txt", AASSET_MODE_STREAMING), &AAsset_close);
			if (!manifest) return true; // Shader-only APKs do not have a runtime manifest.
			const auto length = AAsset_getLength64(manifest.get());
			if (length < 0 || length > 16 * 1024 * 1024) return false;
			std::string text(static_cast<size_t>(length), '\0');
			size_t offset = 0;
			while (offset < text.size())
			{
				const auto count = AAsset_read(manifest.get(), text.data() + offset, text.size() - offset);
				if (count <= 0) return false;
				offset += static_cast<size_t>(count);
			}
			std::istringstream lines(text);
			std::string relative_path;
			while (std::getline(lines, relative_path))
			{
				if (!relative_path.empty() && relative_path.back() == '\r') relative_path.pop_back();
				std::string error;
				const auto writer = [manager, &relative_path](std::ostream& output)
				{
					const auto name = "runtime/" + relative_path;
					Asset asset(AAssetManager_open(manager, name.c_str(), AASSET_MODE_STREAMING), &AAsset_close);
					if (!asset) return false;
					std::array<char, 16384> bytes{};
					int count = 0;
					while ((count = AAsset_read(asset.get(), bytes.data(), bytes.size())) > 0)
					{
						output.write(bytes.data(), count);
						if (!output) return false;
					}
					return count == 0;
				};
				if (!BundledFileInstaller::InstallMissing(root, relative_path, writer, error))
				{
					__android_log_print(ANDROID_LOG_ERROR, "Daedalus", "%s", error.c_str());
					return false;
				}
			}
			return true;
		}

		bool PrepareFiles(android_app& app)
		{
			const std::filesystem::path root(app.activity->internalDataPath);
			std::error_code error;
			for (const auto* folder : { "asset", "shader", "config", "logs" })
			{
				std::filesystem::create_directories(root / folder, error);
				if (error) return false;
			}
			std::filesystem::current_path(root, error);
			if (error) return false;
			auto* manager = app.activity->assetManager; // NativeActivity owns the asset manager.
			auto* directory = AAssetManager_openDir(manager, "");
			if (!directory) return false;
			bool success = true;
			while (const auto* name = AAssetDir_getNextFileName(directory))
			{
				const std::filesystem::path filename(name);
				if (filename.extension() != ".glsl" && filename.extension() != ".hglsl") continue;
				auto* asset = AAssetManager_open(manager, name, AASSET_MODE_STREAMING);
				if (!asset) { success = false; break; }
				const auto destination = root / "shader" / filename;
				const auto temporary = destination.string() + ".tmp";
				std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
				std::array<char, 16384> bytes{};
				int count = 0;
				while ((count = AAsset_read(asset, bytes.data(), bytes.size())) > 0)
					output.write(bytes.data(), count);
				AAsset_close(asset);
				output.close();
				if (count < 0 || !output) { success = false; break; }
				std::filesystem::rename(temporary, destination, error);
				if (error) { success = false; break; }
			}
			AAssetDir_close(directory);
			return success && PrepareBundledRuntime(manager, root);
		}
	}

	void RunAndroidApplication(android_app* app, Application* (*create_application)())
	{
		AndroidRuntime runtime(*app);
		if (!PrepareFiles(*app))
		{
			// The engine's path configuration does not exist until Application construction.
			__android_log_print(ANDROID_LOG_ERROR, "Daedalus", "Cannot prepare private runtime directories/shaders/assets");
			ANativeActivity_finish(app->activity);
			while (!runtime.IsDestroying()) runtime.PollEvents(true);
			return;
		}
		Log::Init();
		while (!runtime.IsDestroying())
		{
			while (!runtime.IsDestroying() && !runtime.IsActive()) runtime.PollEvents(true);
			if (runtime.IsDestroying()) break;
			bool restart = false;
			{
				const std::unique_ptr<Application> application(create_application());
				application->Run();
				restart = static_cast<AndroidWindow&>(application->GetWindow()).IsContextLost();
			}
			if (!restart && !runtime.IsDestroying())
			{
				ANativeActivity_finish(app->activity);
				while (!runtime.IsDestroying()) runtime.PollEvents(true);
			}
		}
		Log::Shutdown();
	}
}
