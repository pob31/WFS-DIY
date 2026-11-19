#pragma once

#include <JuceHeader.h>
#include <vector>
#include "../MapShapes/MapShapeData.h"
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/VarCoercion.h"

/**
 * What the tablet map needs beyond the inputs: the speakers and the drawn
 * shapes (protocol v4, additive). Pure payload builders - collect from the
 * state on the message thread, encode to OSC anywhere - so the chunking can
 * be reasoned about (and tested) on its own.
 *
 * Coordinates are origin-relative stage metres, like the input positions.
 *
 *   /remote/map/rev      ,ii   speakersRev shapesRev
 *   /remote/map/speakers ,iiii rev chunk chunkCount total  + n x (i number, i array, f x, f y, f orientationDeg)
 *   /remote/map/shapes   ,ii   rev count
 *   /remote/map/shape    ,iiiiiiifffs rev index part partCount closed colourARGB dashed
 *                                     lineWidth labelX labelY label  + k x (f x, f y)
 *   /remote/map/request  ,i    mask (1 speakers, 2 shapes)          tablet → desktop
 *
 * Each message stays under ~1000 bytes so a bundle of one fits a datagram.
 * A set is committed by the tablet only once every chunk / part of the same
 * revision has arrived; the revision repeats every 2 s so a lost piece is
 * noticed and re-requested.
 */
namespace RemoteMapGeometry
{

constexpr int speakersPerChunk = 32;
constexpr int pointsPerPart    = 80;
constexpr int maxLabelBytes    = 96;
constexpr int maskSpeakers     = 1;
constexpr int maskShapes       = 2;

struct Speaker
{
    int number = 0;          // 1-based output number (slot + 1)
    int array = 0;           // 0 = single speaker, 1..10
    float x = 0.0f, y = 0.0f;
    float orientation = 0.0f;

    bool operator== (const Speaker& o) const
    {
        return number == o.number && array == o.array && x == o.x && y == o.y && orientation == o.orientation;
    }
    bool operator!= (const Speaker& o) const { return ! (*this == o); }
};

struct ShapeOut
{
    bool closed = true;
    juce::uint32 argb = 0xFFFFFFFF;
    bool dashed = false;
    float lineWidth = 2.0f;
    float labelX = 0.0f, labelY = 0.0f;
    juce::String label;
    std::vector<juce::Point<float>> points;   // flattened outline, stage metres

    bool operator== (const ShapeOut& o) const
    {
        return closed == o.closed && argb == o.argb && dashed == o.dashed && lineWidth == o.lineWidth
            && labelX == o.labelX && labelY == o.labelY && label == o.label && points == o.points;
    }
    bool operator!= (const ShapeOut& o) const { return ! (*this == o); }
};

/** Visible speakers, as MapTab::drawOutputs decides: a single speaker follows
    outputMapVisible, an array member outputArrayMapVisible; absent = visible. */
inline std::vector<Speaker> collectSpeakers (const WFSValueTreeState& state)
{
    using namespace WFSParameterIDs;
    std::vector<Speaker> out;
    const int n = state.getNumOutputChannels();
    for (int i = 0; i < n; ++i)
    {
        const int array = WFSVar::toInt (state.getOutputParameter (i, outputArray), 0);
        const auto visibleVar = state.getOutputParameter (i, array == 0 ? outputMapVisible : outputArrayMapVisible);
        if (! WFSVar::toBool (visibleVar, true))
            continue;

        Speaker s;
        s.number = i + 1;
        s.array = array;
        s.x = WFSVar::toFloat (state.getOutputParameter (i, outputPositionX), 0.0f);
        s.y = WFSVar::toFloat (state.getOutputParameter (i, outputPositionY), 0.0f);
        s.orientation = WFSVar::toFloat (state.getOutputParameter (i, outputOrientation), 0.0f);
        if (std::isfinite (s.x) && std::isfinite (s.y) && std::isfinite (s.orientation))
            out.push_back (s);
    }
    return out;
}

/** UTF-8 text cut to maxBytes without splitting a code point. */
inline juce::String truncateUtf8 (const juce::String& text, int maxBytes)
{
    if ((int) text.getNumBytesAsUTF8() <= maxBytes)
        return text;
    juce::String out;
    for (auto p = text.getCharPointer(); ! p.isEmpty(); ++p)
    {
        juce::String next = out + juce::String::charToString (*p);
        if ((int) next.getNumBytesAsUTF8() > maxBytes)
            break;
        out = next;
    }
    return out;
}

inline std::vector<ShapeOut> collectShapes (const juce::ValueTree& config)
{
    std::vector<ShapeOut> out;
    if (! config.isValid())
        return out;
    for (auto& s : MapDrawing::readShapes (config.getChildWithName (WFSParameterIDs::MapShapes)))
    {
        ShapeOut o;
        o.closed = s.isClosed();
        o.argb = s.colour.getARGB();
        o.dashed = s.dashed;
        o.lineWidth = s.lineWidth;
        const auto anchor = s.getLabelAnchor();
        o.labelX = anchor.x;
        o.labelY = anchor.y;
        o.label = truncateUtf8 (s.label, maxLabelBytes);
        o.points = s.getOutlinePoints();
        if (o.points.size() >= 2)
            out.push_back (std::move (o));
    }
    return out;
}

inline juce::OSCMessage buildRevMessage (int speakersRev, int shapesRev)
{
    juce::OSCMessage m ("/remote/map/rev");
    m.addInt32 (speakersRev);
    m.addInt32 (shapesRev);
    return m;
}

inline void buildSpeakerMessages (std::vector<juce::OSCMessage>& out, int rev, const std::vector<Speaker>& speakers)
{
    const int total = (int) speakers.size();
    const int chunkCount = juce::jmax (1, (total + speakersPerChunk - 1) / speakersPerChunk);
    for (int c = 0; c < chunkCount; ++c)
    {
        juce::OSCMessage m ("/remote/map/speakers");
        m.addInt32 (rev);
        m.addInt32 (c);
        m.addInt32 (chunkCount);
        m.addInt32 (total);
        const int end = juce::jmin (total, (c + 1) * speakersPerChunk);
        for (int i = c * speakersPerChunk; i < end; ++i)
        {
            const auto& s = speakers[(size_t) i];
            m.addInt32 (s.number);
            m.addInt32 (s.array);
            m.addFloat32 (s.x);
            m.addFloat32 (s.y);
            m.addFloat32 (s.orientation);
        }
        out.push_back (std::move (m));
    }
}

inline void buildShapeMessages (std::vector<juce::OSCMessage>& out, int rev, const std::vector<ShapeOut>& shapes)
{
    {
        juce::OSCMessage m ("/remote/map/shapes");
        m.addInt32 (rev);
        m.addInt32 ((int) shapes.size());
        out.push_back (std::move (m));
    }

    for (int index = 0; index < (int) shapes.size(); ++index)
    {
        const auto& s = shapes[(size_t) index];
        const int numPoints = (int) s.points.size();
        const int partCount = juce::jmax (1, (numPoints + pointsPerPart - 1) / pointsPerPart);
        for (int part = 0; part < partCount; ++part)
        {
            juce::OSCMessage m ("/remote/map/shape");
            m.addInt32 (rev);
            m.addInt32 (index);
            m.addInt32 (part);
            m.addInt32 (partCount);
            m.addInt32 (s.closed ? 1 : 0);
            m.addInt32 ((juce::int32) s.argb);
            m.addInt32 (s.dashed ? 1 : 0);
            m.addFloat32 (s.lineWidth);
            m.addFloat32 (s.labelX);
            m.addFloat32 (s.labelY);
            m.addString (part == 0 ? s.label : juce::String());
            const int end = juce::jmin (numPoints, (part + 1) * pointsPerPart);
            for (int k = part * pointsPerPart; k < end; ++k)
            {
                m.addFloat32 (s.points[(size_t) k].x);
                m.addFloat32 (s.points[(size_t) k].y);
            }
            out.push_back (std::move (m));
        }
    }
}

} // namespace RemoteMapGeometry
