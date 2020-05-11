/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   22nd April 2020).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

package com.rmsl.juce;

import android.view.OrientationEventListener;
import android.content.Context;

public class JuceOrientationEventListener extends OrientationEventListener
{
    private native void deviceOrientationChanged (long host, int orientation);

    public JuceOrientationEventListener (long hostToUse, Context context, int rate)
    {
        super (context, rate);

        host = hostToUse;
    }

    @Override
    public void onOrientationChanged (int orientation)
    {
        deviceOrientationChanged (host, orientation);
    }

    private long host;
}
