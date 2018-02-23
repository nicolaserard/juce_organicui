#include "TargetParameter.h"
/*
  ==============================================================================

    TargetParameter.cpp
    Created: 2 Nov 2016 5:00:04pm
    Author:  bkupe

  ==============================================================================
*/


TargetParameter::TargetParameter(const String & niceName, const String & description, const String & initialValue, WeakReference<ControllableContainer> rootReference, bool enabled) :
	StringParameter(niceName, description, initialValue, enabled),
	targetType(CONTROLLABLE),
	useGhosting(true),
	showParameters(true),
	showTriggers(true),
	showFullAddressInEditor(false),
	showParentNameInEditor(true),
	maxDefaultSearchLevel(-1),
	defaultParentLabelLevel(1),
	rootContainer(rootReference),
	target(nullptr),
	targetContainer(nullptr),
	customGetTargetFunc(nullptr),
	customGetControllableLabelFunc(nullptr),
	customCheckAssignOnNextChangeFunc(nullptr),
	defaultContainerTypeCheckFunc(nullptr),
	customGetTargetContainerFunc(nullptr),
	customGetContainerLabelFunc(nullptr)

{
	lockManualControlMode = true;
	type = TARGET; 
	if (rootContainer == nullptr) rootContainer = Engine::mainEngine;
	
	argumentsDescription = "target";
}

TargetParameter::~TargetParameter()
{
	if (rootContainer != nullptr) rootContainer->removeControllableContainerListener(this);
	setTarget((ControllableContainer *)nullptr);
	setTarget((Controllable *)nullptr);
	setValue("");
}

void TargetParameter::setGhostValue(const String & ghostVal)
{
	if (ghostVal == ghostValue) return;

	ghostValue = ghostVal;
	if (ghostValue.isNotEmpty())
	{
		//if (Engine::mainEngine->isLoadingFile) Engine::mainEngine->addEngineListener(this);
		if(rootContainer != nullptr) rootContainer->addControllableContainerListener(this);
	} else
	{
		if(!rootContainer.wasObjectDeleted()) rootContainer->removeControllableContainerListener(this);
	}
}

void TargetParameter::setValueFromTarget(Controllable * c)
{
	if (c == target) return;
	setValue(c->getControlAddress(rootContainer),false,true);
}

void TargetParameter::setValueFromTarget(ControllableContainer * cc)
{
	if (cc == targetContainer) return; 
	setValue(cc->getControlAddress(rootContainer),false,true);
}

void TargetParameter::setValueInternal(var & newVal)
{
	StringParameter::setValueInternal(newVal);

	if (newVal.toString().isNotEmpty())
	{
		if (targetType == CONTAINER)
		{
			WeakReference<ControllableContainer> cc = rootContainer->getControllableContainerForAddress(newVal.toString());
			if (cc != nullptr) setTarget(cc);
			else setGhostValue(newVal.toString());
		} else
		{
			WeakReference<Controllable> c = rootContainer->getControllableForAddress(newVal.toString());
			if (c != nullptr) setTarget(c);
			else setGhostValue(newVal.toString());
		}
	} else
	{
		if(targetType == CONTAINER) setTarget((ControllableContainer *)nullptr);
		else setTarget((ControllableContainer *)nullptr);

		setGhostValue(String::empty);
	}
}

void TargetParameter::setTarget(WeakReference<Controllable> c)
{
	if (target != nullptr)
	{
		if (!target.wasObjectDeleted()) target->removeControllableListener(this);
	}
	
	target = c;

	if (target != nullptr)
	{
		target->addControllableListener(this);
		setGhostValue(String::empty);
	}
}

void TargetParameter::setTarget(WeakReference<ControllableContainer> cc)
{
	if (targetContainer != nullptr)
	{
		if (!targetContainer.wasObjectDeleted()) targetContainer->removeControllableContainerListener(this);
	}

	targetContainer = cc;

	if (targetContainer != nullptr)
	{
		targetContainer->addControllableContainerListener(this);
		setGhostValue(String::empty);
	}
}

void TargetParameter::childStructureChanged(ControllableContainer * cc)
{
	
	if (target == nullptr && ghostValue.isNotEmpty())
	{
		WeakReference<Controllable> c = rootContainer->getControllableForAddress(ghostValue);
		if (c != nullptr) setValueFromTarget(c);
		
	}
}

void TargetParameter::controllableRemoved(Controllable * c)
{
	if (c == target || target.wasObjectDeleted())
	{
		String oldValue = stringValue();
		setValue(String::empty);
		setGhostValue(oldValue);
	}
}

void TargetParameter::controllableContainerRemoved(ControllableContainer * cc)
{
	if (cc == targetContainer || targetContainer.wasObjectDeleted())
	{
		String oldValue = stringValue();
		setValue(String::empty);
		setGhostValue(oldValue);
	}
}

void TargetParameter::endLoadFile()
{
	//DBG("End load file, ghost is " << ghostValue << ", try retargeting");
	/*
	Engine::mainEngine->removeEngineListener(this);
	if (target == nullptr && ghostValue.isNotEmpty())
	{
		WeakReference<Controllable> c = rootContainer->getControllableForAddress(ghostValue);
		if (c != nullptr) setValueFromTarget(c);
	}
	*/
}

TargetParameterUI * TargetParameter::createTargetUI(TargetParameter * _target)
{
	return new TargetParameterUI(_target == nullptr ?this:_target);
}

ControllableUI * TargetParameter::createDefaultUI(Controllable * targetControllable)
{
	return createTargetUI((TargetParameter*)targetControllable);
}
