#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/VarCoercion.h"
#include "ToolArgs.h"

/*
    cluster_transform: move, rotate, scale and re-level a whole cluster in one
    step, the way the Clusters tab's joysticks and dials do - about the cluster's
    reference point, a tracked member moved through its offset, every member's
    stage and distance constraints applied. It sets positions (parameters), it
    starts nothing; the cluster LFO and presets stay the operator's.

    The tab's own code runs it (ClustersTab::applyTransformForMcp, reached
    through MainComponent), so the rules cannot drift apart.
*/
namespace WFSNetwork::Tools::ClusterTransform
{

struct Request
{
    int clusterId = 1;
    float dx = 0.0f, dy = 0.0f, dz = 0.0f;
    float rotateDeg = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f;
    float attenuationDb = 0.0f;
    int plane = 0;   // 0 XY, 1 XZ, 2 YZ
};

/** Returns the number of member inputs (0: the cluster is empty). */
using Apply = std::function<int (const Request&)>;

inline ToolDescriptor describe (WFSValueTreeState& state, const Apply* apply)
{
    ToolDescriptor d;
    d.name        = "cluster_transform";
    d.description = juce::String (
        "Move, rotate, scale and/or re-level every input of a cluster at once, as the Clusters tab's "
        "joysticks and dials do: about the cluster's reference point (first input or barycentre), a "
        "tracked member moved through its offset, each member's stage and distance constraints "
        "applied. Give any of: dx/dy/dz (metres), rotate (degrees, counter-clockwise in the plane), "
        "scale (factor, or scale_x/scale_y for the plane's two axes), attenuation (dB added to every "
        "member, kept within -92..0), plane (XY default, XZ, YZ) for rotate and scale. Applied in that "
        "order. Returns the members' new positions. Not undoable over MCP (the Clusters tab's undo "
        "has it).") + juce::String (kTier2DescriptionSuffix);

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("cluster_id", Args::integerArg ("Cluster number (1-10).", 1, 10));
    props->setProperty ("dx", Args::numberArg ("Move along X, metres."));
    props->setProperty ("dy", Args::numberArg ("Move along Y (upstage +), metres."));
    props->setProperty ("dz", Args::numberArg ("Move along Z, metres."));
    props->setProperty ("rotate", Args::numberArg ("Rotation about the reference point, degrees.", -360.0, 360.0));
    props->setProperty ("scale", Args::numberArg ("Uniform scale factor about the reference point.", 0.01, 100.0));
    props->setProperty ("scale_x", Args::numberArg ("Scale of the plane's first axis.", 0.01, 100.0));
    props->setProperty ("scale_y", Args::numberArg ("Scale of the plane's second axis.", 0.01, 100.0));
    props->setProperty ("attenuation", Args::numberArg ("dB added to every member's attenuation.", -92.0, 92.0));
    props->setProperty ("plane", Args::enumArg ("Plane for rotate and scale.", { "XY", "XZ", "YZ" }));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "cluster_id" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state, apply] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr || Args::missing (*obj, "cluster_id"))
            return ToolResult::error ("invalid_args", "Missing required arg: cluster_id");
        if (apply == nullptr || ! *apply)
            return ToolResult::error ("unavailable", "The Clusters tab is not available in this session");

        juce::String code, message;
        Request r;
        const auto cluster = Args::wholeNumber (obj->getProperty ("cluster_id"), "cluster_id", 1, 10, code, message);
        if (! cluster.has_value()) return ToolResult::error (code, message);
        r.clusterId = *cluster;

        auto readNumber = [&] (const char* name, float& target, double lo, double hi) -> bool
        {
            if (Args::missing (*obj, name))
                return true;
            const auto v = Args::number (obj->getProperty (name), name, code, message);
            if (! v.has_value()) return false;
            if (*v < lo || *v > hi)
            {
                code = "out_of_range";
                message = juce::String (name) + " " + juce::String (*v) + " not in [" + juce::String (lo)
                        + ", " + juce::String (hi) + "]";
                return false;
            }
            target = static_cast<float> (*v);
            return true;
        };
        float scale = 1.0f;
        if (! readNumber ("dx", r.dx, -100.0, 100.0) || ! readNumber ("dy", r.dy, -100.0, 100.0)
            || ! readNumber ("dz", r.dz, -100.0, 100.0) || ! readNumber ("rotate", r.rotateDeg, -360.0, 360.0)
            || ! readNumber ("scale", scale, 0.01, 100.0) || ! readNumber ("attenuation", r.attenuationDb, -92.0, 92.0))
            return ToolResult::error (code, message);
        r.scaleX = r.scaleY = scale;
        if (! readNumber ("scale_x", r.scaleX, 0.01, 100.0) || ! readNumber ("scale_y", r.scaleY, 0.01, 100.0))
            return ToolResult::error (code, message);

        if (! Args::missing (*obj, "plane"))
        {
            const auto p = obj->getProperty ("plane").toString().toUpperCase();
            r.plane = p == "XY" ? 0 : p == "XZ" ? 1 : p == "YZ" ? 2 : -1;
            if (r.plane < 0)
                return ToolResult::error ("invalid_args", "plane must be XY, XZ or YZ");
        }

        if (r.dx == 0 && r.dy == 0 && r.dz == 0 && r.rotateDeg == 0 && r.scaleX == 1 && r.scaleY == 1
            && r.attenuationDb == 0)
            return ToolResult::error ("invalid_args", "Nothing to do: give dx/dy/dz, rotate, scale or attenuation");

        const int members = (*apply) (r);
        if (members == 0)
            return ToolResult::error ("empty", "Cluster " + juce::String (r.clusterId) + " has no inputs");

        juce::Array<juce::var> positions;
        for (int slot = 0; slot < state.getNumInputChannels(); ++slot)
        {
            if (WFSVar::toInt (state.getInputParameter (slot, WFSParameterIDs::inputCluster)) != r.clusterId)
                continue;
            auto p = std::make_unique<juce::DynamicObject>();
            p->setProperty ("input_id", state.getInputChannelNumber (slot));
            p->setProperty ("x", state.getInputParameter (slot, WFSParameterIDs::inputPositionX));
            p->setProperty ("y", state.getInputParameter (slot, WFSParameterIDs::inputPositionY));
            p->setProperty ("z", state.getInputParameter (slot, WFSParameterIDs::inputPositionZ));
            p->setProperty ("attenuation", state.getInputParameter (slot, WFSParameterIDs::inputAttenuation));
            positions.add (juce::var (p.release()));
        }

        if (record != nullptr)
        {
            record->operatorDescription = "Transformed cluster " + juce::String (r.clusterId) + " ("
                                        + juce::String (members) + " inputs; not undoable over MCP)";
            record->affectedParameters.add ("inputPositionX");
            record->affectedGroups.push_back ({ 0, juce::String ("Clusters") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("cluster_id", r.clusterId);
        result->setProperty ("members", positions);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::ClusterTransform
