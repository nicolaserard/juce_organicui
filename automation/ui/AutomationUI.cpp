/*
  ==============================================================================

    AutomationUI.cpp
    Created: 21 Mar 2020 4:06:30pm
    Author:  bkupe

  ==============================================================================
*/

AutomationUI::AutomationUI(Automation* manager) :
    BaseManagerUI(manager->niceName, manager, false),
    autoAdaptViewRange(false),
    paintingMode(false),
    previewMode(false),
    showNumberLines(true)
{
    //autoSelectWithChildRespect = false;
    resizeOnChildBoundsChanged = false;
    setShowAddButton(false);

    animateItemOnAdd = false;
    manager->addAsyncContainerListener(this);
    manager->addAsyncAutomationListener(this);

    transparentBG = true;

    setViewRange(0, manager->length->floatValue());

    startTimerHz(30);

    addExistingItems(false);
    setSize(100, 100);
}

AutomationUI::~AutomationUI()
{
    if (!inspectable.wasObjectDeleted())
    {
        manager->removeAsyncContainerListener(this);
        manager->removeAsyncAutomationListener(this);
    }

    for (auto& ui : itemsUI)
    {
        if (ui != nullptr && !ui->inspectable.wasObjectDeleted())
        {
            ui->item->removeAsyncKeyListener(this);
            ui->removeKeyUIListener(this);
        }
    }
}

void AutomationUI::paint(Graphics& g)
{
    if (getWidth() == 0 || !isShowing()) return;

    if (!transparentBG) g.fillAll(bgColor);

    if (previewMode)
    {
        Path p;
        p.startNewSubPath(Point<float>(0, getYForValue(manager->getValueAtPosition(getPosForX(0)))));
        for (int i = 1; i < getWidth(); i+=2)
        {
            p.lineTo(Point<float>(i, getYForValue(manager->getValueAtPosition(getPosForX(i)))));
        }

        g.setColour(NORMAL_COLOR);
        g.strokePath(p, PathStrokeType(1));
        return;
    }

    if(showNumberLines) drawLinesBackground(g);
    
    g.setColour(Colours::white.withAlpha(.04f));
    float ty = getYForValue(manager->value->floatValue());
    float t0 = getYForValue(0);
    g.fillRect(getLocalBounds().withTop(jmin(ty,t0)).withBottom(jmax(ty,t0)));

    if (manager->items.size() > 0)
    {
        if (manager->items[0] != nullptr && manager->items[0]->position->floatValue() > 0)
        {
            g.setColour(NORMAL_COLOR);
            Point<int> p = getPosInView(manager->items[0]->getPosAndValue());
            const float dashLengths[] = { 5, 5 };
            g.drawDashedLine(Line<int>(Point<int>(0, p.y), p).toFloat(), dashLengths, 2);
        }

        if (manager->items[manager->items.size() - 1] != nullptr && manager->items[manager->items.size()-1]->position->floatValue() < manager->length->floatValue())
        {
            g.setColour(NORMAL_COLOR);
            Point<int> p = getPosInView(manager->items[manager->items.size()-1]->getPosAndValue());
            const float dashLengths[] = { 5, 5 };
            g.drawDashedLine(Line<int>(p, Point<int>(getWidth(), p.y)).toFloat(), dashLengths, 2);
        }
    }
}

void AutomationUI::drawLinesBackground(Graphics& g)
{
    float start = manager->viewValueRange->x;
    float end = manager->viewValueRange->y;

    if (end <= start) return;

    int minGap = 10;
    int fadeGap = 30;
    const int numDecUnitPerUnit = 10;

    float decimalGap = (getHeight() / (end - start)) / numDecUnitPerUnit;
    float unitGap = (getHeight() / (end - start));

    bool showSeconds = unitGap > minGap;

    float decimalSteps = 1;
    float unitSteps = 1;

    if (showSeconds)
    {
        while (decimalGap < minGap)
        {
            if (decimalSteps == 1) decimalSteps *= 2.5f;
            else decimalSteps *= 2;
            decimalGap = (getHeight() / (end - start)) * decimalSteps / numDecUnitPerUnit;
        }
    }

    while (unitGap < minGap)
    {
        unitSteps *= 2;
        unitGap = ((getHeight() / (end - start))) * unitSteps;
    }

    int unitStart = floor((start / unitSteps)) * unitSteps;
    int unitEndTime = jmax<int>(ceil((end / unitSteps)) * unitSteps, unitStart + 1);

    g.setFont(12);
    float unitFadeAlpha = jlimit<float>(0, 1, jmap<float>(unitGap, minGap, fadeGap, 0, 1));
    float fadeAlpha = jlimit<float>(0, 1, jmap<float>(decimalGap, minGap, fadeGap, 0, 1));

    int uIndex = 0;
    for (float i = unitStart; i <= unitEndTime; i += unitSteps)
    {
        int mty = getYForValue(i);

        if (mty >= 0 && mty <= getHeight())
        {
            //Draw minute
            float alpha = 1;
            if (uIndex % 2 == 0) alpha = unitFadeAlpha;

            g.setColour(BG_COLOR.brighter(.3f).withAlpha(alpha * .4f));
            //g.drawLine(tx, 0, tx, getHeight(), 1);
            g.drawHorizontalLine(mty, 30, (float)getWidth());
            //g.setColour(BG_COLOR.darker(.6f));
            //g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 2);

            //g.setColour(BG_COLOR.brighter(.7f));
            //g.fillRoundedRectangle( 0, mty - 10, 20, 14, 2);
            g.setColour(BG_COLOR.brighter(.2f).withAlpha(alpha * .5f));
            g.drawText(String(i, 2), 2, mty - 10, 35, 14, Justification::left);

        }
    
        uIndex++;

        if (showSeconds)
        {
            int sIndex = 0;
            for (float s = decimalSteps; s < numDecUnitPerUnit && i + s/numDecUnitPerUnit <= end; s += decimalSteps)
            {
                int sty = getYForValue(i + s*1.0f / numDecUnitPerUnit);
                if (sty >= 0 && sty <= getHeight())
                {
                    float alpha = 1;
                    if (sIndex % 2 == 0) alpha = fadeAlpha;
                    g.setColour(BG_COLOR.brighter(.2f).withAlpha(alpha * .2f));
                    //g.drawLine(tx, 0, tx, getHeight(), 1);
                    g.drawHorizontalLine(sty, 30, (float)getWidth());
                    g.setColour(BG_COLOR.brighter(.2f).withAlpha(alpha));
                    g.drawText(String(s / numDecUnitPerUnit, 2), 2, sty - 10, 35, 14, Justification::left);

                }

                sIndex++;

            }
        }
    }
}

void AutomationUI::paintOverChildren(Graphics& g)
{
    if (previewMode || !isShowing()) return;

    //recorder
    
    bool interactiveMode = !manager->interactiveSimplifiedPoints.isEmpty();

    if (manager->recorder != nullptr)
    {
        if (manager->recorder->isRecording->boolValue())
        {
            int numRKeys = manager->recorder->keys.size();
            if (numRKeys > 0)
            {
                if (!interactiveMode)
                {
                    g.setColour(Colours::red.withAlpha(.2f));
                    g.fillRect(getLocalBounds().withLeft(getXForPos(manager->recorder->keys[0].time)).withRight(getXForPos(manager->position->floatValue())));
                }
                
                if (numRKeys >= 2)
                {
                    Path p;
                    Point<float> k0(manager->recorder->keys[0].time, manager->recorder->keys[0].value);
                    p.startNewSubPath(getPosInView(k0).toFloat());
                    for (int i = 1; i < numRKeys; ++i)
                    {
                        Point<float> ki(manager->recorder->keys[i].time, manager->recorder->keys[i].value);
                        p.lineTo(getPosInView(ki).toFloat());
                    }
                    
                    g.setColour(Colours::orangered);
                    g.strokePath(p, PathStrokeType(interactiveMode ? 1 : 2));
                }
            }
        }
    }

    if (interactiveMode)
    {
        if (manager->interactiveSimplifiedPoints.size() >= 2)
        {
            Path p;
            Point<float> k0(manager->interactiveSimplifiedPoints[0].x, manager->interactiveSimplifiedPoints[0].y);
            p.startNewSubPath(getPosInView(k0).toFloat());
            for (int i = 1; i < manager->interactiveSimplifiedPoints.size(); ++i)
            {
                Point<float> ki(manager->interactiveSimplifiedPoints[i].x, manager->interactiveSimplifiedPoints[i].y);
                p.lineTo(getPosInView(ki).toFloat());
            }

            g.setColour(GREEN_COLOR);
            g.strokePath(p, PathStrokeType(2));
        }
    }

    if (paintingMode && paintingPoints.size() > 0)
    {
        g.setColour(YELLOW_COLOR);
        
        Path p; 
        p.startNewSubPath(getPosInView(paintingPoints[0]).toFloat());
        for (auto& pp : paintingPoints)
        {
            Point<int> vpp = getPosInView(pp);
            g.fillEllipse(Rectangle<int>(0, 0, 2,2).withCentre(vpp).toFloat());
            p.lineTo(vpp.toFloat());
        }

        g.setColour(YELLOW_COLOR.withAlpha(.5f));
        g.strokePath(p, PathStrokeType(1));
    }
}


void AutomationUI::resized()
{
    if (previewMode || !isVisible()) return;
    for (auto& kui : itemsUI) placeKeyUI(kui);

    if (interactiveSimplificationUI != nullptr)
    {
        Rectangle<int> r = getLocalBounds().removeFromTop(24).withSizeKeepingCentre(300, 24).reduced(2);
        validInteractiveBT->setBounds(r.removeFromRight(80));
        r.removeFromRight(8);
        interactiveSimplificationUI->setBounds(r);
    }
}

void AutomationUI::placeKeyUI(AutomationKeyUI* ui)
{
    if (ui == nullptr || !ui->isVisible()) return;

    Rectangle<int> r = getLocalBounds();

    Point<int> p = getPosInView(ui->item->getPosAndValue());
    Rectangle<int> pr = Rectangle<int>(0, 0, 20, 20).withCentre(p);
    if (ui->item->easing != nullptr)
    {
        Rectangle<int> er = getBoundsInView(ui->item->easing->getBounds(true));
        er.setSize(jmax(er.getWidth(), er.getWidth() + 1), jmax(er.getHeight(), er.getHeight() + 1));
        pr = pr.getUnion(er);
    }

    pr = pr.expanded(5, 5).getIntersection(r);
    ui->setBounds(pr);
    ui->setValueBounds(getViewBounds(pr));

}

void AutomationUI::updateHandlesForUI(AutomationKeyUI* ui, bool checkSideItems)
{
    if (ui == nullptr) return;

    int index = itemsUI.indexOf(ui);
    if (checkSideItems)
    {
        if (index > 0)  updateHandlesForUI(itemsUI[index - 1], false);
        if (index < itemsUI.size() - 1)  updateHandlesForUI(itemsUI[index + 1], false);
    }

    bool curSelected = ui->item->isThisOrChildSelected();
    if (curSelected)
    {
        ui->setShowEasingHandles(true, !ui->item->isSelected);
        return;
    }

    bool prevSelected = false;
    if (index > 0 && itemsUI[index - 1] != nullptr)
    {
        AutomationKey* prevItem = itemsUI[index - 1]->item;
        prevSelected = prevItem->isThisOrChildSelected() && !prevItem->isSelected; //we only want to show if easing is selected only easing
    }
    bool nextSelected = index < itemsUI.size() && itemsUI[index + 1] != nullptr && itemsUI[index + 1]->item->isThisOrChildSelected();

    ui->setShowEasingHandles(prevSelected, nextSelected);

}

void AutomationUI::setPreviewMode(bool value)
{
    previewMode = value;
    if (previewMode)
    {
        for (auto& kui : itemsUI) kui->setVisible(false);
    }
    else
    {
        updateItemsVisibility();
    }

    resized();
    repaint();
}

void AutomationUI::setViewRange(float start, float end)
{
    viewPosRange.setXY(start, end);
    viewLength = viewPosRange.y - viewPosRange.x;

    resized();
    shouldRepaint = true;
}

void AutomationUI::updateItemsVisibility()
{
    if (itemsUI.size() == 0) return;
    if (previewMode) return;

    int firstIndex = jmax(manager->items.indexOf(manager->getKeyForPosition(viewPosRange.x)), 0);
    int lastIndex = jmax(manager->items.indexOf(manager->getKeyForPosition(viewPosRange.y))+1, firstIndex);
   
    for (int i=0;i<itemsUI.size();++i)
    {
        itemsUI[i]->setVisible(i >= firstIndex && i <= lastIndex);
    }
}

void AutomationUI::addItemUIInternal(AutomationKeyUI* ui)
{
    ui->addMouseListener(this, true);
    ui->item->addAsyncKeyListener(this);
    ui->addKeyUIListener(this);
}

void AutomationUI::removeItemUIInternal(AutomationKeyUI* ui)
{
    ui->removeMouseListener(this);
    if (!ui->inspectable.wasObjectDeleted())
    {
        ui->item->removeAsyncKeyListener(this);
        ui->removeKeyUIListener(this);
    }
}

void AutomationUI::mouseDown(const MouseEvent& e)
{
    if (e.eventComponent == this)
    {
        if (e.mods.isLeftButtonDown() && e.mods.isCommandDown() && e.mods.isShiftDown())
        {
            paintingMode = true;
            paintingPoints.clear();
            paintingPoints.add(getViewPos(e.getPosition()));
            lastPaintingPoint = paintingPoints[0];
        }
        else if (e.mods.isAltDown())
        {
            viewValueRangeAtMouseDown = manager->viewValueRange->getPoint();
        }else
        {
            BaseManagerUI::mouseDown(e);
        }
    }
}

void AutomationUI::mouseDrag(const MouseEvent& e)
{    
    if (AutomationKeyHandle* handle = dynamic_cast<AutomationKeyHandle*>(e.eventComponent))
    {
        AutomationKey* k = handle->key;
        int index = manager->items.indexOf(k);

        Point<float> offset = getViewPos(e.getEventRelativeTo(this).getOffsetFromDragStart(),true);
        offset.setY(-offset.y);


        if (k->nextKey != nullptr) offset.setX(jmin(offset.x, k->nextKey->position->floatValue() - k->movePositionReference.x));
        if (index > 0) offset.setX(jmax(offset.x, manager->items[index - 1]->position->floatValue() - k->movePositionReference.x));
        

        if (e.mods.isShiftDown()) offset.setY(0);
        if (e.mods.isAltDown()) k->scalePosition(offset, true);
        else k->movePosition(offset, true);
    }
    else if (e.eventComponent == this)
    {
        if (paintingMode)
        {
            Point<float> newPoint = getViewPos(e.getPosition());
            
            float minX = jmin(newPoint.x, lastPaintingPoint.x);
            float maxX = jmax(newPoint.x, lastPaintingPoint.x);

            int inKeyStart = -1;
            int inKeyEnd = -1;
            int indexBeforeNewPoint = 0;
            int indexAfterNewPoint = paintingPoints.size();

            for (int i = 0; i < paintingPoints.size(); ++i)
            {
                Point<float> p = paintingPoints[i];
                if (p.x >= newPoint.x) indexAfterNewPoint = jmin(indexAfterNewPoint, i);
                else indexBeforeNewPoint = jmax(indexBeforeNewPoint, i);

                if (p.x == lastPaintingPoint.x) continue;

                if (p.x >= minX && p.x <= maxX)
                {
                    inKeyStart = inKeyStart == -1 ? i : jmin(i, inKeyStart);
                    inKeyEnd = jmax(inKeyEnd, i);
                }
            }

            bool foundKeysToRemove = inKeyStart != -1 && inKeyEnd != -1;
            if (foundKeysToRemove)
            {
                paintingPoints.removeRange(inKeyStart, inKeyEnd - inKeyStart+1);
            }
            
            paintingPoints.insert(indexAfterNewPoint, newPoint);
            lastPaintingPoint.setXY(newPoint.x, newPoint.y);
            repaint();
        }
        else if (e.mods.isAltDown())
        {
            if (!manager->valueRange->enabled)
            {
                if (e.mods.isShiftDown())
                {
                    float pRel = 1 - (e.getMouseDownPosition().y * 1.0f / getHeight());
                    float rangeAtMouseDown = (viewValueRangeAtMouseDown.y - viewValueRangeAtMouseDown.x);
                    float tRange = rangeAtMouseDown + (e.getOffsetFromDragStart().y / 10.0f);
                    if (tRange > .2f)
                    {
                        float rangeDiff = tRange - rangeAtMouseDown;
                        float tMin = viewValueRangeAtMouseDown.x - rangeDiff * pRel;
                        float tMax = viewValueRangeAtMouseDown.y + rangeDiff * (1 - pRel);
                        manager->viewValueRange->setPoint(tMin, tMax);
                    }
                }
                else
                {
                    float deltaVal = getValueForY(e.getDistanceFromDragStartY(), true);
                    manager->viewValueRange->setPoint(viewValueRangeAtMouseDown.x + deltaVal, viewValueRangeAtMouseDown.y + deltaVal);
                }
            }
        }
        {
            BaseManagerUI::mouseDrag(e);
        }
    }
}

void AutomationUI::mouseUp(const MouseEvent& e)
{
    if (paintingMode)
    {
        manager->addFromPointsAndSimplifyBezier(paintingPoints); //always use bezier for drawing
        paintingMode = false;
        paintingPoints.clear();
        repaint();
    }
    else
    {
        BaseManagerUI::mouseUp(e);
    }
}

void AutomationUI::mouseDoubleClick(const MouseEvent& e)
{
    if (e.eventComponent == this)
    {
        Point<float> p = getViewPos(e.getPosition());
        manager->addKey(p.x, p.y, true);
    }
    else if (EasingUI * eui = dynamic_cast<EasingUI*>(e.eventComponent))
    {
        AutomationKey* startKey = ((AutomationKeyUI*)eui->getParentComponent())->item;
        Point<float> p = eui->easing->getClosestPointForPos(getPosForX(e.getEventRelativeTo(this).getPosition().x));
        
        Array<Point<float>> controlPoints;
        if (startKey->easing->type == Easing::BEZIER)
        {
            CubicEasing* ce1 = (CubicEasing*)startKey->easing.get();
            controlPoints = ce1->getSplitControlPoints(p.x);
        }
            
        AutomationKey* k = manager->addKey(p.x, p.y, true);
        k->easingType->setValueWithData(startKey->easingType->getValueData());
        
        if (startKey->easing->type == Easing::BEZIER)
        {
            CubicEasing* ce1 = (CubicEasing*)startKey->easing.get();
            CubicEasing* ce2 = (CubicEasing*)k->easing.get();
                      
            ce1->anchor1->setPoint(controlPoints[0] - ce1->start);
            ce1->anchor2->setPoint(controlPoints[1] - ce1->end);

            ce2->anchor1->setPoint(controlPoints[2] - ce2->start);
            ce2->anchor2->setPoint(controlPoints[3] - ce2->end);
        }
    }
}

void AutomationUI::addItemFromMenu(AutomationKey* k, bool fromAddbutton, Point<int> pos)
{
    manager->addKey(getPosForX(pos.x), getValueForY(pos.y), true);
}

void AutomationUI::addMenuExtraItems(PopupMenu& p, int startIndex)
{
    //p.addSeparator();
    PopupMenu em;
    int index = 0;
    for (auto& e : Easing::typeNames)
    {
        em.addItem(startIndex+(index++), e);
    }
    p.addSubMenu("Change all easings", em);
}

void AutomationUI::handleMenuExtraItemsResult(int result, int startIndex)
{
    //Easing::Type t = (Easing::Type)(result - startIndex);
    String typeName = Easing::typeNames[result - startIndex];

    Array<UndoableAction*> actions;
    for (auto& i : manager->items)
    {
        actions.add(i->easingType->setUndoableValue(i->easingType->getValueKey(), typeName, true));
    }

    UndoMaster::getInstance()->performActions("Change all easings", actions);
}

Component* AutomationUI::getSelectableComponentForItemUI(AutomationKeyUI* ui)
{
    return &ui->handle;
}

Point<float> AutomationUI::getViewPos(Point<int> pos, bool relative)
{
    return Point<float>(getPosForX(pos.x, relative), getValueForY(pos.y, relative));
}

Rectangle<float> AutomationUI::getViewBounds(Rectangle<int> pos, bool relative)
{
    Rectangle<float> r = Rectangle<float>(getViewPos(pos.getBottomLeft()), getViewPos(pos.getTopRight()));
    if (relative) r.setPosition(0, 0);
    return r;
}

Point<int> AutomationUI::getPosInView(Point<float> pos, bool relative)
{
    return Point<int>(getXForPos(pos.x, relative), getYForValue(pos.y, relative));
}

Rectangle<int> AutomationUI::getBoundsInView(Rectangle<float> pos, bool relative)
{
    Rectangle<int> r = Rectangle<int>(getPosInView(pos.getTopLeft()), getPosInView(pos.getBottomRight()));
    if (relative) r.setPosition(0, 0);
    return r;
}

float AutomationUI::getPosForX(int x, bool relative)
{
    float rel = (x *1.0f / getWidth()) * viewLength;
    return relative ? rel : viewPosRange.x + rel;
}

int AutomationUI::getXForPos(float x, bool relative)
{
    return ((relative ? x : x - viewPosRange.x) / viewLength) * getWidth();
}

float AutomationUI::getValueForY(int y, bool relative)
{
    float valRange = (manager->viewValueRange->y - manager->viewValueRange->x);
    float rel = (1 - y * 1.0f / getHeight());
    return relative ? (1 - rel)* valRange : manager->viewValueRange->x + rel * valRange;
}

int AutomationUI::getYForValue(float x, bool relative)
{
    return (1 - (relative ? x : x - manager->viewValueRange->x) / (manager->viewValueRange->y - manager->viewValueRange->x)) * getHeight();
}

void AutomationUI::newMessage(const AutomationKey::AutomationKeyEvent& e)
{
    switch (e.type)
    {
    case AutomationKey::AutomationKeyEvent::KEY_UPDATED:
    {
        placeKeyUI(getUIForItem(e.key));
    }
    break;

    case AutomationKey::AutomationKeyEvent::SELECTION_CHANGED:
    {
        updateHandlesForUI(getUIForItem(e.key), true);
    }
    break;
    }
}

void AutomationUI::newMessage(const ContainerAsyncEvent& e)
{
    if (e.targetControllable == nullptr || e.targetControllable.wasObjectDeleted() || inspectable.wasObjectDeleted()) return;
    if (e.type == ContainerAsyncEvent::ControllableFeedbackUpdate)
    {
        if (e.targetControllable == manager->value || e.targetControllable == manager->position)
        {
            shouldRepaint = true;
        }
        else if (manager->items.size() > 0 && (e.targetControllable->parentContainer == manager->items[0] || e.targetControllable->parentContainer == manager->items[manager->items.size() - 1]))
        {
            shouldRepaint = true;
        }
        else if (e.targetControllable == manager->viewValueRange)
        {
            resized();
            shouldRepaint = true;
        }
        else if (e.targetControllable == manager->length)
        {
            if (autoAdaptViewRange)
            {
                setViewRange(0, manager->length->floatValue());
            }
        }
    }
}

void AutomationUI::newMessage(const Automation::AutomationEvent& e)
{
    if (e.type == e.INTERACTIVE_SIMPLIFICATION_CHANGED)
    {
        bool interactiveMode = !manager->interactiveSourcePoints.isEmpty();
        if (interactiveMode)
        {
            if (interactiveSimplificationUI == nullptr)
            {
                interactiveSimplificationUI.reset(manager->recorder->simplificationTolerance->createSlider());
                validInteractiveBT.reset(new TextButton("Validate"));
                
                validInteractiveBT->addListener(this);

                addAndMakeVisible(interactiveSimplificationUI.get());
                addAndMakeVisible(validInteractiveBT.get());
            }
        }
        else
        {
            if (interactiveSimplificationUI != nullptr)
            {
                removeChildComponent(interactiveSimplificationUI.get());
                removeChildComponent(validInteractiveBT.get()); 
                
                interactiveSimplificationUI.reset();
                validInteractiveBT.reset();
            }
        }

        resized();
        repaint();
    }
}

void AutomationUI::keyEasingHandleMoved(AutomationKeyUI* ui, bool syncOtherHandle, bool isFirst)
{
    if (syncOtherHandle)
    {
        int index = itemsUI.indexOf(ui);
        if (isFirst)
        {
            if (index > 0)
            {
                if (itemsUI[index - 1]->item->easingType->getValueDataAsEnum<Easing::Type>() == Easing::BEZIER)
                {
                    if (CubicEasing* ce = dynamic_cast<CubicEasing*>(itemsUI[index - 1]->item->easing.get()))
                    {
                        CubicEasing* e = dynamic_cast<CubicEasing*>(ui->item->easing.get());
                        ce->anchor2->setPoint(-e->anchor1->getPoint());
                    }
                }
            }
        }
        else
        {
            if (index < itemsUI.size() - 2)
            {
                if (itemsUI[index + 1]->item->easingType->getValueDataAsEnum<Easing::Type>() == Easing::BEZIER)
                {
                    if (CubicEasing* ce = dynamic_cast<CubicEasing*>(itemsUI[index + 1]->item->easing.get()))
                    {
                        CubicEasing* e = dynamic_cast<CubicEasing*>(ui->item->easing.get());
                        ce->anchor1->setPoint(-e->anchor2->getPoint());
                    }
                }
            }
        }
    }
}

void AutomationUI::timerCallback()
{
    if (shouldRepaint)
    {
        repaint();
        shouldRepaint = false;
    }
}

void AutomationUI::buttonClicked(Button* b)
{
    if (b == validInteractiveBT.get())
    {
        manager->finishInteractiveSimplification();
    }
    else
    {
        BaseManagerUI::buttonClicked(b);
    }
}

