#pragma once


class DashboardControllableItemUI :
	public DashboardInspectableItemUI
{
public:
	DashboardControllableItemUI(DashboardControllableItem * controllableItem);
	~DashboardControllableItemUI();

	DashboardControllableItem* controllableItem;
	std::unique_ptr<ControllableUI> itemUI;

	virtual void paint(Graphics& g) override;
	virtual void resizedDashboardItemInternal() override;

	virtual ControllableUI * createControllableUI();
	virtual void rebuildUI();

	virtual void updateUIParameters();

	virtual void updateEditModeInternal(bool editMode) override;

	virtual void inspectableChanged() override;

	virtual void controllableFeedbackUpdateInternal(Controllable* c) override;
};