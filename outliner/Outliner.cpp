#include "Outliner.h"
/*
  ==============================================================================

	Outliner.cpp
	Created: 7 Oct 2016 10:31:23am
	Author:  bkupe

  ==============================================================================
*/


juce_ImplementSingleton(Outliner)

Outliner::Outliner(const String &contentName) :
	ShapeShifterContentComponent(contentName),
	enabled(true)
{

	if (Engine::mainEngine != nullptr) Engine::mainEngine->addControllableContainerListener(this);

	showHiddenContainers = false;

	rootItem.reset(new OutlinerItem(Engine::mainEngine));
	treeView.setRootItem(rootItem.get());
	treeView.setRootItemVisible(false);
	addAndMakeVisible(treeView);
	treeView.getViewport()->setScrollBarThickness(10);

	rebuildTree();

	helpID = "Outliner";

}

Outliner::~Outliner()
{
	//DBG("Outliner destroy, engine ?" << (int)Engine::mainEngine);
	treeView.setRootItem(nullptr);
	if (Engine::mainEngine != nullptr) Engine::mainEngine->removeControllableContainerListener(this);

}

void Outliner::clear()
{
	rootItem->clearSubItems();
}

void Outliner::setEnabled(bool value)
{
	if (enabled == value) return;
	if (enabled)
	{
		Engine::mainEngine->addControllableContainerListener(this);
		rebuildTree();
	}
	else
	{
		clear();
		Engine::mainEngine->removeControllableContainerListener(this);
	}
}

void Outliner::resized()
{
	juce::Rectangle<int> r = getLocalBounds();
	r.removeFromTop(20);
	treeView.setBounds(r);
}

void Outliner::paint(Graphics & g)
{
	//g.fillAll(Colours::green.withAlpha(.2f));
}


void Outliner::rebuildTree(ControllableContainer* fromContainer)
{
	if (Engine::mainEngine == nullptr) return;

	if (fromContainer == nullptr) fromContainer = Engine::mainEngine;
	OutlinerItem* fromItem = getItemForContainer(fromContainer);
	fromItem->clearSubItems();

	std::unique_ptr<XmlElement> os = treeView.getOpennessState(true);
	buildTree(fromItem, fromContainer);
	rootItem->setOpen(true);

	treeView.restoreOpennessState(*os, true);
}

void Outliner::buildTree(OutlinerItem * parentItem, ControllableContainer * parentContainer)
{
	if (parentContainer == nullptr) return;
	Array<WeakReference<ControllableContainer>> childContainers = parentContainer->getAllContainers(false);
	
	parentContainer->controllableContainers.getLock().enter();
	for (auto &cc : parentContainer->controllableContainers)
	{
		/*if (cc->skipControllableNameInAddress && !showHiddenContainers)
		{
			buildTree(parentItem, cc);
		}
		else
		{*/
			OutlinerItem * ccItem = new OutlinerItem(cc);
			parentItem->addSubItem(ccItem);

			buildTree(ccItem, cc);
		//}
	}
	parentContainer->controllableContainers.getLock().exit();

	parentContainer->controllables.getLock().enter();
	for (auto &c : parentContainer->controllables)
	{
		if (c->hideInOutliner) continue;
		OutlinerItem * cItem = new OutlinerItem(c);
		parentItem->addSubItem(cItem);
	}
	parentContainer->controllables.getLock().exit();

}

OutlinerItem* Outliner::getItemForContainer(ControllableContainer* cc)
{
	if (cc == nullptr || cc == Engine::mainEngine) return rootItem.get();

	Array<int> containerIndices;
	ControllableContainer* cParent = cc;
	while (cParent != Engine::mainEngine && cParent != nullptr)
	{
		ControllableContainer* parent = cParent->parentContainer;
		if (parent == nullptr) return nullptr;
		containerIndices.add(parent->controllableContainers.indexOf(cParent));
	}

	OutlinerItem* result = rootItem.get();
	for (auto& index : containerIndices)
	{
		jassert(result != nullptr);
		result = (OutlinerItem *)result->getSubItem(index);
	}

	return result;
}

void Outliner::childStructureChanged(ControllableContainer *)
{
	if (enabled)
	{
		MessageManagerLock mmLock;
		rebuildTree();
	}

}


// OUTLINER ITEM

OutlinerItem::OutlinerItem(WeakReference<ControllableContainer> _container) :
	InspectableContent(_container),
	isContainer(true),
	itemName(_container->niceName),
	container(_container),
	controllable(nullptr)
{
	container->addControllableContainerListener(this);
}

OutlinerItem::OutlinerItem(WeakReference<Controllable> _controllable) :
	InspectableContent(_controllable),
	isContainer(false),
	itemName(_controllable->niceName),
	container(nullptr),
	controllable(_controllable)
{
}

OutlinerItem::~OutlinerItem()
{
	if (isContainer) container->removeControllableContainerListener(this);
	masterReference.clear();
}


bool OutlinerItem::mightContainSubItems()
{
	return isContainer;
}

Component * OutlinerItem::createItemComponent()
{
	return new OutlinerItemComponent(this);
}

String OutlinerItem::getUniqueName() const
{
	if ((isContainer && container.wasObjectDeleted()) || (!isContainer && controllable.wasObjectDeleted())) return "";

	String n = isContainer ? container.get()->getControlAddress() : controllable.get()->getControlAddress();
	if (n.isEmpty()) n = isContainer ? container->shortName : controllable->shortName;

	return n;
}


void OutlinerItem::inspectableSelectionChanged(Inspectable * i)
{
	InspectableContent::inspectableSelectionChanged(i);

	MessageManagerLock mmLock;
	if (!mmLock.lockWasGained()) return;

	setSelected(inspectable->isSelected, true);

	if (inspectable->isSelected)
	{
		//close everything before ?

		//open all parents to view the item
		if (!areAllParentsOpen())
		{
			TreeViewItem * ti = getParentItem();
			while (ti != nullptr)
			{
				if (!ti->isOpen()) ti->setOpen(true);
				ti = ti->getParentItem();
			}
		}
	}

}

void OutlinerItemComponent::itemNameChanged()
{
	label.setText(item->container->niceName, dontSendNotification);
}




// OutlinerItemComponent

OutlinerItemComponent::OutlinerItemComponent(OutlinerItem * _item) :
	InspectableContentComponent(_item->inspectable),
	item(_item),
	label("label", _item->itemName)
{
	item->addItemListener(this);
	autoDrawContourWhenSelected = false;
	setTooltip(item->isContainer ? item->container->getControlAddress() : item->controllable->description + "\nControl Address : " + item->controllable->controlAddress);
	addAndMakeVisible(&label);


	label.setFont(label.getFont().withHeight(12));
	label.setColour(label.backgroundWhenEditingColourId, Colours::white);


	if (item->isContainer && item->container->nameCanBeChangedByUser)
	{
		label.addListener(this);
		label.setEditable(false, true);
		label.addMouseListener(this, true);
	}
	else
	{
		label.setInterceptsMouseClicks(false, false);
	}
}

OutlinerItemComponent::~OutlinerItemComponent()
{
	if (!item.wasObjectDeleted()) item->removeItemListener(this);
}

void OutlinerItemComponent::paint(Graphics & g)
{
	juce::Rectangle<int> r = getLocalBounds();

	Colour c = BLUE_COLOR;
	if (inspectable.wasObjectDeleted()) return;
	if (item->isContainer) c = item->container->nameCanBeChangedByUser ? HIGHLIGHT_COLOR : TEXT_COLOR;


	int labelWidth = label.getFont().getStringWidth(label.getText());

	if (item->isSelected())
	{
		g.setColour(c);
		g.fillRoundedRectangle(r.withSize(labelWidth + 20, r.getHeight()).toFloat(), 2);
	}

	r.removeFromLeft(3);
	label.setBounds(r);
	label.setColour(Label::textColourId, item->isSelected() ? Colours::grey.darker() : c);
}


void OutlinerItem::childAddressChanged(ControllableContainer *)
{
	if (isContainer)
	{
		itemName = container->niceName;
		itemListeners.call(&OutlinerItemListener::itemNameChanged);
	}
}
void OutlinerItemComponent::labelTextChanged(Label *)
{
	if (item.wasObjectDeleted()) return;
	item->container->setUndoableNiceName(label.getText());
}

void OutlinerItemComponent::mouseDown(const MouseEvent &e)
{
	InspectableContentComponent::mouseDown(e);

	if (e.mods.isRightButtonDown())
	{
		PopupMenu p;
		p.addItem(-1, "Copy OSC Control Address");
		p.addItem(-2, "Copy Script Control Address");
		
		p.addSeparator();

		if (item->controllable != nullptr)
		{
			Controllable::Type cType = item->controllable->type;
			if (cType == Controllable::FLOAT ||
				cType == Controllable::INT ||
				cType == Controllable::POINT2D ||
				cType == Controllable::POINT3D ||
				cType == Controllable::COLOR ||
				cType == Controllable::BOOL )
			{
				p.addItem(-10, "Watch this with The Detective");
			}
		}

		PopupMenu dashboardMenu;
		int index = 0;
		for (auto& di : DashboardManager::getInstance()->items)
		{
			dashboardMenu.addItem(index + 10000, di->niceName);
			index++;
		}
		p.addSubMenu("Send to Dashboard", dashboardMenu);
		
		int result = p.show();
		switch (result)
		{
		case -1:
			if (item->isContainer) SystemClipboard::copyTextToClipboard(item->container->getControlAddress());
			else SystemClipboard::copyTextToClipboard(item->controllable->controlAddress);
			break;
		case -2:
			if (item->isContainer)  SystemClipboard::copyTextToClipboard("root" + item->container->getControlAddress().replaceCharacter('/', '.'));
			else SystemClipboard::copyTextToClipboard("root" + item->controllable->controlAddress.replaceCharacter('/', '.'));
			break;

		case -10:
			Detective::getInstance()->watchControllable(item->controllable);
			break;

		default:
			if (result >= 10000)
			{
				DashboardManager::getInstance()->items[result - 10000]->itemManager.addItem((item->isContainer?item->container->createDashboardItem():item->controllable->createDashboardItem()));
			}
			break;

		}
	}
}
