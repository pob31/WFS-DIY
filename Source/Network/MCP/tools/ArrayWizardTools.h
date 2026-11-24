#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSParameterDefaults.h"
#include "../../../Helpers/ArrayGeometryCalculator.h"
#include "ToolArgs.h"

/*
    The Wizard of OutZ over MCP: lay out a range of outputs as one speaker
    array, with the window's own geometry (ArrayGeometry, Helpers/) and the same
    writes as its Apply (OutputArrayHelperContent::applyToOutputs): position,
    orientation, array assignment, and the acoustic settings that are given -
    HF damping, Live Source / Floor Reflection enables, parallax, distance
    attenuation, a Low Cut on band 1 and a High Cut on band 6. One Output-domain
    undo transaction, as the window.
*/
namespace WFSNetwork::Tools::ArrayWizard
{

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "output_apply_array_geometry";
    d.description = juce::String (
        "Lay out a run of outputs as one speaker array, as the Wizard of OutZ does: computes each "
        "speaker's position and orientation and assigns them to an array. method and its arguments "
        "(metres, degrees):\n"
        "- straight_center: center_x, center_y, z, spacing, orientation (0 = facing the audience)\n"
        "- straight_endpoints: start_x, start_y, end_x, end_y, z, orientation (omit for perpendicular)\n"
        "- curved: start_x, start_y, end_x, end_y, sag (+ bows upstage), z - speakers face out of the curve\n"
        "- circle: center_x, center_y, radius, start_angle (0 = top, clockwise), z, facing_inward\n"
        "- surround_pairs: count is the number of PAIRS; center_x, width, y_start, y_end, z\n"
        "Optional acoustics written to every speaker: hf_damping (dB/m), h_parallax, v_parallax (m), "
        "distance_atten_percent, live_source_enable, floor_reflections_enable, low_cut_hz (band 1), "
        "high_cut_hz (band 6). Not undoable over MCP (one step on the Outputs tab).")
        + juce::String (kTier2DescriptionSuffix);

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("array", Args::integerArg ("Output array number (1-10).", 1, 10));
    props->setProperty ("start_output", Args::integerArg ("First output channel (1-based).", 1));
    props->setProperty ("count", Args::integerArg ("Number of speakers (pairs for surround_pairs).", 1, 128));
    props->setProperty ("method", Args::enumArg ("Geometry.", { "straight_center", "straight_endpoints", "curved",
                                                                "circle", "surround_pairs" }));
    for (const char* n : { "center_x", "center_y", "z", "spacing", "orientation", "start_x", "start_y", "end_x",
                           "end_y", "sag", "radius", "start_angle", "width", "y_start", "y_end",
                           "hf_damping", "h_parallax", "v_parallax", "distance_atten_percent",
                           "low_cut_hz", "high_cut_hz" })
        props->setProperty (n, Args::numberArg ("See the description."));
    for (const char* n : { "facing_inward", "live_source_enable", "floor_reflections_enable" })
        props->setProperty (n, Args::booleanArg ("See the description."));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "array", "start_output", "count", "method" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr)
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
        juce::String code, message;
        const int numOutputs = state.getNumOutputChannels();
        const auto arrayId = Args::wholeNumber (obj->getProperty ("array"), "array", 1, 10, code, message);
        if (! arrayId) return ToolResult::error (code, message);
        const auto start = Args::wholeNumber (obj->getProperty ("start_output"), "start_output", 1, juce::jmax (1, numOutputs), code, message);
        if (! start) return ToolResult::error (code, message);
        const auto count = Args::wholeNumber (obj->getProperty ("count"), "count", 1, 128, code, message);
        if (! count) return ToolResult::error (code, message);
        const auto method = obj->getProperty ("method").toString();

        auto get = [&] (const char* name, std::optional<float> fallback = std::nullopt) -> std::optional<float>
        {
            if (Args::missing (*obj, name))
            {
                if (! fallback)
                {
                    code = "invalid_args";
                    message = "Missing required arg for " + method + ": " + name;
                }
                return fallback;
            }
            auto v = Args::number (obj->getProperty (name), name, code, message);
            return v ? std::optional<float> (static_cast<float> (*v)) : std::nullopt;
        };

        std::vector<SpeakerPosition> positions;
        if (method == "straight_center")
        {
            auto cx = get ("center_x"), cy = get ("center_y"), z = get ("z", 0.0f), sp = get ("spacing"), o = get ("orientation", 0.0f);
            if (! cx || ! cy || ! z || ! sp || ! o) return ToolResult::error (code, message);
            positions = ArrayGeometry::calculateStraightFromCenter (*count, *cx, *cy, *z, *sp, *o);
        }
        else if (method == "straight_endpoints")
        {
            auto sx = get ("start_x"), sy = get ("start_y"), ex = get ("end_x"), ey = get ("end_y"), z = get ("z", 0.0f), o = get ("orientation", -1.0f);
            if (! sx || ! sy || ! ex || ! ey || ! z || ! o) return ToolResult::error (code, message);
            positions = ArrayGeometry::calculateStraightFromEndpoints (*count, *sx, *sy, *ex, *ey, *z, *o);
        }
        else if (method == "curved")
        {
            auto sx = get ("start_x"), sy = get ("start_y"), ex = get ("end_x"), ey = get ("end_y"), sag = get ("sag"), z = get ("z", 0.0f);
            if (! sx || ! sy || ! ex || ! ey || ! sag || ! z) return ToolResult::error (code, message);
            positions = ArrayGeometry::calculateCurvedArray (*count, *sx, *sy, *ex, *ey, *sag, *z);
        }
        else if (method == "circle")
        {
            auto cx = get ("center_x"), cy = get ("center_y"), r = get ("radius"), a = get ("start_angle", 0.0f), z = get ("z", 0.0f);
            if (! cx || ! cy || ! r || ! a || ! z) return ToolResult::error (code, message);
            const bool inward = ! obj->hasProperty ("facing_inward") || static_cast<bool> (obj->getProperty ("facing_inward"));
            positions = ArrayGeometry::calculateCircleArray (*count, *cx, *cy, *r, *a, *z, inward);
        }
        else if (method == "surround_pairs")
        {
            auto cx = get ("center_x", 0.0f), w = get ("width"), y0 = get ("y_start"), y1 = get ("y_end"), z = get ("z", 0.0f);
            if (! cx || ! w || ! y0 || ! y1 || ! z) return ToolResult::error (code, message);
            positions = ArrayGeometry::calculateSurroundPairs (*count, *cx, *w, *y0, *y1, *z);
        }
        else
            return ToolResult::error ("invalid_args", "method must be straight_center, straight_endpoints, curved, circle or surround_pairs");

        const int n = static_cast<int> (positions.size());
        if (n == 0)
            return ToolResult::error ("invalid_args", "The geometry produced no speaker positions");
        if (*start - 1 + n > numOutputs)
            return ToolResult::error ("out_of_range", juce::String (n) + " speakers from output " + juce::String (*start)
                                                        + " need more than the " + juce::String (numOutputs) + " outputs");

        using namespace WFSParameterDefaults;
        auto opt = [&] (const char* name, float lo, float hi) -> std::optional<float>
        {
            if (Args::missing (*obj, name)) return std::nullopt;
            return juce::jlimit (lo, hi, static_cast<float> (static_cast<double> (obj->getProperty (name))));
        };
        const auto hf   = opt ("hf_damping", outputHFdampingMin, outputHFdampingMax);
        const auto hp   = opt ("h_parallax", outputHparallaxMin, outputHparallaxMax);
        const auto vp   = opt ("v_parallax", outputVparallaxMin, outputVparallaxMax);
        const auto dist = opt ("distance_atten_percent", (float) outputDistanceAttenPercentMin, (float) outputDistanceAttenPercentMax);
        const auto lowCut  = opt ("low_cut_hz",  (float) eqFrequencyMin, (float) eqFrequencyMax);
        const auto highCut = opt ("high_cut_hz", (float) eqFrequencyMin, (float) eqFrequencyMax);

        auto* um = state.getUndoManagerForDomain (UndoDomain::Output);
        um->beginNewTransaction ("Array Position Helper (MCP)");
        const auto inStage = [] (float v) { return juce::jlimit (outputPositionMin, outputPositionMax, v); };

        juce::Array<juce::var> placed;
        for (int i = 0; i < n; ++i)
        {
            const int idx = *start - 1 + i;
            const auto& p = positions[(size_t) i];
            auto pos  = state.getOutputPositionSection (idx);
            auto opts = state.getOutputOptionsSection (idx);
            auto chan = state.getOutputChannelSection (idx);
            if (! pos.isValid() || ! opts.isValid() || ! chan.isValid())
                continue;

            pos.setProperty (WFSParameterIDs::outputPositionX, inStage (p.x), um);
            pos.setProperty (WFSParameterIDs::outputPositionY, inStage (p.y), um);
            pos.setProperty (WFSParameterIDs::outputPositionZ, inStage (p.z), um);
            pos.setProperty (WFSParameterIDs::outputOrientation,
                             juce::jlimit (outputOrientationMin, outputOrientationMax, juce::roundToInt (p.orientation)), um);
            chan.setProperty (WFSParameterIDs::outputArray, *arrayId, um);
            if (hf)   pos.setProperty  (WFSParameterIDs::outputHFdamping, *hf, um);
            if (hp)   opts.setProperty (WFSParameterIDs::outputHparallax, *hp, um);
            if (vp)   opts.setProperty (WFSParameterIDs::outputVparallax, *vp, um);
            if (dist) opts.setProperty (WFSParameterIDs::outputDistanceAttenPercent, juce::roundToInt (*dist), um);
            if (obj->hasProperty ("live_source_enable"))
                opts.setProperty (WFSParameterIDs::outputLSattenEnable, static_cast<bool> (obj->getProperty ("live_source_enable")) ? 1 : 0, um);
            if (obj->hasProperty ("floor_reflections_enable"))
                opts.setProperty (WFSParameterIDs::outputFRenable, static_cast<bool> (obj->getProperty ("floor_reflections_enable")) ? 1 : 0, um);
            if (lowCut || highCut)
                if (auto eq = state.getOutputEQSection (idx); eq.isValid())
                    eq.setProperty (WFSParameterIDs::outputEQenabled, 1, um);
            if (lowCut)
                if (auto b = state.getOutputEQBand (idx, 0); b.isValid())
                {
                    b.setProperty (WFSParameterIDs::eqShape, 1, um);   // LowCut
                    b.setProperty (WFSParameterIDs::eqFrequency, *lowCut, um);
                }
            if (highCut)
                if (auto b = state.getOutputEQBand (idx, 5); b.isValid())
                {
                    b.setProperty (WFSParameterIDs::eqShape, 6, um);   // HighCut
                    b.setProperty (WFSParameterIDs::eqFrequency, *highCut, um);
                }

            auto s = std::make_unique<juce::DynamicObject>();
            s->setProperty ("output_id", idx + 1);
            s->setProperty ("x", p.x);
            s->setProperty ("y", p.y);
            s->setProperty ("z", p.z);
            s->setProperty ("orientation", juce::roundToInt (p.orientation));
            placed.add (juce::var (s.release()));
        }

        if (record != nullptr)
        {
            record->operatorDescription = "Laid out outputs " + juce::String (*start) + "-" + juce::String (*start + n - 1)
                                        + " as array " + juce::String (*arrayId) + " (" + method
                                        + "; undo on the Outputs tab, not over MCP)";
            record->affectedParameters.add ("outputPositionX");
            record->affectedGroups.push_back ({ 0, juce::String ("Array Wizard") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("array", *arrayId);
        result->setProperty ("speakers", placed);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::ArrayWizard
