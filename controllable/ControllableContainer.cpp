#include "ControllableContainer.h"
/*
 ==============================================================================

 ControllableContainer.cpp
 Created: 8 Mar 2016 1:15:36pm
 Author:  bkupe

 ==============================================================================
 */

ControllableComparator ControllableContainer::comparator;

ControllableContainer::ControllableContainer(const String & niceName) :
	ScriptTarget("", this),
	hasCustomShortName(false),
	nameCanBeChangedByUser(false),
	canInspectChildContainers(true),
	editorIsCollapsed(false),
	editorCanBeCollapsed(true),
	hideEditorHeader(false),
	skipLabelInTarget(false),
	userCanAddControllables(false),
	customUserCreateControllableFunc(nullptr),
	customGetEditorFunc(nullptr),
	saveAndLoadRecursiveData(false),
	saveAndLoadName(false),
	includeInRecursiveSave(true),
	includeTriggersInSaveLoad(false),
	isCurrentlyLoadingData(false),
	notifyStructureChangeWhenLoadingData(true),
    includeInScriptObject(true),
    parentContainer(nullptr),
    queuedNotifier(500) //what to put in max size ??
						//500 seems ok on my computer, but if too low, generates leaks when closing app while heavy use of async (like  parameter update from audio signal)
{
	setNiceName(niceName);

	//script
	scriptObject.setMethod("getChild", ControllableContainer::getChildFromScript);
	scriptObject.setMethod("getParent", ControllableContainer::getParentFromScript);
	scriptObject.setMethod("setName", ControllableContainer::setNameFromScript);
}

ControllableContainer::~ControllableContainer()
{
	//controllables.clear();
	//DBG("CLEAR CONTROLLABLE CONTAINER");
	clear();
	masterReference.clear();
}


void ControllableContainer::clear() {

	{
		ScopedLock lock(controllables.getLock());
		controllables.clear();
	}
	
	queuedNotifier.cancelPendingUpdate();

	controllableContainers.clear();
	ownedContainers.clear();
}


UndoableAction * ControllableContainer::addUndoableControllable(Controllable * c, bool onlyReturnAction)
{
	if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile)
	{
		//if Main Engine loading, just set the value without undo history
		addControllable(c);
		return nullptr;
	}

	UndoableAction * a = new AddControllableAction(this, c);
	if (onlyReturnAction) return a;

	UndoMaster::getInstance()->performAction("Add " + c->niceName, a);
	return a;
}

void ControllableContainer::addControllable(Controllable * c)
{
	if (c->type == Controllable::TRIGGER) addTriggerInternal((Trigger *)c);
	else addParameterInternal((Parameter *)c);

	c->addControllableListener(this);
	c->addAsyncWarningTargetListener(this);
	c->warningResolveInspectable = this;
}

void ControllableContainer::addParameter(Parameter * p)
{
	addControllable(p);
}

FloatParameter * ControllableContainer::addFloatParameter(const String & _niceName, const String & description, const float & initialValue, const float & minValue, const float & maxValue, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile)?_niceName:getUniqueNameInContainer(_niceName);
	FloatParameter * p = new FloatParameter(targetName, description, initialValue, minValue, maxValue, enabled);
	addControllable(p);
	return p;
}

IntParameter * ControllableContainer::addIntParameter(const String & _niceName, const String & _description, const int & initialValue, const int & minValue, const int & maxValue, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	IntParameter * p = new IntParameter(targetName, _description, initialValue, minValue, maxValue, enabled);
	addControllable(p);
	return p;
}

BoolParameter * ControllableContainer::addBoolParameter(const String & _niceName, const String & _description, const bool & value, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	BoolParameter * p = new BoolParameter(targetName, _description, value, enabled);
	addControllable(p);
	return p;
}

StringParameter * ControllableContainer::addStringParameter(const String & _niceName, const String & _description, const String &value, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	StringParameter * p = new StringParameter(targetName, _description, value, enabled);
	addControllable(p);
	return p;
}

EnumParameter * ControllableContainer::addEnumParameter(const String & _niceName, const String & _description, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	EnumParameter * p = new EnumParameter(targetName, _description, enabled);
	addControllable(p);
	return p;
}

Point2DParameter * ControllableContainer::addPoint2DParameter(const String & _niceName, const String & _description, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	Point2DParameter * p = new Point2DParameter(targetName, _description, enabled);
	addControllable(p);
	return p;
}

Point3DParameter * ControllableContainer::addPoint3DParameter(const String & _niceName, const String & _description, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	Point3DParameter * p = new Point3DParameter(targetName, _description, enabled);
	addControllable(p);
	return p;
}

ColorParameter * ControllableContainer::addColorParameter(const String & _niceName, const String & _description, const Colour &initialColor, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	ColorParameter * p = new ColorParameter(targetName, _description, initialColor, enabled);
	addControllable(p);
	return p;
}

TargetParameter * ControllableContainer::addTargetParameter(const String & _niceName, const String & _description, WeakReference<ControllableContainer> rootReference, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	TargetParameter * p = new TargetParameter(targetName, _description, "", rootReference, enabled);
	addControllable(p);
	return p;
}

FileParameter * ControllableContainer::addFileParameter(const String & _niceName, const String & _description, const String & _initialValue)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	FileParameter * p = new FileParameter(targetName, _description, _initialValue);
	addControllable(p);
	return p;
}

Trigger * ControllableContainer::addTrigger(const String & _niceName, const String & _description, const bool & enabled)
{
	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? _niceName : getUniqueNameInContainer(_niceName);
	Trigger * t = new Trigger(targetName, _description, enabled);
	addControllable(t);
	return t;
}


void ControllableContainer::addTriggerInternal(Trigger * t)
{
	{
		ScopedLock lock(controllables.getLock());
		controllables.add(t);
	}
	
	t->setParentContainer(this);
	t->addTriggerListener(this);
	onControllableAdded(t);
	controllableContainerListeners.call(&ControllableContainerListener::controllableAdded, t);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableAdded, this, t));
	notifyStructureChanged();
}

void ControllableContainer::addParameterInternal(Parameter * p)
{
	p->setParentContainer(this);

	{
		ScopedLock lock(controllables.getLock());
		controllables.add(p);
	}
	p->addParameterListener(this);
	p->addAsyncParameterListener(this);
	onControllableAdded(p);
	controllableContainerListeners.call(&ControllableContainerListener::controllableAdded, p);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableAdded, this, p));
	notifyStructureChanged();
}

UndoableAction * ControllableContainer::removeUndoableControllable(Controllable * c, bool onlyReturnAction)
{
	if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile)
	{
		//if Main Engine loading, just set the value without undo history
		addControllable(c);
		return nullptr;
	}

	UndoableAction * a = new RemoveControllableAction(this, c);
	if (onlyReturnAction) return a;

	UndoMaster::getInstance()->performAction("Remove " + c->niceName, a);
	return a;
}

void ControllableContainer::removeControllable(WeakReference<Controllable> c)
{
	if (c.wasObjectDeleted())
	{
		DBG("Remove controllable but ref was deleted");
		return;
	}

	controllableContainerListeners.call(&ControllableContainerListener::controllableRemoved, c);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableRemoved, this, c));

	Parameter * p = dynamic_cast<Parameter*>(c.get());
	if (p != nullptr) {
		p->removeParameterListener(this);
		p->removeAsyncParameterListener(this);
	}

	c->removeAsyncWarningTargetListener(this);
	c->removeControllableListener(this);

	onControllableRemoved(c);

	{
		ScopedLock lock(controllables.getLock());
		controllables.removeObject(c);
	}
	 
	notifyStructureChanged();
}


void ControllableContainer::notifyStructureChanged() 
{
	if (isCurrentlyLoadingData && !notifyStructureChangeWhenLoadingData) return;
	
	liveScriptObjectIsDirty = true;

	controllableContainerListeners.call(&ControllableContainerListener::childStructureChanged, this);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ChildStructureChanged, this));

}

void ControllableContainer::newMessage(const Parameter::ParameterEvent &e) 
{
	if (e.type == Parameter::ParameterEvent::VALUE_CHANGED) {
		onContainerParameterChangedAsync(e.parameter, e.value);
	}
}
void ControllableContainer::newMessage(const WarningTarget::WarningTargetEvent& e)
{
	switch (e.type)
	{
	case WarningTarget::WarningTargetEvent::WARNING_CHANGED:
		warningChanged(e.target);
		break;
	}
}

UndoableAction * ControllableContainer::setUndoableNiceName(const String & newNiceName, bool onlyReturnAction)
{
	if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile)
	{
		//if Main Engine loading, just set the value without undo history
		setNiceName(newNiceName);
		return nullptr;
	}

	UndoableAction * a = new ControllableContainerChangeNameAction(this, niceName, newNiceName);
	if (onlyReturnAction) return a;

	UndoMaster::getInstance()->performAction("Rename " + niceName, a);
	return a;	
}
void ControllableContainer::setNiceName(const String &_niceName) {
	if (niceName == _niceName) return;
	niceName = _niceName;
	if (!hasCustomShortName) setAutoShortName();
	liveScriptObjectIsDirty = true;
	onContainerNiceNameChanged();
}

void ControllableContainer::setCustomShortName(const String &_shortName) {
	shortName = _shortName;
	hasCustomShortName = true;
	scriptTargetName = shortName;
	liveScriptObjectIsDirty = true;
	updateChildrenControlAddress();
	onContainerShortNameChanged();
	controllableContainerListeners.call(&ControllableContainerListener::childAddressChanged, this);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ChildAddressChanged, this));

}

void ControllableContainer::setAutoShortName() {
	hasCustomShortName = false;
	shortName = StringUtil::toShortName(niceName,true);
	scriptTargetName = shortName;
	liveScriptObjectIsDirty = true;
	updateChildrenControlAddress();
	onContainerShortNameChanged();
	controllableContainerListeners.call(&ControllableContainerListener::childAddressChanged, this);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ChildAddressChanged, this));
}



Controllable * ControllableContainer::getControllableByName(const String & name, bool searchNiceNameToo, bool searchLowerCaseToo)
{
	for (auto &c : controllables)
	{
		if (c->shortName == name || (searchNiceNameToo && c->niceName == name) || (searchLowerCaseToo && c->shortName.toLowerCase() == name.toLowerCase())) return c;
	}

	return nullptr;
}

Parameter * ControllableContainer::getParameterByName(const String & name, bool searchNiceNameToo, bool searchLowerCaseToo)
{
	return dynamic_cast<Parameter *>(getControllableByName(name, searchNiceNameToo, searchLowerCaseToo));
}

void ControllableContainer::addChildControllableContainer(ControllableContainer * container, bool owned, int index, bool notify)
{

	String targetName = (Engine::mainEngine == nullptr || Engine::mainEngine->isLoadingFile) ? container->niceName : getUniqueNameInContainer(container->niceName);
	container->setNiceName(targetName);

	controllableContainers.insert(index, container);
	if (owned) ownedContainers.add(container);

	container->addControllableContainerListener(this);
	container->addAsyncWarningTargetListener(this);
	container->setParentContainer(this);

	if (Engine::mainEngine != nullptr && !Engine::mainEngine->isLoadingFile)
	{
		if (container->getWarningMessage().isNotEmpty()) warningChanged(container);
	}

	controllableContainerListeners.call(&ControllableContainerListener::controllableContainerAdded, container);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerAdded, this, container));

	if(notify) notifyStructureChanged();
}

void ControllableContainer::addChildControllableContainers(Array<ControllableContainer*> containers, bool owned, int index, bool notify)
{
	int i = index;
	for (auto &c : containers) addChildControllableContainer(c, owned, i++, false);
	if(notify) notifyStructureChanged();
}

void ControllableContainer::removeChildControllableContainer(ControllableContainer * container)
{
	
	this->controllableContainers.removeAllInstancesOf(container);
	container->removeControllableContainerListener(this);
	container->removeAsyncWarningTargetListener(this);

	if (Engine::mainEngine != nullptr && !Engine::mainEngine->isClearing)
	{
		warningChanged(container);
	}

	controllableContainerListeners.call(&ControllableContainerListener::controllableContainerRemoved, container);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerRemoved, this, container));

	notifyStructureChanged();
	container->setParentContainer(nullptr);
	if (ownedContainers.contains(container)) ownedContainers.removeObject(container);
}


ControllableContainer * ControllableContainer::getControllableContainerByName(const String & name, bool searchNiceNameToo, bool searchLowerCaseToo)
{
	for (auto &cc : controllableContainers)
	{
		if (!cc.wasObjectDeleted() && cc != nullptr && (
			cc->shortName == name || 
			(searchNiceNameToo && cc->niceName == name) || 
			(searchLowerCaseToo && cc->shortName.toLowerCase() == name.toLowerCase())
			))
			return cc;
	}

	
	return nullptr;

}

ControllableContainer * ControllableContainer::getControllableContainerForAddress(const String&  address, bool recursive, bool getNotExposed)
{
	StringArray addrArray;
	addrArray.addTokens(address, juce::StringRef("/"), juce::StringRef("\""));
	addrArray.remove(0);

	return getControllableContainerForAddress(addrArray, recursive, getNotExposed);
}

ControllableContainer * ControllableContainer::getControllableContainerForAddress(StringArray  addressSplit, bool recursive, bool getNotExposed)
{

	if (addressSplit.size() == 0) jassertfalse; // SHOULD NEVER BE THERE !

	bool isTargetFinal = addressSplit.size() == 1;

	if (isTargetFinal)
	{

		if (ControllableContainer * res = getControllableContainerByName(addressSplit[0]))   //get not exposed here here ?
			return res;

		//no found in direct children Container, maybe in a skip container ?
		for (auto &cc : controllableContainers)
		{
			if (cc == nullptr || cc.wasObjectDeleted()) continue;
			/*if (cc->skipControllableNameInAddress)
			{
				if (ControllableContainer * res = cc->getControllableContainerForAddress(addressSplit, recursive, getNotExposed)) return res;
			}*/
		}
	} else //if recursive here ?
	{
		for (auto &cc : controllableContainers)
		{
			if (cc == nullptr || cc.wasObjectDeleted()) continue;
			
			/*if (!cc->skipControllableNameInAddress)
			{
			*/
				if (cc->shortName == addressSplit[0])
				{
					addressSplit.remove(0);
					return cc->getControllableContainerForAddress(addressSplit, recursive, getNotExposed);
				}
			/*} else
			{
				ControllableContainer * tc = cc->getControllableContainerByName(addressSplit[0]);
				if (tc != nullptr)
				{
					addressSplit.remove(0);
					return tc->getControllableContainerForAddress(addressSplit, recursive, getNotExposed);
				}

			}*/
		}
	}

	return nullptr;

}

String ControllableContainer::getControlAddress(ControllableContainer * relativeTo) {

	StringArray addressArray;
	ControllableContainer * pc = this;
	while (pc != relativeTo && pc != nullptr && pc != Engine::mainEngine)
	{
		/*if (!pc->skipControllableNameInAddress)*/ addressArray.insert(0, pc->shortName);
		pc = pc->parentContainer;
	}
	if (addressArray.size() == 0)return "";
	else return "/" + addressArray.joinIntoString("/");
}

void ControllableContainer::orderControllablesAlphabetically()
{
	{
		ScopedLock lock(controllables.getLock());
		controllables.sort(ControllableContainer::comparator, true);
	}

	controllableContainerListeners.call(&ControllableContainerListener::controllableContainerReordered, this);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerReordered, this));

}

void ControllableContainer::setParentContainer(ControllableContainer * container)
{
	this->parentContainer = container;
	for (auto &c : controllables) if(c != nullptr) c->updateControlAddress();
	for (auto &cc : controllableContainers) if (!cc.wasObjectDeleted()) cc->updateChildrenControlAddress();

}

void ControllableContainer::updateChildrenControlAddress()
{
	for (auto& c : controllables)
	{
		if (c == nullptr)
		{
			jassertfalse; //should not be here
			continue;
		}
		c->updateControlAddress();
	}

	for (auto& cc : controllableContainers)
	{
		if (cc == nullptr)
		{
			jassertfalse; //should not be here
			continue;
		}
		cc->updateChildrenControlAddress();
	}

}

Array<WeakReference<Controllable>> ControllableContainer::getAllControllables(bool recursive, bool getNotExposed)
{
	Array<WeakReference<Controllable>> result;
	for (auto &c : controllables)
	{
		if (getNotExposed || c->isControllableExposed) result.add(c);
	}

	if (recursive)
	{
		for (auto &cc : controllableContainers)
		{
			if (cc.wasObjectDeleted() || cc.get() == nullptr) continue;
			result.addArray(cc->getAllControllables(true, getNotExposed));
		}
	}

	return result;

}

Array<WeakReference<Parameter>> ControllableContainer::getAllParameters(bool recursive, bool getNotExposed)
{
	Array<WeakReference<Parameter>> result;

	for (auto &c : controllables)
	{
		if (c->type == Controllable::Type::TRIGGER) continue;
		if (getNotExposed || c->isControllableExposed) {
			if (Parameter * cc = dynamic_cast<Parameter*>(c)) {
				result.add(cc);
			}
		}
	}

	if (recursive)
	{
		for (auto &cc : controllableContainers) result.addArray(cc->getAllParameters(true, getNotExposed));
	}

	return result;
}

Array<WeakReference<ControllableContainer>> ControllableContainer::getAllContainers(bool recursive)
{
	Array<WeakReference<ControllableContainer>> result;
	for (auto& cc : controllableContainers) 
	{
		if (cc.wasObjectDeleted() || cc.get() == nullptr) continue; 
		result.add(cc);
		if (recursive) result.addArray(cc->getAllContainers(true));
	}

	return result;
}



Controllable * ControllableContainer::getControllableForAddress(const String &address, bool recursive, bool getNotExposed)
{
	StringArray addrArray;
	addrArray.addTokens(address.startsWith("/")?address:"/"+address, juce::StringRef("/"), juce::StringRef("\""));
	addrArray.remove(0);

	return getControllableForAddress(addrArray, recursive, getNotExposed);
}

Controllable * ControllableContainer::getControllableForAddress(StringArray addressSplit, bool recursive, bool getNotExposed)
{
	if (addressSplit.size() == 0) jassertfalse; // SHOULD NEVER BE THERE !

	bool isTargetAControllable = addressSplit.size() == 1;

	if (isTargetAControllable)
	{
		Controllable* c = getControllableByName(addressSplit[0], false, true);
		if (c != nullptr)
		{
			if (c->isControllableExposed || getNotExposed) return c;
			else return nullptr;
			
		}

	} else  //if recursive here ?
	{
		ControllableContainer* cc = getControllableContainerByName(addressSplit[0], false, true);
		if (cc != nullptr)
		{
			addressSplit.remove(0);
			return cc->getControllableForAddress(addressSplit, recursive, getNotExposed);
		}

	}
	return nullptr;
}

bool ControllableContainer::containsControllable(Controllable * c, int maxSearchLevels)
{
	if (c == nullptr) return false;

	ControllableContainer * pc = c->parentContainer;
	if (pc == nullptr) return false;
	int curLevel = 0;

	while (pc != nullptr)
	{
		if (pc == this) return true;
		curLevel++;
		if (maxSearchLevels >= 0 && curLevel > maxSearchLevels) return false;
		pc = pc->parentContainer;
	}

	return false;
}


void ControllableContainer::dispatchFeedback(Controllable * c)
{
	//    @ben removed else here to enable containerlistener call back of non root (proxies) is it overkill?
	if (parentContainer != nullptr) { parentContainer->dispatchFeedback(c); }
	if (!c->isControllableExposed) return;

	controllableContainerListeners.call(&ControllableContainerListener::controllableFeedbackUpdate, this, c);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableFeedbackUpdate, this, c));
}

void ControllableContainer::dispatchState(Controllable* c)
{
	if (parentContainer != nullptr) { parentContainer->dispatchState(c); }
	if (!c->isControllableExposed) return;

	controllableContainerListeners.call(&ControllableContainerListener::controllableStateUpdate, this, c);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableStateUpdate, this, c));
}


void ControllableContainer::controllableStateChanged(Controllable* c)
{
	if (c->parentContainer == this) dispatchState(c);
}

void ControllableContainer::parameterValueChanged(Parameter * p)
{
	if (p->parentContainer == this)
	{
		onContainerParameterChanged(p);
		dispatchFeedback(p);
	} else
	{
		onExternalParameterValueChanged(p);
	}

}


void ControllableContainer::parameterRangeChanged(Parameter * p)
{
	if (p->parentContainer == this)
	{
		dispatchFeedback(p);
	}
	else
	{
		onExternalParameterRangeChanged(p);
	}
}


void ControllableContainer::triggerTriggered(Trigger * t)
{
	if (t->parentContainer == this) onContainerTriggerTriggered(t);
	else onExternalTriggerTriggered(t);


	if (t->isControllableExposed) dispatchFeedback(t);
}

void ControllableContainer::controllableFeedbackUpdate(ControllableContainer * cc, Controllable * c)
{ 
	onControllableFeedbackUpdate(cc, c); //This is the function to override from child classes
}

void ControllableContainer::controllableNameChanged(Controllable * c)
{
	notifyStructureChanged();
}

void ControllableContainer::askForRemoveControllable(Controllable * c, bool addToUndo)
{
	if (addToUndo) removeUndoableControllable(c);
	else removeControllable(c);
}

void ControllableContainer::warningChanged(WarningTarget *target)
{
	notifyWarningChanged();
	onWarningChanged(target);
	if (parentContainer != nullptr) parentContainer->warningChanged(target);
}

String ControllableContainer::getWarningMessage() const
{
	StringArray s;
	if(WarningTarget::getWarningMessage().isNotEmpty()) s.add(WarningTarget::getWarningMessage());

	
	for (auto& c : controllables)
	{
		if (c == nullptr) continue;
		String cs = c->getWarningMessage();
		if (cs.isNotEmpty())
		{
			s.add(c->parentContainer->niceName+" > "+c->niceName + " : " + cs);
		}
	}

	for (auto& cc : controllableContainers)
	{
		if (cc.wasObjectDeleted()) continue;
		String cs = cc->getWarningMessage();
		if (cs.isNotEmpty())
		{
			s.add(cs);
		}
	}

	return s.joinIntoString("\n");
}

String ControllableContainer::getWarningTargetName() const 
{ 
	return niceName;
}

var ControllableContainer::getJSONData()
{
	var data(new DynamicObject());

	var paramsData;

	
	Array<WeakReference<Controllable>> cont = getAllControllables(false, true);

	for (auto &wc : cont) {
		if (wc->type == Controllable::TRIGGER && !includeTriggersInSaveLoad) continue;
		if (wc.wasObjectDeleted()) continue;
		if (!wc->isSavable) continue;
		Parameter * p = dynamic_cast<Parameter *>(wc.get());
		if (p != nullptr && p->saveValueOnly && !p->isControllableFeedbackOnly && !p->isOverriden && !p->forceSaveValue && p->controlMode == Parameter::ControlMode::MANUAL) continue; //do not save parameters that have not changed. it should light up the file. But save custom-made parameters even if there not overriden !
		paramsData.append(wc->getJSONData(this));
	}

	//data.getDynamicObject()->setProperty("uid", uid.toString());
	if(paramsData.size() > 0) data.getDynamicObject()->setProperty("parameters", paramsData);
	
	if (saveAndLoadName)
	{
		data.getDynamicObject()->setProperty("niceName", niceName);
		if (hasCustomShortName) data.getDynamicObject()->setProperty("shortName", shortName);
	}

	if (editorIsCollapsed) data.getDynamicObject()->setProperty("editorIsCollapsed", true); //only set if true to avoid too much data


	if (saveAndLoadRecursiveData)
	{
		var containersData = new DynamicObject();
		for (auto &cc : controllableContainers)
		{
			if (!cc->includeInRecursiveSave) continue;
			
			var ccData = cc->getJSONData();
			if (ownedContainers.contains(cc))
			{
				ccData.getDynamicObject()->setProperty("owned", true);
				if(!saveAndLoadName) ccData.getDynamicObject()->setProperty("niceName", cc->niceName);
			}

			containersData.getDynamicObject()->setProperty(cc->shortName, ccData);
		}

		if(containersData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("containers", containersData);
	}

	return data;
}

void ControllableContainer::loadJSONData(var data, bool createIfNotThere)
{

	if (data.isVoid()) return;
	if (data.getDynamicObject() == nullptr) return;

	isCurrentlyLoadingData = true;
	//if (data.getDynamicObject()->hasProperty("uid")) uid = data.getDynamicObject()->getProperty("uid");
	if (data.getDynamicObject()->hasProperty("niceName")) setNiceName(data.getDynamicObject()->getProperty("niceName"));
	if (data.getDynamicObject()->hasProperty("shortName")) setCustomShortName(data.getDynamicObject()->getProperty("shortName"));
	if (data.getDynamicObject()->hasProperty("editorIsCollapsed")) editorIsCollapsed = data.getDynamicObject()->getProperty("editorIsCollapsed");

	Array<var> * paramsData = data.getDynamicObject()->getProperty("parameters").getArray();

	if (paramsData != nullptr)
	{
		for (var &pData : *paramsData)
		{
			DynamicObject * o = pData.getDynamicObject();
			String pControlAddress = o->getProperty("controlAddress");

			Controllable * c = getControllableForAddress(pControlAddress, false);

			if (c != nullptr)
			{
				if (Parameter * p = dynamic_cast<Parameter*>(c)) {
					if (p->isSavable) p->loadJSONData(pData.getDynamicObject());
				}

			} else if (createIfNotThere)
			{
				c = ControllableFactory::getInstance()->createControllable(o->getProperty("type"));
				if (c != nullptr)
				{
					c->saveValueOnly = false; //auto set here because it will likely need that if it has been created from data
					c->loadJSONData(pData);
					addControllable(c);
				}
			}
		}
	}

	if (saveAndLoadRecursiveData && data.hasProperty("containers"))
	{
		NamedValueSet ccData = data.getProperty("containers",var()).getDynamicObject()->getProperties();
		for (auto &nv : ccData)
		{
			ControllableContainer * cc = getControllableContainerByName(nv.name.toString());
			if (cc == nullptr && createIfNotThere)
			{
				bool owned = nv.value.getProperty("owned", false);
				cc = new ControllableContainer(nv.value.getProperty("niceName", nv.name.toString()));
				addChildControllableContainer(cc, owned);
			}

			if (cc != nullptr) cc->loadJSONData(nv.value, true);
		}
	}

	loadJSONDataInternal(data);

	isCurrentlyLoadingData = false;
	controllableContainerListeners.call(&ControllableContainerListener::controllableContainerFinishedLoading, this);
	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerFinishedLoading, this));
}

void ControllableContainer::childStructureChanged(ControllableContainer * cc)
{
	notifyStructureChanged();
}

void ControllableContainer::childAddressChanged(ControllableContainer * cc)
{
	notifyStructureChanged();
}


String ControllableContainer::getUniqueNameInContainer(const String & sourceName, int suffix)
{
	String resultName = sourceName;
	if (suffix > 0)
	{
		StringArray sa;
		sa.addTokens(resultName, false);
		if (sa.size() > 1 && (sa[sa.size() - 1].getIntValue() != 0 || sa[sa.size() - 1].containsOnly("0")))
		{
			int num = sa[sa.size() - 1].getIntValue() + suffix;
			sa.remove(sa.size() - 1);
			sa.add(String(num));
			resultName = sa.joinIntoString(" ");
		} else
		{
			resultName += " " + String(suffix);
		}
	}

	if (getControllableByName(resultName, true) != nullptr)
	{
		return getUniqueNameInContainer(sourceName, suffix + 1);
	}

	if (getControllableContainerByName(resultName, true) != nullptr)
	{
		return getUniqueNameInContainer(sourceName, suffix + 1);
	}

	return resultName;
}

void ControllableContainer::updateLiveScriptObjectInternal(DynamicObject * parent) 
{	
	ScriptTarget::updateLiveScriptObjectInternal(parent);
	

	bool transferToParent = parent != nullptr;

	for (auto &cc : controllableContainers)
	{
		if (cc == nullptr || cc.wasObjectDeleted()) continue;

		if (!cc->includeInScriptObject) continue;
		/*if (cc->skipControllableNameInAddress)
		{
			cc->updateLiveScriptObject(transferToParent?parent:(DynamicObject *)liveScriptObject);
		}else
		{*/
			if (transferToParent) parent->setProperty(cc->shortName, cc->getScriptObject());
			else liveScriptObject->setProperty(cc->shortName, cc->getScriptObject());
		//}

	}

	for (auto &c : controllables)
	{
		if (!c->includeInScriptObject) continue;
		if(transferToParent) parent->setProperty(c->shortName,c->getScriptObject());
		else liveScriptObject->setProperty(c->shortName, c->getScriptObject());
	}
	
	/*if (!(skipControllableNameInAddress && parent != nullptr))
	{*/
		liveScriptObject->setProperty("name", shortName);
		liveScriptObject->setProperty("niceName", niceName);
	//}

	
}

var ControllableContainer::getChildFromScript(const var::NativeFunctionArgs & a)
{
	if (a.numArguments == 0) return var();
	ControllableContainer * m = getObjectFromJS<ControllableContainer>(a);
	if (m == nullptr) return var();
	String nameToFind = a.arguments[0].toString();
	ControllableContainer * cc = m->getControllableContainerByName(nameToFind);
	if (cc != nullptr) return cc->getScriptObject();

	Controllable * c = m->getControllableByName(nameToFind);
	if (c != nullptr) return c->getScriptObject();

	LOG("Child not found from script " + a.arguments[0].toString());
	return var();
}

var ControllableContainer::getParentFromScript(const juce::var::NativeFunctionArgs & a)
{
	ControllableContainer * m = getObjectFromJS<ControllableContainer>(a);
	if (m->parentContainer == nullptr) return var();
	return m->parentContainer->getScriptObject();
}

var ControllableContainer::setNameFromScript(const juce::var::NativeFunctionArgs& a)
{
	if (a.numArguments == 0) return var();
	ControllableContainer * cc = getObjectFromJS<ControllableContainer>(a);
	cc->setNiceName(a.arguments[0].toString());
	if (a.numArguments >= 2) cc->setCustomShortName(a.arguments[1].toString());
	else cc->setAutoShortName();

	return var(); 
}


InspectableEditor * ControllableContainer::getEditor(bool isRoot)
{
	if (customGetEditorFunc != nullptr) return customGetEditorFunc(this, isRoot);
	return new GenericControllableContainerEditor(this, isRoot);
}

DashboardItem * ControllableContainer::createDashboardItem()
{
	return new DashboardCCItem(this);
}

EnablingControllableContainer::EnablingControllableContainer(const String & n, bool _canBeDisabled) :
	ControllableContainer(n),
	enabled(nullptr),
	canBeDisabled(false) 
{
	setCanBeDisabled(_canBeDisabled);
	
}

void EnablingControllableContainer::setCanBeDisabled(bool value)
{
	if (canBeDisabled == value) return;

	canBeDisabled = value;

	if (canBeDisabled)
	{
		enabled = addBoolParameter("Enabled", "Activate OSC Input for this module", true);
		enabled->hideInEditor = true;
	} else
	{
		removeControllable(enabled);
		enabled = nullptr;
	}
}

InspectableEditor * EnablingControllableContainer::getEditor(bool isRoot)
{
	if (customGetEditorFunc != nullptr) return customGetEditorFunc(this, isRoot);
	return new EnablingControllableContainerEditor(this, isRoot);
}

ControllableContainer * ControllableContainer::ControllableContainerAction::getControllableContainer()
{
	if (containerRef != nullptr && !containerRef.wasObjectDeleted()) return containerRef.get();
	else if(Engine::mainEngine != nullptr)
	{
		ControllableContainer * cc = Engine::mainEngine->getControllableContainerForAddress(controlAddress, true);
		return cc;
	}

	return nullptr;
}


bool ControllableContainer::ControllableContainerChangeNameAction::perform()
{
	ControllableContainer * cc = getControllableContainer();
	if (cc != nullptr)
	{
		cc->setNiceName(newName);
		return true;
	}
	return false;
}

bool ControllableContainer::ControllableContainerChangeNameAction::undo()
{
	ControllableContainer * cc = getControllableContainer();
	if (cc != nullptr)
	{
		cc->setNiceName(oldName);
		return true;
	}
	return false;
}

ControllableContainer::ControllableContainerControllableAction::ControllableContainerControllableAction(ControllableContainer * cc, Controllable * c) :
	ControllableContainerAction(cc),
	cRef(c)
{
	if (c != nullptr)
	{
		cShortName = c->shortName;
		data = c->getJSONData();
		cType = c->getTypeString();
	}
}

Controllable * ControllableContainer::ControllableContainerControllableAction::getItem()
{
	if (cRef != nullptr && !cRef.wasObjectDeleted()) return dynamic_cast<Controllable *>(cRef.get());
	else
	{
		ControllableContainer * cc = this->getControllableContainer();
		if (cc != nullptr) return cc->getControllableByName(cShortName);
	}

	return nullptr;
}

bool ControllableContainer::AddControllableAction::perform()
{
	ControllableContainer * cc = this->getControllableContainer();
	if (cc == nullptr)
	{
		return false;
	}

	Controllable * c = this->getItem();
	if (c != nullptr)
	{
		cc->addControllable(c);
	} else
	{
		c = ControllableFactory::createControllable(cType);
	}

	if (c == nullptr) return false;

	this->cShortName = c->shortName;
	return true;
}

bool ControllableContainer::AddControllableAction::undo()
{
	Controllable * c = this->getItem();
	if (c == nullptr) return false;
	data = c->getJSONData();
	ControllableContainer * cc = getControllableContainer();
	if (cc != nullptr)
	{
		cc->removeControllable(c);
		cRef = nullptr;
	}
	return true;
}

ControllableContainer::RemoveControllableAction::RemoveControllableAction(ControllableContainer * cc, Controllable * c) :
	ControllableContainerControllableAction(cc, c)
{
}

bool ControllableContainer::RemoveControllableAction::perform()
{
	Controllable * c = this->getItem();

	if (c == nullptr) return false;
	getControllableContainer()->removeControllable(c);
	cRef = nullptr;
	return true;
}

bool ControllableContainer::RemoveControllableAction::undo()
{
	ControllableContainer * cc = getControllableContainer();
	if (cc == nullptr) return false;
	Controllable * c = ControllableFactory::createControllable(cType);
	if (c != nullptr)
	{
		c->loadJSONData(data); 
		cc->addControllable(c);
		cRef = c;
	}
	return true;
}
