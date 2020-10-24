#include "Menus/Hooks_ContainerMenu.h"

namespace SkyrimSoulsRE
{
	RE::UI_MESSAGE_RESULTS ContainerMenuEx::ProcessMessage_Hook(RE::UIMessage& a_message)
	{
		if (a_message.type == RE::UI_MESSAGE_TYPE::kHide)
		{
			HUDMenuEx* hudMenu = static_cast<HUDMenuEx*>(RE::UI::GetSingleton()->GetMenu(RE::HUDMenu::MENU_NAME).get());
			if (hudMenu)
			{
				hudMenu->SetSkyrimSoulsMode(false);
			}
		}
		return _ProcessMessage(this, a_message);
	}

	void ContainerMenuEx::AdvanceMovie_Hook(float a_interval, std::uint32_t a_currentTime)
	{
		this->UpdateBottomBar();
		if (containerMode == ContainerMode::kPickpocket)
		{
			this->UpdatePickpocketChance();
		}

		AutoCloseManager* autoCloseManager = AutoCloseManager::GetSingleton();
		autoCloseManager->CheckAutoClose(RE::ContainerMenu::MENU_NAME);

		return _AdvanceMovie(this, a_interval, a_currentTime);
	}

	std::int32_t ContainerMenuEx::CalcPickPocketChance(RE::StandardItemData* a_itemData)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

		RE::Actor* targetActor = static_cast<RE::Actor*>(ContainerMenuEx::containerRef);

		float itemWeight = a_itemData->objDesc->GetWeight();
		std::uint32_t count = 1;

		if (a_itemData->objDesc->object->IsAmmo() || a_itemData->objDesc->object->IsLockpick() || a_itemData->objDesc->object->IsGold())
		{
			count = a_itemData->GetCount();
			itemWeight = -1.0f;
		}

		std::uint32_t stealValue = targetActor->GetStealValue(a_itemData->objDesc, count, true);
		
		bool isDetected = targetActor->RequestDetectionLevel(player, RE::DETECTION_PRIORITY::kNormal) > 0;
		float playerSkill = player->GetActorValue(RE::ActorValue::kPickpocket);
		float targetSkill = targetActor->GetActorValue(RE::ActorValue::kPickpocket);

		auto chance = RE::AIFormulas::ComputePickpocketSuccess(playerSkill, targetSkill, stealValue, itemWeight, player, targetActor, isDetected, a_itemData->objDesc->object);

		if (chance > 100) {
			chance = 100;
		}
		else if (chance < 0) {
			chance = 0;
		}

		return chance;
	}

	void ContainerMenuEx::UpdatePickpocketChance()
	{
		if (containerRef && containerRef->formType == RE::FormType::ActorCharacter)
		{
			RE::ItemList::Item* item = this->itemList->GetSelectedItem();
			if (item)
			{
				std::int32_t chance = CalcPickPocketChance(&item->data);

				RE::GFxValue isViewingContainer;
				this->uiMovie->Invoke("_root.Menu_mc.isViewingContainer", &isViewingContainer, nullptr, 0);

				if (isViewingContainer.IsUndefined())
				{
					return;
				}

				RE::BSFixedStringW toSteal = L"$ TO STEAL";
				RE::BSFixedStringW toPlace = L"$ TO PLACE";

				auto translator = this->uiMovie->GetState<RE::GFxTranslator>(RE::GFxState::StateType::kTranslator);
				RE::GPtr bsTranslator{ skyrim_cast<RE::BSScaleformTranslator*>(translator.get()) };
				if (bsTranslator) {
					auto& map = bsTranslator->translator.translationMap;
					auto it = map.find(toSteal);
					if (it != map.end()) {
						toSteal = it->second;
					}
					it = map.find(toPlace);
					if (it != map.end()) {
						toPlace = it->second;
					}
				}

				std::wstring desc;

				desc = isViewingContainer.GetBool() ? desc = toSteal.data() : desc = toPlace.data();

				std::wstring stealText(L"<font face=\'$EverywhereBoldFont\' size=\'24\' color=\'#FFFFFF\'>" + std::to_wstring(chance) + L"%</font>" + desc);

				RE::GFxValue stealTextValue;
				this->uiMovie->GetVariable(&stealTextValue, "_root.Menu_mc.itemCardFadeHolder.StealTextInstance.PercentTextInstance");
				if (!stealTextValue.IsUndefined())
				{
					RE::GFxValue newText(stealText.c_str());
					stealTextValue.SetMember("htmlText", newText);
				}
			}
		}
	}

	void ContainerMenuEx::UpdateBottomBar()
	{
		using func_t = decltype(&ContainerMenuEx::UpdateBottomBar);
		REL::Relocation<func_t> func(Offsets::Menus::ContainerMenu::UpdateBottomBar);
		return func(this);
	}

	void ContainerMenuEx::EquipItem_Hook(const RE::FxDelegateArgs& a_args)
	{
		class EquipItemTask : public UnpausedTask
		{
			double		equipHand;
			bool		hasCount;
			double		count;

		public:
			EquipItemTask(double a_equipHand, bool a_hasCount, double a_count = 0)
			{
				this->equipHand = a_equipHand;
				this->hasCount = a_hasCount;
				this->count = a_count;
				this->usesDelay = false;
			}

			void Run() override
			{
				RE::UI* ui = RE::UI::GetSingleton();

				if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME))
				{
					RE::IMenu* menu = ui->GetMenu(RE::ContainerMenu::MENU_NAME).get();

					if (this->hasCount)
					{
						RE::GFxValue arg[2];
						arg[0] = this->equipHand;
						arg[1] = this->count;
						const RE::FxDelegateArgs args(0, menu, menu->uiMovie.get(), arg, 2);
						ContainerMenuEx::_EquipItem(args);
					}
					else
					{
						RE::GFxValue arg = this->equipHand;
						const RE::FxDelegateArgs args(0, menu, menu->uiMovie.get(), &arg, 2);
						ContainerMenuEx::_EquipItem(args);
					}
				}
			}
		};

		EquipItemTask* task;
		if (a_args.GetArgCount() == 1)
		{
			task = new EquipItemTask(a_args[0].GetNumber(), false);
		}
		else
		{
			task = new EquipItemTask(a_args[0].GetNumber(), true, a_args[1].GetNumber());
		}
		UnpausedTaskQueue* queue = UnpausedTaskQueue::GetSingleton();
		queue->AddTask(task);
	}

	void ContainerMenuEx::TakeAllItems_Hook(const RE::FxDelegateArgs& a_args)
	{
		class TakeAllItemsTask : public UnpausedTask
		{
		public:
			TakeAllItemsTask()
			{
				this->usesDelay = false;
			}

			void Run() override
			{
				RE::UI* ui = RE::UI::GetSingleton();

				if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME))
				{
					ContainerMenuEx* menu = static_cast<ContainerMenuEx*>(ui->GetMenu(RE::ContainerMenu::MENU_NAME).get());

					const RE::FxDelegateArgs args(0, menu, menu->uiMovie.get(), nullptr, 0);
					ContainerMenuEx::_TakeAllItems(args);
				}
			}
		};
		TakeAllItemsTask* task = new TakeAllItemsTask();
		UnpausedTaskQueue* queue = UnpausedTaskQueue::GetSingleton();
		queue->AddTask(task);
	}

	void ContainerMenuEx::TransferItem_Hook(const RE::FxDelegateArgs& a_args)
	{
		class TransferItemTask : public UnpausedTask
		{
			double	count;
			bool	isViewingContainer;
		public:
			TransferItemTask(double a_count, bool a_isViewingContainer)
			{
				this->count = a_count;
				this->isViewingContainer = a_isViewingContainer;
				this->usesDelay = false;
			}

			void Run() override
			{
				RE::UI* ui = RE::UI::GetSingleton();

				if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME))
				{
					ContainerMenuEx* menu = static_cast<ContainerMenuEx*>(ui->GetMenu(RE::ContainerMenu::MENU_NAME).get());

					RE::ItemList::Item* selectedItem = menu->itemList->GetSelectedItem();

					if (selectedItem)
					{
						RE::GFxValue arg[2];
						arg[0] = this->count;
						arg[1] = this->isViewingContainer;
						const RE::FxDelegateArgs args(0, menu, menu->uiMovie.get(), arg, 2);
						ContainerMenuEx::_TransferItem(args);

						if (containerMode == ContainerMode::kSteal && menu->value != 0)
						{
							RE::PlayerCharacter::GetSingleton()->StealAlarm(containerRef, selectedItem->data.objDesc->object, static_cast<std::int32_t>(count), selectedItem->data.objDesc->GetValue(), containerRef->GetOwner(), true);
							menu->value = 0;
						}
					}
				}
			}
		};
		TransferItemTask* task = new TransferItemTask(a_args[0].GetNumber(), a_args[1].GetBool());
		UnpausedTaskQueue* queue = UnpausedTaskQueue::GetSingleton();
		queue->AddTask(task);
	}

	RE::IMenu* ContainerMenuEx::Creator()
	{
		RE::ContainerMenu* menu = static_cast<RE::ContainerMenu*>(CreateMenu(RE::ContainerMenu::MENU_NAME));

		RE::FxDelegate* dlg = menu->fxDelegate.get();

		_TransferItem = dlg->callbacks.GetAlt("ItemTransfer")->callback;
		dlg->callbacks.GetAlt("ItemTransfer")->callback = TransferItem_Hook;

		_EquipItem = dlg->callbacks.GetAlt("EquipItem")->callback;
		dlg->callbacks.GetAlt("EquipItem")->callback = EquipItem_Hook;

		_TakeAllItems = dlg->callbacks.GetAlt("TakeAllItems")->callback;
		dlg->callbacks.GetAlt("TakeAllItems")->callback = TakeAllItems_Hook;

		HUDMenuEx* hudMenu = static_cast<HUDMenuEx*>(RE::UI::GetSingleton()->GetMenu(RE::HUDMenu::MENU_NAME).get());
		if (hudMenu)
		{
			hudMenu->SetSkyrimSoulsMode(true);
		}

		std::uint32_t* handle = reinterpret_cast<std::uint32_t*>(Offsets::Menus::ContainerMenu::TargetRefHandle.address());
		RE::TESObjectREFRPtr refptr = nullptr;
		RE::TESObjectREFR* ref = nullptr;
		if (RE::TESObjectREFR::LookupByHandle(*handle, refptr))
		{
			ref = refptr.get();
		}
		else
		{
			SKSE::log::error("Failed to find Container Menu target!");
		}

		containerRef = ref;
		containerMode = *(reinterpret_cast<ContainerMode*>(Offsets::Menus::ContainerMenu::ContainerMode.address()));

		AutoCloseManager* autoCloseManager = AutoCloseManager::GetSingleton();		
		autoCloseManager->InitAutoClose(RE::ContainerMenu::MENU_NAME, ref, containerMode == ContainerMenuEx::ContainerMode::kPickpocket);

		return menu;
	}

	void ContainerMenuEx::InstallHook()
	{
		REL::Relocation<std::uintptr_t> vTable(Offsets::Menus::ContainerMenu::Vtbl);
		_ProcessMessage = vTable.write_vfunc(0x4, &ContainerMenuEx::ProcessMessage_Hook);
		_AdvanceMovie = vTable.write_vfunc(0x5, &ContainerMenuEx::AdvanceMovie_Hook);
	}
}