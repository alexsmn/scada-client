#pragma once

class MainWindow;
class NodeRef;
class TimedDataService;
class Profile;

void ExecuteWriteDialog(MainWindow* main_window, TimedDataService& timed_data_service, const NodeRef& node, bool manual, Profile& profile);
