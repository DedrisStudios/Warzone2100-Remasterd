/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2023  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "../hci.h"
#include "groups.h"
#include "objects_stats.h"
#include "../group.h"
#include "../game_world.h"
// DedrisReforged: repurposed into an SC2-style selected-units console.
#include "../selection.h"
#include "../droid.h"
#include "../display.h"
#include "../display3d.h"
#include "../map.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pieblitfunc.h"

static bool groupButtonEnabled = true;

void setGroupButtonEnabled(bool bNewState)
{
	groupButtonEnabled = bNewState;
}

bool getGroupButtonEnabled()
{
	return groupButtonEnabled;
}

class GroupsUIController: public std::enable_shared_from_this<GroupsUIController>
{
public:
	struct GroupDisplayInfo
	{
		size_t numberInGroup = 0;
		size_t numberDamagedInGroup = 0;
		size_t numberCommandedByGroup = 0; // the number of droids commanded by commanders in this group
		uint64_t totalGroupMaxHealth = 0;
		DROID_TEMPLATE displayDroidTemplate;

		// DedrisReforged (SC2 selection console): per selection-stack data.
		uint64_t totalCurrentBody = 0;          // sum of current body of units in this stack
		std::vector<uint32_t> componentKey;     // type signature (buildComponentsFromDroid) for sub-select
		uint32_t repDroidId = 0;                // representative unit (for centre-camera)

		uint8_t currAttackGlowAlpha = 0;

		// used for calculating display info
		uint32_t lastAttackedGameTime = 0;
		uint64_t lastAccumulatedDamage = 0;
		uint64_t lastUnitsKilled = 0;
	};
public:
	GroupDisplayInfo* getGroupInfoAt(size_t index)
	{
		ASSERT_OR_RETURN(nullptr, index < groupInfo.size(), "Invalid group index (%zu); max: (%zu)", index, groupInfo.size());
		return &groupInfo[index];
	}

	size_t size() const
	{
		return groupInfo.size();
	}

	void updateData();

	void addGroupDamageForCurrentTick(size_t group, uint64_t additionalDamage, bool unitKilled)
	{
		auto& curr = groupInfo[group];
		curr.lastAttackedGameTime = gameTime;
		curr.lastAccumulatedDamage += additionalDamage;
		if (unitKilled)
		{
			++curr.lastUnitsKilled;
			curr.lastAccumulatedDamage = std::max<uint64_t>(curr.lastAccumulatedDamage, 1);
		}
	}

	void addCommanderGroupDamageForCurrentTick(const DROID *psCommander, uint64_t additionalDamage, bool unitKilled)
	{
		if (psCommander->group >= groupInfo.size())
		{
			return;
		}
		addGroupDamageForCurrentTick(psCommander->group, additionalDamage, unitKilled);
	}

	void selectGroup(size_t groupNumber)
	{
		// select the group
		kf_SelectGrouping(groupNumber);
	}

	void assignSelectedDroidsToGroup(size_t groupNumber)
	{
		assignObjectToGroup(selectedPlayer, groupNumber, true);
	}

	// DedrisReforged (SC2 selection console): number of populated selection stacks.
	size_t activeStacks() const { return numActiveStacks; }

	// Narrow the current selection down to just the units of the clicked stack's type.
	void subSelectStack(size_t slot)
	{
		if (slot >= numActiveStacks || selectedPlayer >= MAX_PLAYERS) { return; }
		const std::vector<uint32_t> key = groupInfo[slot].componentKey;
		std::vector<DROID*> keep;
		for (DROID* psDroid : gameWorld.objects.droids[selectedPlayer])
		{
			if (psDroid->selected && buildComponentsFromDroid(psDroid) == key)
			{
				keep.push_back(psDroid);
			}
		}
		if (keep.empty()) { return; }
		clearSelection();
		for (DROID* psDroid : keep)
		{
			SelectDroid(psDroid, true /* programmatic: no script-event spam */);
		}
	}

	// Centre the camera on the clicked stack's representative unit.
	void centerOnStack(size_t slot)
	{
		if (slot >= numActiveStacks || selectedPlayer >= MAX_PLAYERS) { return; }
		const uint32_t id = groupInfo[slot].repDroidId;
		for (DROID* psDroid : gameWorld.objects.droids[selectedPlayer])
		{
			if (psDroid->id == id)
			{
				Vector2i t = map_coord(Vector2i(psDroid->pos.x, psDroid->pos.y));
				setViewPos(t.x, t.y, true);
				return;
			}
		}
	}

private:
	std::array<GroupDisplayInfo, 10> groupInfo;
	size_t numActiveStacks = 0;
};

class GroupButton : public DynamicIntFancyButton
{
private:
	typedef DynamicIntFancyButton BaseWidget;
	std::shared_ptr<GroupsUIController> controller;
	std::shared_ptr<W_LABEL> groupCountLabel;
	size_t slot;
protected:
	GroupButton() : DynamicIntFancyButton() { }
public:
	static std::shared_ptr<GroupButton> make(const std::shared_ptr<GroupsUIController>& controller, size_t slot)
	{
		class make_shared_enabler: public GroupButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->slot = slot;
		widget->initialize();
		return widget;
	}

	void initialize()
	{
		// DedrisReforged: SC2-style selected-units cell — count of units of this type,
		// bright, top-right of the cell.
		attach(groupCountLabel = std::make_shared<W_LABEL>());
		groupCountLabel->setGeometry(OBJ_TEXTX + 40, OBJ_B1TEXTY + 20, 16, 16);
		groupCountLabel->setFontColour(pal_RGBA(220, 237, 228, 255));
		groupCountLabel->setString("");
		groupCountLabel->setTransparentToMouse(true);

		buttonBackgroundEmpty = true;

		setTip("Seleziona solo queste unità (tasto destro o tieni premuto: centra la vista)");

		setHelp(WidgetHelp()
		  .setTitle(WzString::fromUtf8("Unità selezionate"))
		  .addInteraction({WidgetHelp::InteractionTriggers::PrimaryClick}, WzString::fromUtf8("Seleziona solo le unità di questo tipo"))
		  .addInteraction({WidgetHelp::InteractionTriggers::SecondaryClick, WidgetHelp::InteractionTriggers::ClickAndHold}, WzString::fromUtf8("Centra la telecamera su queste unità")));
	}

	void clickPrimary() override
	{
		controller->subSelectStack(slot);
	}

	void clickSecondary(bool synthesizedFromHold) override
	{
		controller->centerOnStack(slot);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		auto groupInfo = controller->getGroupInfoAt(slot);
		if (!groupInfo || slot >= controller->activeStacks() || groupInfo->numberInGroup == 0)
		{
			groupCountLabel->setString("");
			displayBlank(xOffset, yOffset, false);
			return;
		}

		// 3D portrait of the representative unit of this selection stack.
		displayIMD(AtlasImage(), ImdObject::DroidTemplate(&(groupInfo->displayDroidTemplate)), xOffset, yOffset);

		// Count of units of this type in the current selection, top-right.
		groupCountLabel->setString(WzString::format("%zu", groupInfo->numberInGroup));
		int32_t xNumberOffset = 0;
		const uint32_t xFitNumberInTheBox = 16;
		if (groupCountLabel->getMaxLineWidth() > xFitNumberInTheBox)
		{
			xNumberOffset -= groupCountLabel->getMaxLineWidth() - xFitNumberInTheBox;
		}
		groupCountLabel->move(OBJ_TEXTX + 40 + xNumberOffset, groupCountLabel->y());

		// Health bar along the bottom of the cell, coloured by aggregate body %.
		if (groupInfo->totalGroupMaxHealth > 0)
		{
			const int pct = static_cast<int>((groupInfo->totalCurrentBody * 100) / groupInfo->totalGroupMaxHealth);
			const int clamped = std::max(0, std::min(100, pct));
			const int bx0 = xOffset + x() + 3;
			const int bx1 = xOffset + x() + width() - 3;
			const int by1 = yOffset + y() + height() - 3;
			const int by0 = by1 - 3;
			pie_UniTransBoxFill(bx0, by0, bx1, by1, pal_RGBA(10, 15, 13, 220));
			const int fillW = ((bx1 - bx0) * clamped) / 100;
			const PIELIGHT hpCol = (pct > REPAIRLEV_HIGH) ? WZCOL_HEALTH_HIGH : ((pct > REPAIRLEV_LOW) ? WZCOL_HEALTH_MEDIUM : WZCOL_HEALTH_LOW);
			if (fillW > 0)
			{
				pie_UniTransBoxFill(bx0, by0, bx0 + fillW, by1, hpCol);
			}
		}
	}
	bool isHighlighted() const override
	{
		return false;
	}
};

void GroupsForum::display(int xOffset, int yOffset)
{
	// draw the background
	BaseWidget::display(xOffset, yOffset);
}

void GroupsForum::run(W_CONTEXT *psContext)
{
	BaseWidget::run(psContext);
	// DedrisReforged: keep the SC2-style selected-units console live (selection + health)
	// at ~10Hz without an O(N) rescan every frame.
	if (realTime - lastSelRefreshTime >= 100)
	{
		groupsUIController->updateData();
		lastSelRefreshTime = realTime;
	}
}

void GroupsForum::initialize()
{
	groupsUIController = std::make_shared<GroupsUIController>();

	// the layout should be like this when the build menu is open
	id = IDOBJ_GROUP;
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(GROUP_BACKX, GROUP_BACKY, GROUP_BACKWIDTH, GROUP_BACKHEIGHT);
	}));
	addTabList();
	// DedrisReforged: cells are selection stacks (slot 0..9), populated by updateData().
	for (size_t slot = 0; slot < 10; slot++)
	{
		auto buttonHolder = std::make_shared<WIDGET>();
		groupsList->addWidgetToLayout(buttonHolder);
		auto groupButton = makeGroupButton(slot);
		buttonHolder->attach(groupButton);
		groupButton->setGeometry(0, 0, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	}

	setHelp(WidgetHelp()
	  .setTitle(WzString::fromUtf8("Unità selezionate"))
	  .setDescription(WzString::fromUtf8("Mostra le unità selezionate, raggruppate per tipo."))
	  .addInteraction({WidgetHelp::InteractionTriggers::PrimaryClick}, WzString::fromUtf8("Seleziona solo le unità di quel tipo"))
	  .addInteraction({WidgetHelp::InteractionTriggers::SecondaryClick, WidgetHelp::InteractionTriggers::ClickAndHold}, WzString::fromUtf8("Centra la telecamera su quelle unità")));
}

void GroupsUIController::updateData()
{
	// DedrisReforged: SC2-style — reflect the CURRENT SELECTION, grouped by unit type
	// (buildComponentsFromDroid signature), instead of keyboard control-groups.
	struct StackAccum
	{
		size_t count = 0;
		size_t damaged = 0;
		uint64_t maxHealth = 0;
		uint64_t curHealth = 0;
		DROID *rep = nullptr;
		std::vector<uint32_t> key;
	};

	std::vector<StackAccum> stacks;
	std::map<std::vector<uint32_t>, size_t> keyToIdx;

	if (selectedPlayer < MAX_PLAYERS)
	{
		for (DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
		{
			if (!psDroid->selected)
			{
				continue;
			}
			std::vector<uint32_t> key = buildComponentsFromDroid(psDroid);
			size_t idx;
			auto it = keyToIdx.find(key);
			if (it == keyToIdx.end())
			{
				idx = stacks.size();
				keyToIdx[key] = idx;
				stacks.emplace_back();
				stacks[idx].rep = psDroid;
				stacks[idx].key = key;
			}
			else
			{
				idx = it->second;
			}
			StackAccum &s = stacks[idx];
			s.count++;
			s.maxHealth += psDroid->originalBody;
			s.curHealth += psDroid->body;
			if (psDroid->body < psDroid->originalBody)
			{
				s.damaged++;
			}
		}
	}

	numActiveStacks = std::min<size_t>(stacks.size(), groupInfo.size());
	for (size_t idx = 0; idx < groupInfo.size(); ++idx)
	{
		GroupDisplayInfo &info = groupInfo[idx];
		if (idx < numActiveStacks)
		{
			const StackAccum &s = stacks[idx];
			info.numberInGroup = s.count;
			info.numberDamagedInGroup = s.damaged;
			info.numberCommandedByGroup = 0;
			info.totalGroupMaxHealth = s.maxHealth;
			info.totalCurrentBody = s.curHealth;
			info.componentKey = s.key;
			info.repDroidId = (s.rep) ? s.rep->id : 0;
			templateSetParts(s.rep, &(info.displayDroidTemplate));
		}
		else
		{
			info.numberInGroup = 0;
			info.numberDamagedInGroup = 0;
			info.numberCommandedByGroup = 0;
			info.totalGroupMaxHealth = 0;
			info.totalCurrentBody = 0;
			info.componentKey.clear();
			info.repDroidId = 0;
		}
	}
}

void GroupsForum::updateData()
{
	groupsUIController->updateData();
}

void GroupsForum::updateSelectedGroup(size_t group)
{
	size_t groupButtonIdx = (group > 0) ? group - 1 : 9;
	if (groupButtonIdx >= groupsList->childrenSize())
	{
		return;
	}
	// ensure that the page that contains this group is displayed
	groupsList->goToChildPage(groupButtonIdx);
}

void GroupsForum::addGroupDamageForCurrentTick(size_t group, uint64_t additionalDamage, bool unitKilled)
{
	groupsUIController->addGroupDamageForCurrentTick(group, additionalDamage, unitKilled);
}

void GroupsForum::addCommanderGroupDamageForCurrentTick(const DROID *psCommander, uint64_t additionalDamage, bool unitKilled)
{
	groupsUIController->addCommanderGroupDamageForCurrentTick(psCommander, additionalDamage, unitKilled);
}

void GroupsForum::addTabList()
{
	attach(groupsList = IntListTabWidget::make(TabAlignment::RightAligned));
	groupsList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT * 2);
	groupsList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int groupListWidth = OBJ_BUTWIDTH * 5 + STAT_GAP * 4;
	groupsList->setGeometry((OBJ_BACKWIDTH - groupListWidth) / 2, OBJ_TABY, groupListWidth, OBJ_BACKHEIGHT - OBJ_TABY);
	WzString unitGroupsStr = WzString::fromUtf8("Unità selezionate:");
	unitGroupsStr += " ";
	groupsList->setTitle(unitGroupsStr);
}

std::shared_ptr<GroupButton> GroupsForum::makeGroupButton(size_t groupNumber)
{
	return GroupButton::make(groupsUIController, groupNumber);
}

void GroupsForum::setHelp(optional<WidgetHelp> _help)
{
	help = _help;
}
