#include "DashboardTriggerItem.h"
DashboardTriggerItem::DashboardTriggerItem(Trigger * item) :
	DashboardControllableItem(item),
	trigger(item)
{
	bgColor = addColorParameter("Background Color", "Color of the background", BG_COLOR.brighter(), false);
	bgColor->canBeDisabledByUser = true;
	customImagePath = addFileParameter("Custom Image", "Choose a custom image instead of the default button");
	customImagePath->fileTypeFilter = "*.png;*.jpg;*.jpeg;*.gif";

	setInspectable(item);
	ghostInspectable();
}

DashboardTriggerItem::~DashboardTriggerItem()
{
	if (trigger != nullptr && !inspectable.wasObjectDeleted()) trigger->removeTriggerListener(this);
}

DashboardItemUI* DashboardTriggerItem::createUI()
{
	return new DashboardTriggerItemUI(this);
}

void DashboardTriggerItem::setInspectableInternal(Inspectable* i)
{
	DashboardControllableItem::setInspectableInternal(i);

	if (trigger != nullptr && !inspectable.wasObjectDeleted())
	{
		trigger->removeTriggerListener(this);
	}

	trigger = dynamic_cast<Trigger*>(i);

	if (trigger != nullptr)
	{
		trigger->addTriggerListener(this);
	}
}

void DashboardTriggerItem::onExternalTriggerTriggered(Trigger* t)
{
	DashboardControllableItem::onExternalTriggerTriggered(t);

	if (t == trigger)
	{
		var data(new DynamicObject());
		data.getDynamicObject()->setProperty("controlAddress", trigger->getControlAddress());
		notifyDataFeedback(data);
	}
}

var DashboardTriggerItem::getServerData()
{
	var data = DashboardControllableItem::getServerData();
	if (bgColor->enabled) data.getDynamicObject()->setProperty("bgColor", bgColor->value);
	if (customImagePath->enabled) data.getDynamicObject()->setProperty("customImage", customImagePath->value);
	return data;
}
