#pragma once

#include <JuceHeader.h>

/*
* Interface for components that can act as a drag source for modulation
* connections. Returns the centre of the crosshair handle in local coordinates,
* which is used as the start point of the drag-and-drop cable.
*/
class ModulationDragSource
{
public:
    virtual ~ModulationDragSource() = default;

    /** Centre of the crosshair handle, in the component's local coordinates. */
    virtual juce::Point<float> getDragSource() = 0;
};
