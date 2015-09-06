#include <iostream>
#include <sstream>
#include "Hotkeys.h"
#include "logger.h"
#include "GWToolbox.h"
#include "Config.h"

using namespace GWAPI;
using namespace OSHGui;

TBHotkey::TBHotkey(Key key, Key modifier, bool active, wstring ini_section)
	: active_(active), key_(key), modifier_(modifier), ini_section_(ini_section) {

	pressed_ = false;

	TBHotkey* self = this;
	SetBackColor(Drawing::Color::Empty());
	SetSize(WIDTH, HEIGHT);

	CheckBox* checkbox = new CheckBox();
	checkbox->SetChecked(active);
	checkbox->SetText("");
	checkbox->SetLocation(0, 5);
	checkbox->SetBackColor(Drawing::Color::Black());
	checkbox->GetCheckedChangedEvent() += CheckedChangedEventHandler([self, checkbox, ini_section](Control*) {
		self->set_active(checkbox->GetChecked());
		GWToolbox::instance()->config()->iniWriteBool(ini_section.c_str(), TBHotkey::IniKeyActive(), checkbox->GetChecked());
	});
	AddControl(checkbox);

	HotkeyControl* hotkey_button = new HotkeyControl();
	hotkey_button->SetSize(WIDTH - checkbox->GetRight() - 60 - HSPACE * 2, LINE_HEIGHT);
	hotkey_button->SetLocation(checkbox->GetRight() + HSPACE, 0);
	hotkey_button->SetHotkey(key);
	hotkey_button->SetHotkeyModifier(modifier);
	hotkey_button->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	hotkey_button->GetFocusLostEvent() += FocusLostEventHandler([](Control*, Control*) {
		GWToolbox::capture_input = false;
	});
	hotkey_button->GetHotkeyChangedEvent() += HotkeyChangedEventHandler(
		[self, hotkey_button, ini_section](Control*) {
		Key key = hotkey_button->GetHotkey();
		Key modifier = hotkey_button->GetHotkeyModifier();
		self->set_key(key);
		self->set_modifier(modifier);
		GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(),
			self->IniKeyHotkey(), (long)key);
		GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(),
			self->IniKeyModifier(), (long)modifier);
	});
	AddControl(hotkey_button);

	Button* run_button = new Button();
	run_button->SetText("Run");
	run_button->SetSize(60, LINE_HEIGHT);
	run_button->SetLocation(WIDTH - 60, 0);
	run_button->GetClickEvent() += ClickEventHandler([self](Control*) {
		self->exec();
	});
	AddControl(run_button);
}


HotkeySendChat::HotkeySendChat(Key key, Key modifier, bool active, wstring ini_section,
	wstring msg, wchar_t channel)
	: TBHotkey(key, modifier, active, ini_section), msg_(msg), channel_(channel) {
	
	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Send Chat");
	AddControl(label);

	ComboBox* combo = new ComboBox();
	combo->AddItem("/");
	combo->AddItem("!");
	combo->AddItem("@");
	combo->AddItem("#");
	combo->AddItem("$");
	combo->AddItem("%");
	combo->SetSelectedIndex(ChannelToIndex(channel));
	combo->SetSize(30, LINE_HEIGHT);
	combo->SetLocation(label->GetRight() + HSPACE, ITEM_Y);
	HotkeySendChat* self = this;
	combo->GetSelectedIndexChangedEvent() += SelectedIndexChangedEventHandler(
		[self, combo, ini_section](Control*) {
		wchar_t channel = self->IndexToChannel(combo->GetSelectedIndex());
		self->set_channel(channel);
		GWToolbox::instance()->config()->iniWrite(ini_section.c_str(), 
			self->IniKeyChannel(), wstring(1, channel).c_str());
	});
	AddControl(combo);
	
	string text = string(msg.begin(), msg.end());
	TextBox* text_box = new TextBox();
	text_box->SetText(text);
	text_box->SetSize(WIDTH - combo->GetRight() - HSPACE, LINE_HEIGHT);
	text_box->SetLocation(combo->GetRight() + HSPACE, ITEM_Y);
	text_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, text_box, ini_section](Control*) {
		string text = text_box->GetText();
		wstring wtext = wstring(text.begin(), text.end());
		self->set_msg(wtext);
		GWToolbox::instance()->config()->iniWrite(ini_section.c_str(),
			self->IniKeyMsg(), wtext.c_str());
	});
	text_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	text_box->GetFocusLostEvent() += FocusLostEventHandler([](Control*, Control*) {
		GWToolbox::capture_input = false;
	});
	AddControl(text_box);
}

int HotkeySendChat::ChannelToIndex(wchar_t channel) {
	switch (channel) {
	case L'/': return 0;
	case L'!': return 1;
	case L'@': return 2;
	case L'#': return 3;
	case L'$': return 4;
	case L'%': return 5;
	default:
		LOG("Warning - bad channel %lc", channel);
		return 0;
	}
}

wchar_t HotkeySendChat::IndexToChannel(int index) {
	switch (index) {
	case 0: return L'/';
	case 1: return L'!';
	case 2: return L'@';
	case 3: return L'#';
	case 4: return L'$';
	case 5: return L'%';
	default:
		LOG("Warning - bad index %d", index);
		return L'/';
	}
}

HotkeyUseItem::HotkeyUseItem(Key key, Key modifier, bool active, wstring ini_section,
	UINT item_id_, wstring item_name_) :
	TBHotkey(key, modifier, active, ini_section), 
	item_id_(item_id_), 
	item_name_(item_name_) {

	HotkeyUseItem* self = this;

	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Use Item");
	AddControl(label);

	Label* label_id = new Label();
	label_id->SetLocation(label->GetRight() + HSPACE, LABEL_Y);
	label_id->SetText("ID:");
	AddControl(label_id);

	int width_left = WIDTH - label_id->GetRight();
	TextBox* id_box = new TextBox();
	id_box->SetText(to_string(item_id_));
	id_box->SetSize(width_left / 2 - HSPACE / 2, LINE_HEIGHT);
	id_box->SetLocation(label_id->GetRight(), ITEM_Y);
	id_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, id_box, ini_section](Control*) {
		try {
			long id = std::stol(id_box->GetText());
			self->set_item_id((UINT)id);
			GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(),
				self->IniItemIDKey(), id);
		} catch (...) {}
	});
	id_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	id_box->GetFocusLostEvent() += FocusLostEventHandler([id_box](Control*, Control*) {
		GWToolbox::capture_input = false;
		try {
			std::stol(id_box->GetText());
		} catch (...) {
			id_box->SetText("0");
		}
	});
	AddControl(id_box);

	TextBox* name_box = new TextBox();
	string text = string(item_name_.begin(), item_name_.end());
	name_box->SetText(text.c_str());
	name_box->SetSize(width_left / 2 - HSPACE / 2, LINE_HEIGHT);
	name_box->SetLocation(id_box->GetRight() + HSPACE , ITEM_Y);
	name_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, name_box, ini_section](Control*) {
		string text = name_box->GetText();
		wstring wtext = wstring(text.begin(), text.end());
		self->set_item_name(wtext);
		GWToolbox::instance()->config()->iniWrite(ini_section.c_str(),
			self->IniItemNameKey(), wtext.c_str());
	});
	name_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	name_box->GetFocusLostEvent() += FocusLostEventHandler([](Control*, Control*) {
		GWToolbox::capture_input = false;
	});
	AddControl(name_box);
}

HotkeyDropUseBuff::HotkeyDropUseBuff(Key key, Key modifier, bool active, wstring ini_section, UINT skillID) :
TBHotkey(key, modifier, active, ini_section), skillID_(skillID) {
	HotkeyDropUseBuff* self = this;

	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Drop / Use Buff");
	AddControl(label);

	ComboBox* combo = new ComboBox();
	combo->AddItem("Recall");
	combo->AddItem("UA");
	combo->SetSize(WIDTH - label->GetRight() - HSPACE, LINE_HEIGHT);
	combo->SetLocation(label->GetRight() + HSPACE, ITEM_Y);
	switch (skillID) {
	case GwConstants::SkillID::Recall:
		combo->SetSelectedIndex(0);
		break;
	case GwConstants::SkillID::UA:
		combo->SetSelectedIndex(1);
		break;
	default:
		combo->AddItem(to_string(skillID));
		combo->SetSelectedIndex(2);
		break;
	}
	combo->GetSelectedIndexChangedEvent() += SelectedIndexChangedEventHandler(
		[self, combo, ini_section](Control*) {
		UINT skillID = self->IndexToSkillID(combo->GetSelectedIndex());
		self->set_skillID(skillID);
		GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(),
			self->IniSkillIDKey(), (long)skillID);
	});
	combo_ = combo;
	AddControl(combo);
}

UINT HotkeyDropUseBuff::IndexToSkillID(int index) {
	switch (index) {
	case 0: return GwConstants::SkillID::Recall;
	case 1: return GwConstants::SkillID::UA;
	case 2: 
		if (combo_->GetItemsCount() == 3) {
			string s = combo_->GetItem(2);
			try {
				int i = std::stoi(s);
				return (UINT)i;
			} catch (...) {
				return 0;
			}
		}
	default:
		LOG("Warning. bad skill id %d\n", index);
		return GwConstants::SkillID::Recall;
	}
}

HotkeyToggle::HotkeyToggle(Key key, Key modifier, bool active, wstring ini_section, int toggle_id)
	: TBHotkey(key, modifier, active, ini_section) {

	HotkeyToggle* self = this;
	target_ = (Toggle)toggle_id;

	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Toggle function");
	AddControl(label);

	ComboBox* combo = new ComboBox();
	combo->SetSize(WIDTH - label->GetRight() - HSPACE, LINE_HEIGHT);
	combo->SetLocation(label->GetRight() + HSPACE, ITEM_Y);
	combo->AddItem("Clicker");
	combo->AddItem("Pcons");
	combo->AddItem("Coin drop");
	combo->AddItem("Rupt bot");
	combo->SetSelectedIndex(toggle_id - 1);
	combo->GetSelectedIndexChangedEvent() += SelectedIndexChangedEventHandler(
		[self, combo, ini_section](Control*) {
		int index = combo->GetSelectedIndex() + 1;
		Toggle target = (Toggle)index;
		self->set_target(target);
		GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(), 
			self->IniToggleIDKey(), (long)target);
	});
	AddControl(combo);
}

HotkeyTarget::HotkeyTarget(Key key, Key modifier, bool active, wstring ini_section, 
	UINT targetID, wstring target_name)
	: TBHotkey(key, modifier, active, ini_section), targetID_(targetID), target_name_(target_name) {

	HotkeyTarget* self = this;

	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Target");
	AddControl(label);

	Label* label_id = new Label();
	label_id->SetLocation(label->GetRight() + HSPACE, LABEL_Y);
	label_id->SetText("ID:");
	AddControl(label_id);

	int width_left = WIDTH - label_id->GetRight();
	TextBox* id_box = new TextBox();
	id_box->SetText(to_string(targetID_));
	id_box->SetSize(width_left / 2 - HSPACE / 2, LINE_HEIGHT);
	id_box->SetLocation(label_id->GetRight(), ITEM_Y);
	id_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, id_box, ini_section](Control*) {
		try {
			long id = std::stol(id_box->GetText());
			self->set_targetID((UINT)id);
			GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(),
				self->IniTargetIDKey(), id);
		} catch (...) {}
	});
	id_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	id_box->GetFocusLostEvent() += FocusLostEventHandler([id_box](Control*, Control*) {
		GWToolbox::capture_input = false;
		try {
			std::stol(id_box->GetText());
		} catch (...) {
			id_box->SetText("0");
		}
	});
	AddControl(id_box);

	TextBox* name_box = new TextBox();
	string text = string(target_name_.begin(), target_name_.end());
	name_box->SetText(text.c_str());
	name_box->SetSize(width_left / 2 - HSPACE / 2, LINE_HEIGHT);
	name_box->SetLocation(id_box->GetRight() + HSPACE, ITEM_Y);
	name_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, name_box, ini_section](Control*) {
		string text = name_box->GetText();
		wstring wtext = wstring(text.begin(), text.end());
		self->set_target_name(wtext);
		GWToolbox::instance()->config()->iniWrite(ini_section.c_str(),
			self->IniTargetNameKey(), wtext.c_str());
	});
	name_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	name_box->GetFocusLostEvent() += FocusLostEventHandler([](Control*, Control*) {
		GWToolbox::capture_input = false;
	});
	AddControl(name_box);
}

HotkeyMove::HotkeyMove(Key key, Key modifier, bool active, wstring ini_section,
	float x, float y, wstring name)
	: TBHotkey(key, modifier, active, ini_section), x_(x), y_(y), name_(name) {

	HotkeyMove* self = this;

	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Move");
	AddControl(label);

	Label* label_x = new Label();
	label_x->SetLocation(label->GetRight() + HSPACE, LABEL_Y);
	label_x->SetText("X");
	AddControl(label_x);

	std::stringstream ss;
	TextBox* box_x = new TextBox();
	ss.clear();
	ss << x;
	box_x->SetText(ss.str());
	box_x->SetSize(50, LINE_HEIGHT);
	box_x->SetLocation(label_x->GetRight(), ITEM_Y);
	box_x->GetTextChangedEvent() += TextChangedEventHandler(
		[self, box_x, ini_section](Control*) {
		try {
			float x = std::stof(box_x->GetText());
			self->set_x(x);
			GWToolbox::instance()->config()->iniWriteDouble(ini_section.c_str(), self->IniXKey(), x);
		} catch (...) {}
	});
	box_x->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	box_x->GetFocusLostEvent() += FocusLostEventHandler([box_x](Control*, Control*) {
		GWToolbox::capture_input = false;
		try {
			std::stof(box_x->GetText());
		} catch (...) {
			box_x->SetText("0.0");
		}
	});
	AddControl(box_x);

	Label* label_y = new Label();
	label_y->SetLocation(box_x->GetRight() + HSPACE, LABEL_Y);
	label_y->SetText("Y");
	AddControl(label_y);

	TextBox* box_y = new TextBox();
	ss.clear();
	ss << y;
	box_y->SetText(ss.str());
	box_y->SetSize(50, LINE_HEIGHT);
	box_y->SetLocation(label_y->GetRight(), ITEM_Y);
	box_y->GetTextChangedEvent() += TextChangedEventHandler(
		[self, box_y, ini_section](Control*) {
		try {
			float y = std::stof(box_y->GetText());
			self->set_y(y);
			GWToolbox::instance()->config()->iniWriteDouble(ini_section.c_str(), self->IniYKey(), y);
		} catch (...) {}
	});
	box_y->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	box_y->GetFocusLostEvent() += FocusLostEventHandler([box_y](Control*, Control*) {
		GWToolbox::capture_input = false;
		try {
			std::stof(box_y->GetText());
		} catch (...) {
			box_y->SetText("0.0");
		}
	});
	AddControl(box_y);

	TextBox* name_box = new TextBox();
	string text = string(name_.begin(), name_.end());
	name_box->SetText(text.c_str());
	name_box->SetSize(WIDTH - box_y->GetRight() - HSPACE, LINE_HEIGHT);
	name_box->SetLocation(box_y->GetRight() + HSPACE, ITEM_Y);
	name_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, name_box, ini_section](Control*) {
		string text = name_box->GetText();
		wstring wtext = wstring(text.begin(), text.end());
		self->set_name(wtext);
		GWToolbox::instance()->config()->iniWrite(ini_section.c_str(),
			self->IniNameKey(), wtext.c_str());
	});
	name_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	name_box->GetFocusLostEvent() += FocusLostEventHandler([](Control*, Control*) {
		GWToolbox::capture_input = false;
	});
	AddControl(name_box);
}

HotkeyDialog::HotkeyDialog(Key key, Key modifier, bool active, wstring ini_section, UINT id, wstring name)
	: TBHotkey(key, modifier, active, ini_section), id_(id), name_(name) {

	HotkeyDialog* self = this;

	Label* label = new Label();
	label->SetLocation(ITEM_X, LABEL_Y);
	label->SetText("Dialog");
	AddControl(label);

	Label* label_id = new Label();
	label_id->SetLocation(label->GetRight() + HSPACE, LABEL_Y);
	label_id->SetText("ID:");
	AddControl(label_id);

	int width_left = WIDTH - label_id->GetRight();
	TextBox* id_box = new TextBox();
	id_box->SetText(to_string(id_));
	id_box->SetSize(width_left / 2 - HSPACE / 2, LINE_HEIGHT);
	id_box->SetLocation(label_id->GetRight(), ITEM_Y);
	id_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, id_box, ini_section](Control*) {
		try {
			long id = std::stol(id_box->GetText());
			self->set_id((UINT)id);
			GWToolbox::instance()->config()->iniWriteLong(ini_section.c_str(),
				self->IniDialogIDKey(), id);
		} catch (...) {}
	});
	id_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	id_box->GetFocusLostEvent() += FocusLostEventHandler([id_box](Control*, Control*) {
		GWToolbox::capture_input = false;
		try {
			std::stol(id_box->GetText());
		} catch (...) {
			id_box->SetText("0");
		}
	});
	AddControl(id_box);

	TextBox* name_box = new TextBox();
	string text = string(name_.begin(), name_.end());
	name_box->SetText(text.c_str());
	name_box->SetSize(width_left / 2 - HSPACE / 2, LINE_HEIGHT);
	name_box->SetLocation(id_box->GetRight() + HSPACE, ITEM_Y);
	name_box->GetTextChangedEvent() += TextChangedEventHandler(
		[self, name_box, ini_section](Control*) {
		string text = name_box->GetText();
		wstring wtext = wstring(text.begin(), text.end());
		self->set_name(wtext);
		GWToolbox::instance()->config()->iniWrite(ini_section.c_str(),
			self->IniDialogNameKey(), wtext.c_str());
	});
	name_box->GetFocusGotEvent() += FocusGotEventHandler([](Control*) {
		GWToolbox::capture_input = true;
	});
	name_box->GetFocusLostEvent() += FocusLostEventHandler([](Control*, Control*) {
		GWToolbox::capture_input = false;
	});
	AddControl(name_box);
}

void HotkeyUseItem::exec() {
	if (!isExplorable()) return;

	if (GWAPI::GWAPIMgr::GetInstance()->Items->UseItemByModelId(item_id_)) {
		LOG("Hotkey fired: used item %s (ID: %u)", item_name_, item_id_);
	} else {
		wstring name = item_name_.empty() ? to_wstring(item_id_) : item_name_;
		wstring msg = wstring(L"[Warning] ") + item_name_ + L"not found!";
		GWAPI::GWAPIMgr::GetInstance()->Chat->WriteChat(msg.c_str());
		LOGW(msg.c_str());
	}
}

void HotkeySendChat::exec() {
	if (isLoading()) return;

	GWAPIMgr::GetInstance()->Chat->SendChat(msg_.c_str(), channel_);
	if (channel_ == L'/') {
		GWAPIMgr::GetInstance()->Chat->WriteChat((wstring(L"/") + msg_).c_str());
	}
}

void HotkeyDropUseBuff::exec() {
	if (!isExplorable()) return;

	GWAPIMgr* API = GWAPIMgr::GetInstance();
	Buff buff = API->Effects->GetPlayerBuffBySkillId(skillID_);
	if (buff.SkillId) {
		API->Effects->DropBuff(buff.BuffId);
	} else {
		int slot = API->Skillbar->getSkillSlot(skillID_);
		if (slot > 0 && API->Skillbar->GetPlayerSkillbar().Skills[slot].Recharge == 0) {
			API->Skillbar->UseSkill(slot, API->Agents->GetTargetId());
		}
	}
}

void HotkeyToggle::exec() {
	GWToolbox* tb = GWToolbox::instance();
	switch (target_) {
	case HotkeyToggle::Clicker:
		tb->main_window()->hotkey_panel()->toggleClicker();
		break;
	case HotkeyToggle::Pcons:
		tb->main_window()->pcon_panel()->toggleActive();
		break;
	case HotkeyToggle::CoinDrop:
		tb->main_window()->hotkey_panel()->toggleCoinDrop();
		break;
	case HotkeyToggle::RuptBot:
		tb->main_window()->hotkey_panel()->toggleRupt();
		break;
	}
}

void HotkeyTarget::exec() {
	if (isLoading()) return;

	GWAPIMgr* API = GWAPIMgr::GetInstance();
	Agent* me = API->Agents->GetPlayer();
	AgentArray agents = API->Agents->GetAgentArray();

	unsigned long distance = GwConstants::SqrRange::Compass;
	int closest = -1;

	for (size_t i = 0; i < agents.size(); ++i) {
		if (agents[i]->PlayerNumber == targetID_ && agents[i]->HP >= 0) {

			unsigned long newDistance = API->Agents->GetSqrDistance(me, agents[i]);
			if (newDistance < distance) {
				closest = i;
				distance = newDistance;
			}
		}
	}
	if (closest > 0) {
		API->Agents->ChangeTarget(agents[closest]);
	}
}

void HotkeyMove::exec() {
	if (!isExplorable()) return;

	GWAPIMgr* API = GWAPIMgr::GetInstance();
	Agent* me = API->Agents->GetPlayer();
	double sqrDist = (me->X - x_) * (me->X - x_) + (me->Y - y_) * (me->Y - y_);
	if (sqrDist < GwConstants::SqrRange::Compass) {
		API->Agents->Move(x_, y_);
	}
	GWAPIMgr::GetInstance()->Chat->WriteChat(L"Movement macro activated");
}

void HotkeyDialog::exec() {
	if (isLoading()) return;

	GWAPIMgr::GetInstance()->Agents->Dialog(id_);
}

void HotkeyPingBuild::exec() {
	if (isLoading()) return;

	// TODO
}