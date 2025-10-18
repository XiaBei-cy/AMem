#pragma once

#include "Window.h"
#include <vector>
#include <string>

class ProcessListWindow : public Window {
public:
	ProcessListWindow();
	void onDraw() override;

private:
	bool hasData = false;
	std::vector<std::pair<int, std::string>> processes;
}; 