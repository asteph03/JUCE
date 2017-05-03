/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "../jucer_JucerDocument.h"
#include "jucer_ComponentLayoutEditor.h"
#include "jucer_PaintRoutineEditor.h"
#include "jucer_ComponentLayoutPanel.h"


//==============================================================================
/**
*/
class JucerDocumentTabs : public TabbedComponent
{
public:
    JucerDocumentTabs(JucerDocument* const);
    
    void currentTabChanged (const int, const String&);
    
private:
    JucerDocument* const document;
};

class JucerDocumentEditor   : public Component,
                              public ApplicationCommandTarget,
                              public ChangeListener
{
public:
    //==============================================================================
    JucerDocumentEditor (JucerDocument* const document);
    ~JucerDocumentEditor();

    JucerDocument* getDocument() const noexcept         { return document; }

    void refreshPropertiesPanel() const;
    void updateTabs();

    void showLayout();
    void showGraphics (PaintRoutine* routine);

    void setViewportToLastPos (Viewport* vp, EditingPanelBase& editor);
    void storeLastViewportPos (Viewport* vp, EditingPanelBase& editor);

    Image createComponentLayerSnapshot() const;

    //==============================================================================
    void paint (Graphics& g);
    void resized();
    void changeListenerCallback (ChangeBroadcaster*);
    bool keyPressed (const KeyPress&);

    //==============================================================================
    ApplicationCommandTarget* getNextCommandTarget();
    void getAllCommands (Array <CommandID>&);
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result);
    bool perform (const InvocationInfo&);

    static JucerDocumentEditor* getActiveDocumentHolder();

private:
    ScopedPointer<JucerDocument> document;
    JucerDocumentTabs tabbedComponent;
    ComponentLayoutPanel* compLayoutPanel;

    bool isSomethingSelected() const;
    int lastViewportX, lastViewportY;

    double currentZoomLevel;

    // only non-zero if a layout tab is selected
    ComponentLayout* getCurrentLayout() const;
    // only non-zero if a graphics tab is selected
    PaintRoutine* getCurrentPaintRoutine() const;

    void setZoom (double scale);
    double getZoom() const;

    void addElement (const int index);
    void addComponent (const int index);
};
