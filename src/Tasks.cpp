#include "skse64/gamethreads.h" //TaskDelegate

#include "RE/Skyrim.h"

#include "Tasks.h"
#include "Offsets.h"
#include "Hooks.h"

#include <thread> //std::this_thread::sleep_for
#include <chrono> //std::chrono::seconds

#include <SKSE\API.h>

namespace Tasks
{

	//SleepWaitMenu
	SleepWaitDelegate::SleepWaitDelegate()
	{
	}

	void SleepWaitDelegate::Run()
	{
		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings* interfaceStrings = RE::InterfaceStrings::GetSingleton();

		const RE::FxDelegateArgs * args;

		RE::IMenu * sleepWaitMenu = ui->GetMenu(interfaceStrings->sleepWaitMenu).get();
		if (sleepWaitMenu) {

			RE::GFxValue time = sleepWaitTime;
			UInt32 numArgs = 1;

			args = new RE::FxDelegateArgs(RE::GFxValue(), sleepWaitMenu, sleepWaitMenu->view.get(), &time, numArgs);
		} else {
			return;
		}

		RequestSleepWait_Original = reinterpret_cast<void(*)(const RE::FxDelegateArgs&)>(Offsets::StartSleepWait_Original.GetUIntPtr());
		RequestSleepWait_Original(*args);
		delete(args);
	}

	void SleepWaitDelegate::Dispose()
	{
		delete this;
	}

	void SleepWaitDelegate::RegisterTask(const RE::FxDelegateArgs& a_args)
	{
		SleepWaitDelegate * task = new SleepWaitDelegate();
		task->sleepWaitTime = a_args[0].GetNumber();

		auto taskInterface = SKSE::GetTaskInterface();
		taskInterface->AddTask(task);
	}

	SaveGameDelegate::SaveGameDelegate()
	{
	}

	void SaveGameDelegate::Run()
	{
		typedef Hooks::BGSSaveLoadManagerEx::SaveMode SaveMode;
		typedef Hooks::BGSSaveLoadManagerEx::DumpFlag DumpFlag;

		//Save
		bool(*SaveGame_Original)(RE::BGSSaveLoadManager*, SaveMode, DumpFlag, const char*);
		SaveGame_Original = reinterpret_cast<bool(*)(RE::BGSSaveLoadManager*, SaveMode, DumpFlag, const char*)>(Offsets::SaveGame_Original.GetUIntPtr());
		SaveGame_Original(RE::BGSSaveLoadManager::GetSingleton(), SaveMode::kSave, this->dumpFlag, this->saveName);
	}

	void SaveGameDelegate::Dispose()
	{
		delete this;
	}

	void SaveGameDelegate::RegisterTask(Hooks::BGSSaveLoadManagerEx::DumpFlag a_dumpFlag, const char* a_name)
	{
		//Create save screenshot
		void(*RequestScreenshot_Original)();
		RequestScreenshot_Original = reinterpret_cast<void(*)()>(Offsets::RequestScreenshot_Original.GetUIntPtr());
		RequestScreenshot_Original();

		void(*CreateScreenshot_Original)();
		CreateScreenshot_Original = reinterpret_cast<void(*)()>(Offsets::CreateScreenshot_Original.GetUIntPtr());
		CreateScreenshot_Original();

		SaveGameDelegate* task = new SaveGameDelegate();

		if (a_name) {
			const size_t len = strlen(a_name);
			char* tmp = new char[len + 1];
			strncpy_s(tmp, len + 1, a_name, len);
			task->saveName = tmp;
		}
		else {
			task->saveName = nullptr;
		}

		task->dumpFlag = a_dumpFlag;
		auto taskInterface = SKSE::GetTaskInterface();
		taskInterface->AddTask(task);
	}

	ServeTimeDelegate::ServeTimeDelegate()
	{
	}

	void ServeTimeDelegate::Run()
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		player->ServePrisonTime();
	}

	void ServeTimeDelegate::Dispose()
	{
		delete this;
	}

	void ServeTimeDelegate::RegisterTask()
	{
		ServeTimeDelegate* task = new ServeTimeDelegate();
		auto taskInterface = SKSE::GetTaskInterface();
		taskInterface->AddTask(task);
	}


	UpdateInventoryDelegate::UpdateInventoryDelegate()
	{

	}

	void UpdateInventoryDelegate::Run()
	{
		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings * interfaceStrings = RE::InterfaceStrings::GetSingleton();
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

		player->currentProcess->UpdateEquipment(player);
		(this->containerOwner)->currentProcess->UpdateEquipment(this->containerOwner);
		(this->list)->Update(player);
	}

	void UpdateInventoryDelegate::Dispose()
	{
		delete this;
	}

	void UpdateInventoryDelegate::RegisterTask(RE::ItemList* a_list, RE::Actor* a_containerOwner)
	{
		UpdateInventoryDelegate * task = new UpdateInventoryDelegate();
		task->list = a_list;
		task->containerOwner = a_containerOwner;
		auto taskInterface = SKSE::GetTaskInterface();
		taskInterface->AddTask(task);;
	}

	MessageBoxButtonPressDelegate::MessageBoxButtonPressDelegate()
	{

	}

	void MessageBoxButtonPressDelegate::Run()
	{
		RE::UI* ui = RE::UI::GetSingleton();
		RE::InterfaceStrings* interfaceStrings = RE::InterfaceStrings::GetSingleton();

		const RE::FxDelegateArgs* args;

		RE::IMenu* messageBoxMenu = ui->GetMenu(interfaceStrings->messageBoxMenu).get();
		if (messageBoxMenu) {

			RE::GFxValue index = this->selectedIndex;
			UInt32 numArgs = 1;

			args = new RE::FxDelegateArgs(RE::GFxValue(), messageBoxMenu, messageBoxMenu->view.get(), &index, numArgs);
		}
		else {
			return;
		}

		void(*MessageBoxButtonPress_Original)(const RE::FxDelegateArgs&);
		MessageBoxButtonPress_Original = reinterpret_cast<void(*)(const RE::FxDelegateArgs&)>(Offsets::MessageBoxButtonPress_Original.GetUIntPtr());
		MessageBoxButtonPress_Original(*args);
		delete(args);
	}
	void MessageBoxButtonPressDelegate::Dispose()
	{
		delete this;
	}
	void MessageBoxButtonPressDelegate::RegisterTask(const RE::FxDelegateArgs& a_args)
	{
		if (a_args.GetArgCount() == 1 && a_args[0].IsNumber())
		{
			MessageBoxButtonPressDelegate* task = new MessageBoxButtonPressDelegate();
			task->selectedIndex = a_args[0].GetNumber();
			auto taskInterface = SKSE::GetTaskInterface();
			taskInterface->AddTask(task);
		}
	}
}
