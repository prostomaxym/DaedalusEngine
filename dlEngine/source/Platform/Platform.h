#include <Daedalus/Core/IWindow.h>
#include <Daedalus/Core/Input.h>

#include <memory>

namespace Daedalus {

class Platform
{
public:
	static std::unique_ptr<IWindow> createWindow();
	static void InitInputSystem();
};

}
