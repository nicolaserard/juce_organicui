/*
  ==============================================================================

   

  ==============================================================================
*/

#ifdef ORGANIC_UI_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "juce_organicui.h"

//==============================================================================


#include "manager/Factory.cpp"
#include "resources/OrganicBinaryData.cpp"
#include "script/ScriptExpression.cpp"
#include "controllable/Controllable.cpp"
#include "controllable/ControllableContainer.cpp"
#include "controllable/ControllableFactory.cpp"
#include "controllable/ControllableHelpers.cpp"
#include "controllable/parameter/BoolParameter.cpp"
#include "controllable/parameter/ColorParameter.cpp"
#include "controllable/parameter/EnumParameter.cpp"
#include "controllable/parameter/FloatParameter.cpp"
#include "controllable/parameter/FloatRangeParameter.cpp"
#include "controllable/parameter/IntParameter.cpp"
#include "controllable/parameter/IntRangeParameter.cpp"
#include "controllable/parameter/Parameter.cpp"
#include "controllable/parameter/Point2DParameter.cpp"
#include "controllable/parameter/Point3DParameter.cpp"
#include "controllable/parameter/StringParameter.cpp"
#include "controllable/parameter/FileParameter.cpp"
#include "controllable/parameter/TargetParameter.cpp"
#include "controllable/parameter/ui/BetterStepper.cpp"
#include "controllable/parameter/ui/BoolImageToggleUI.cpp"
#include "controllable/parameter/ui/BoolToggleUI.cpp"
#include "controllable/parameter/ui/ColorParameterUI.cpp"
#include "controllable/parameter/ui/DoubleSliderUI.cpp"
#include "controllable/parameter/ui/EnumParameterUI.cpp"
#include "controllable/parameter/ui/FloatParameterLabelUI.cpp"
#include "controllable/parameter/ui/IntParameterLabelUI.cpp"
#include "controllable/parameter/ui/FloatSliderUI.cpp"
#include "controllable/parameter/ui/FloatStepperUI.cpp"
#include "controllable/parameter/ui/IntSliderUI.cpp"
#include "controllable/parameter/ui/IntStepperUI.cpp"
#include "controllable/parameter/ui/ParameterUI.cpp"
#include "controllable/parameter/ui/ParameterEditor.cpp"
#include "controllable/parameter/ui/StringParameterUI.cpp"
#include "controllable/parameter/ui/TargetParameterUI.cpp"
#include "controllable/parameter/ui/TripleSliderUI.cpp"
#include "controllable/Trigger.cpp"
#include "controllable/ui/ControllableEditor.cpp"
#include "controllable/ui/ControllableUI.cpp"
#include "controllable/ui/GenericControllableContainerEditor.cpp"
#include "controllable/ui/TriggerBlinkUI.cpp"
#include "controllable/ui/TriggerButtonUI.cpp"
#include "controllable/ui/TriggerImageUI.cpp"
#include "controllable/ui/TriggerUI.cpp"

#include "automation/recorder/AutomationRecorder.cpp"
#include "automation/easing/Easing.cpp"
#include "automation/AutomationKey.cpp"
#include "automation/Automation.cpp"
#include "automation/parameter/ParameterAutomation.cpp"
#include "automation/parameter/ParameterAutomationManager.cpp"
#include "automation/easing/ui/EasingUI.cpp"
#include "automation/ui/AutomationKeyUI.cpp"
#include "automation/ui/AutomationMultiKeyTransformer.cpp"
#include "automation/ui/AutomationUI.cpp"
#include "automation/ui/AutomationEditor.cpp"
#include "automation/parameter/ui/PlayableParameterAutomationEditor.cpp"

#include "dashboard/Dashboard.cpp"
#include "dashboard/DashboardItem.cpp"
#include "dashboard/DashboardItemManager.cpp"
#include "dashboard/DashboardManager.cpp"
#include "dashboard/DashboardPanel.cpp"
#include "dashboard/DashboardPanelManager.cpp"
#include "dashboard/DashboardScriptItem.cpp"
#include "dashboard/ui/DashboardItemManagerUI.cpp"
#include "dashboard/ui/DashboardItemUI.cpp"
#include "dashboard/ui/DashboardManagerUI.cpp"
#include "dashboard/ui/DashboardManagerView.cpp"
#include "dashboard/ui/DashboardPanelUI.cpp"
#include "dashboard/ui/DashboardScriptItemUI.cpp"
#include "dashboard/ui/DashboardUI.cpp"
#include "dashboard/ui/DashboardView.cpp"
#include "engine/Engine.cpp"
#include "engine/EngineFileDocument.cpp"
#include "settings/GlobalSettings.cpp"
#include "settings/ProjectSettings.cpp"
#include "inspectable/Inspectable.cpp"
#include "inspectable/InspectableContent.cpp"
#include "inspectable/InspectableSelectionManager.cpp"
#include "help/HelpBox.cpp"
#include "inspectable/ui/InspectableContentComponent.cpp"
#include "inspectable/ui/InspectableEditor.cpp"
#include "inspectable/ui/InspectableSelector.cpp"
#include "inspectable/ui/Inspector.cpp"
#include "logger/CustomLogger.cpp"
#include "logger/ui/CustomLoggerUI.cpp"
#include "manager/Factory.h"
#include "manager/BaseItem.cpp"
#include "manager/GenericControllableManager.cpp"
#include "manager/GenericControllableItem.cpp"
#include "manager/ui/GenericControllableItemUI.cpp" 
#include "manager/ui/BaseItemEditor.cpp"
#include "outliner/Outliner.cpp"
#include "progress/ProgressNotifier.cpp"
#include "progress/ui/ProgressWindow.cpp"
#include "script/ScriptTarget.cpp"
#include "script/Script.cpp"
#include "script/ScriptManager.cpp"
#include "script/ScriptUtil.cpp"
#include "script/ui/ScriptEditor.cpp"
#include "ui/AssetManager.cpp"
#include "ui/LookAndFeelOO.cpp"
#include "ui/shapeshifter/GapGrabber.cpp"
#include "ui/shapeshifter/ShapeShifter.cpp"
#include "ui/shapeshifter/ShapeShifterContainer.cpp"
#include "ui/shapeshifter/ShapeShifterContent.cpp"
#include "ui/shapeshifter/ShapeShifterFactory.cpp"
#include "ui/shapeshifter/ShapeShifterManager.cpp"
#include "ui/shapeshifter/ShapeShifterPanel.cpp"
#include "ui/shapeshifter/ShapeShifterPanelHeader.cpp"
#include "ui/shapeshifter/ShapeShifterPanelTab.cpp"
#include "ui/shapeshifter/ShapeShifterWindow.cpp"
#include "help/HelpPanel.cpp"
#include "undo/UndoMaster.cpp"
#include "updater/AppUpdater.cpp"


#include "app/CrashHandler.cpp"
#include "app/OrganicMainComponent.cpp"
#include "app/OrganicMainComponentCommands.cpp"
#include "app/OrganicApplication.cpp"

#include "remotecontrol/OSCRemoteControl.cpp"