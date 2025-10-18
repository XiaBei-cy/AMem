#include "Gui.h"
#include "Window.h"
#include "CEWindow.h"
#include "ServerConnectWindow.h"
#include "ModulesWindow.h"
#include "../imgui/imgui.h"
#include <map>

namespace Gui {
	std::list<std::unique_ptr<Window>> windows;
	std::list<std::pair<std::string, int>> logs;

	void addWindow(Window* window)
	{
		static std::map<std::string, int> totalWindows;
		if (!window)
			return;

		int& count = totalWindows[window->name];
		if (count > 0)
			window->name = window->name + " " + std::to_string(count + 1);
		count++;

		windows.emplace_back(window);
	}

	static void drawLogsPanel()
	{
		if (logs.empty())
			return;
		if (ImGui::Begin("Logs")) {
			for (const auto& [msg, dup]: logs) {
				if (dup > 0)
					ImGui::TextUnformatted((msg + "  (x" + std::to_string(dup + 1) + ")").c_str());
				else
					ImGui::TextUnformatted(msg.c_str());
			}
		}
		ImGui::End();
	}

	void mainLoop()
	{
		static bool bootstrapped = false;
		if (!bootstrapped) {
			if (windows.empty()) {
				Gui::addWindow(new CEWindow());
				Gui::addWindow(new ServerConnectWindow());
				
			}
			Gui::log("欢迎使用 Cheat Turbine！");
			bootstrapped = true;
		}

		for (auto it = windows.begin(); it != windows.end(); )
		{
			Window* w = it->get();
			if (!w || !w->pOpen) {
				it = windows.erase(it);
				continue;
			}
			(*w)();
			++it;
		}

		drawLogsPanel();
	}
} 