#include "VersionWindow.h"
#include "../imgui/imgui.h"
#include "../socket/client_singleton.h"
#include "Gui.h"

VersionWindow::VersionWindow()
{
	name = "Server Version";
}

void VersionWindow::onDraw()
{
	if (ImGui::Button("Fetch Version"))
	{
		ServerVersionInfo info{};
		if (FetchServerVersion(info)) {
			hasData = true;
			version = info.version;
			versionString = info.versionString;
			Gui::log("Server version: %d (%s)", version, versionString.c_str());
		} else {
			hasData = false;
			Gui::log("Failed to fetch server version");
		}
	}

	if (hasData) {
		ImGui::Text("Version: %d", version);
		ImGui::Text("String: %s", versionString.c_str());
	}
} 