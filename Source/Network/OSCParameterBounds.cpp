#include "OSCParameterBounds.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

#include <unordered_map>

namespace WFSNetwork
{

namespace
{
    // The juce::Identifier::operator== comparison uses pointer-equality
    // on the shared string pool, so std::unordered_map<juce::Identifier>
    // needs a hasher. JUCE provides one via std::hash<juce::Identifier>
    // in juce_String.h (operator delegates to the underlying char*).
    struct IdentifierHash
    {
        size_t operator() (const juce::Identifier& id) const noexcept
        {
            return std::hash<const char*>{} (id.toString().toRawUTF8());
        }
    };

    using BoundsMap = std::unordered_map<juce::Identifier, ParamBounds, IdentifierHash>;

    // Convenience macros: the WFSParameterIDs identifier name and the
    // WFSParameterDefaults constant prefix usually match exactly.
    #define BIND_F(name)                                                    \
        m[WFSParameterIDs::name] = {                                        \
            (double) WFSParameterDefaults::name##Min,                       \
            (double) WFSParameterDefaults::name##Max,                       \
            false                                                           \
        }
    #define BIND_I(name)                                                    \
        m[WFSParameterIDs::name] = {                                        \
            (double) WFSParameterDefaults::name##Min,                       \
            (double) WFSParameterDefaults::name##Max,                       \
            true                                                            \
        }

    // For paramIds that share a Min/Max constant pair (e.g. X/Y/Z all
    // use `inputPositionMin/Max`), bind one paramId to the constant
    // pair from a different name.
    #define BIND_F_AS(paramId, prefix)                                      \
        m[WFSParameterIDs::paramId] = {                                     \
            (double) WFSParameterDefaults::prefix##Min,                     \
            (double) WFSParameterDefaults::prefix##Max,                     \
            false                                                           \
        }
    #define BIND_I_AS(paramId, prefix)                                      \
        m[WFSParameterIDs::paramId] = {                                     \
            (double) WFSParameterDefaults::prefix##Min,                     \
            (double) WFSParameterDefaults::prefix##Max,                     \
            true                                                            \
        }

    // Hand-pick boolean toggles where Min/Max constants don't exist but
    // the legal range is clearly 0..1.
    #define BIND_BOOL(name)                                                 \
        m[WFSParameterIDs::name] = { 0.0, 1.0, true }

    const BoundsMap& bounds()
    {
        static const BoundsMap map = []
        {
            BoundsMap m;
            m.reserve (256);

            //------------------------------------------------------------------
            // Config / Stage geometry
            //------------------------------------------------------------------
            BIND_I (algorithmDSP);
            BIND_I (stageShape);
            BIND_F (stageWidth);
            BIND_F (stageDepth);
            BIND_F (stageHeight);
            BIND_F (stageDiameter);
            BIND_F (domeElevation);
            BIND_F_AS (originWidth,  originWidth);
            BIND_F_AS (originDepth,  originDepth);
            BIND_F_AS (originHeight, originHeight);
            BIND_F (speedOfSound);
            BIND_F (temperature);
            BIND_F (masterLevel);
            BIND_F (systemLatency);
            BIND_F (haasEffect);

            //------------------------------------------------------------------
            // Config / Binaural
            //------------------------------------------------------------------
            BIND_I (binauralOutputChannel);
            BIND_F (binauralListenerDistance);
            BIND_I (binauralListenerAngle);
            BIND_F (binauralAttenuation);
            BIND_F (binauralDelay);

            //------------------------------------------------------------------
            // Config / Tracking
            //------------------------------------------------------------------
            BIND_I (trackingProtocol);

            //------------------------------------------------------------------
            // Cluster / LFO (per-cluster)
            //------------------------------------------------------------------
            BIND_F (clusterLFOperiod);
            BIND_I (clusterLFOphase);
            // Shape: enum-like, bind with explicit range from inputLFOshape.
            BIND_I_AS (clusterLFOshapeX,     inputLFOshape);
            BIND_I_AS (clusterLFOshapeY,     inputLFOshape);
            BIND_I_AS (clusterLFOshapeZ,     inputLFOshape);
            BIND_I_AS (clusterLFOshapeRot,   inputLFOshape);
            BIND_I_AS (clusterLFOshapeScale, inputLFOshape);
            BIND_F_AS (clusterLFOrateX,     clusterLFOrate);
            BIND_F_AS (clusterLFOrateY,     clusterLFOrate);
            BIND_F_AS (clusterLFOrateZ,     clusterLFOrate);
            BIND_F_AS (clusterLFOrateRot,   clusterLFOrate);
            BIND_F_AS (clusterLFOrateScale, clusterLFOrate);
            BIND_F_AS (clusterLFOamplitudeX, clusterLFOamplitudeXYZ);
            BIND_F_AS (clusterLFOamplitudeY, clusterLFOamplitudeXYZ);
            BIND_F_AS (clusterLFOamplitudeZ, clusterLFOamplitudeXYZ);
            BIND_I_AS (clusterLFOamplitudeRot,   clusterLFOamplitudeRot);
            BIND_F_AS (clusterLFOamplitudeScale, clusterLFOamplitudeScale);

            //------------------------------------------------------------------
            // Input / Channel
            //------------------------------------------------------------------
            BIND_F (inputAttenuation);
            BIND_F (inputDelayLatency);
            BIND_BOOL (inputMinimalLatency);

            //------------------------------------------------------------------
            // Input / Position (X/Y/Z share one Min/Max pair)
            //------------------------------------------------------------------
            BIND_F_AS (inputPositionX, inputPosition);
            BIND_F_AS (inputPositionY, inputPosition);
            BIND_F_AS (inputPositionZ, inputPosition);
            BIND_F_AS (inputOffsetX,   inputOffset);
            BIND_F_AS (inputOffsetY,   inputOffset);
            BIND_F_AS (inputOffsetZ,   inputOffset);
            BIND_BOOL (inputConstraintX);
            BIND_BOOL (inputConstraintY);
            BIND_BOOL (inputConstraintZ);
            BIND_BOOL (inputConstraintDistance);
            BIND_F_AS (inputConstraintDistanceMin, inputPosition);
            BIND_F_AS (inputConstraintDistanceMax, inputPosition);
            BIND_BOOL (inputFlipX);
            BIND_BOOL (inputFlipY);
            BIND_BOOL (inputFlipZ);
            BIND_I (inputCluster);
            BIND_BOOL (inputTrackingActive);
            BIND_I_AS (inputTrackingID, inputTrackingID);
            BIND_I (inputTrackingSmooth);
            BIND_BOOL (inputMaxSpeedActive);
            BIND_F (inputMaxSpeed);
            BIND_BOOL (inputPathModeActive);
            BIND_I (inputHeightFactor);
            BIND_I (inputCoordinateMode);

            //------------------------------------------------------------------
            // Input / Attenuation
            //------------------------------------------------------------------
            BIND_BOOL (inputAttenuationLaw);
            BIND_F (inputDistanceAttenuation);
            BIND_F (inputDistanceRatio);
            BIND_I (inputCommonAtten);

            //------------------------------------------------------------------
            // Input / Directivity
            //------------------------------------------------------------------
            BIND_I (inputDirectivity);
            BIND_I (inputRotation);
            BIND_I (inputTilt);
            BIND_F (inputHFshelf);

            //------------------------------------------------------------------
            // Input / Live Source Tamer
            //------------------------------------------------------------------
            BIND_BOOL (inputLSactive);
            BIND_F (inputLSradius);
            BIND_I (inputLSshape);
            BIND_F (inputLSattenuation);
            BIND_BOOL (inputLSpeakEnable);
            BIND_F (inputLSpeakThreshold);
            BIND_F (inputLSpeakRatio);
            BIND_BOOL (inputLSslowEnable);
            BIND_F (inputLSslowThreshold);
            BIND_F (inputLSslowRatio);

            //------------------------------------------------------------------
            // Input / Hackoustics (Floor Reflections)
            //------------------------------------------------------------------
            BIND_BOOL (inputFRactive);
            BIND_F (inputFRattenuation);
            BIND_BOOL (inputFRlowCutActive);
            BIND_I_AS (inputFRlowCutFreq, inputFRfreq);
            BIND_BOOL (inputFRhighShelfActive);
            BIND_I_AS (inputFRhighShelfFreq, inputFRfreq);
            BIND_F (inputFRhighShelfGain);
            BIND_F (inputFRhighShelfSlope);
            BIND_I (inputFRdiffusion);

            //------------------------------------------------------------------
            // Input / Jitter + LFO
            //------------------------------------------------------------------
            BIND_F (inputJitter);
            BIND_BOOL (inputLFOactive);
            BIND_F (inputLFOperiod);
            BIND_I (inputLFOphase);
            BIND_I_AS (inputLFOshapeX, inputLFOshape);
            BIND_I_AS (inputLFOshapeY, inputLFOshape);
            BIND_I_AS (inputLFOshapeZ, inputLFOshape);
            BIND_F_AS (inputLFOrateX, inputLFOrate);
            BIND_F_AS (inputLFOrateY, inputLFOrate);
            BIND_F_AS (inputLFOrateZ, inputLFOrate);
            BIND_F_AS (inputLFOamplitudeX, inputLFOamplitude);
            BIND_F_AS (inputLFOamplitudeY, inputLFOamplitude);
            BIND_F_AS (inputLFOamplitudeZ, inputLFOamplitude);
            BIND_I_AS (inputLFOphaseX, inputLFOphase);
            BIND_I_AS (inputLFOphaseY, inputLFOphase);
            BIND_I_AS (inputLFOphaseZ, inputLFOphase);
            BIND_I (inputLFOgyrophone);

            //------------------------------------------------------------------
            // Input / AutomOtion
            //------------------------------------------------------------------
            BIND_F_AS (inputOtomoX, inputOtomo);
            BIND_F_AS (inputOtomoY, inputOtomo);
            BIND_F_AS (inputOtomoZ, inputOtomo);
            BIND_BOOL (inputOtomoAbsoluteRelative);
            BIND_BOOL (inputOtomoStayReturn);
            BIND_I (inputOtomoSpeedProfile);
            BIND_F (inputOtomoDuration);
            BIND_I (inputOtomoCurve);
            BIND_BOOL (inputOtomoTrigger);
            BIND_F_AS (inputOtomoThreshold, inputOtomoThreshold);
            BIND_F_AS (inputOtomoReset,     inputOtomoReset);
            BIND_BOOL (inputOtomoPauseResume);
            BIND_I_AS (inputOtomoCoordinateMode, inputCoordinateMode);
            BIND_F (inputOtomoR);
            BIND_F (inputOtomoTheta);
            BIND_F (inputOtomoRsph);
            BIND_F (inputOtomoPhi);

            //------------------------------------------------------------------
            // Input / Sidelines
            //------------------------------------------------------------------
            BIND_BOOL (inputSidelinesActive);
            BIND_F (inputSidelinesFringe);

            //------------------------------------------------------------------
            // Input / Sampler
            //------------------------------------------------------------------
            BIND_BOOL (inputSamplerActive);

            //------------------------------------------------------------------
            // Output / Channel
            //------------------------------------------------------------------
            BIND_I (outputArray);
            BIND_F (outputAttenuation);
            BIND_F (outputDelayLatency);

            //------------------------------------------------------------------
            // Output / Position
            //------------------------------------------------------------------
            BIND_F_AS (outputPositionX, outputPosition);
            BIND_F_AS (outputPositionY, outputPosition);
            BIND_F_AS (outputPositionZ, outputPosition);
            BIND_I (outputOrientation);
            BIND_I (outputAngleOn);
            BIND_I (outputAngleOff);
            BIND_I (outputPitch);
            BIND_F (outputHFdamping);
            BIND_I (outputCoordinateMode);

            //------------------------------------------------------------------
            // Output / Options
            //------------------------------------------------------------------
            BIND_BOOL (outputMiniLatencyEnable);
            BIND_BOOL (outputLSattenEnable);
            BIND_BOOL (outputFRenable);
            BIND_I (outputDistanceAttenPercent);
            BIND_F_AS (outputHparallax, outputParallax);
            BIND_F_AS (outputVparallax, outputParallax);

            //------------------------------------------------------------------
            // Output / EQ (per-band)
            //------------------------------------------------------------------
            BIND_BOOL (outputEQenabled);
            BIND_I (eqShape);
            BIND_F (eqFrequency);
            BIND_F (eqGain);
            BIND_F (eqQ);
            BIND_F (eqSlope);

            //------------------------------------------------------------------
            // Reverb / Channel + Position
            //------------------------------------------------------------------
            BIND_F (reverbAttenuation);
            BIND_F (reverbDelayLatency);
            BIND_F_AS (reverbPositionX, reverbPosition);
            BIND_F_AS (reverbPositionY, reverbPosition);
            BIND_F_AS (reverbPositionZ, reverbPosition);
            BIND_F_AS (reverbReturnOffsetX, reverbReturnOffset);
            BIND_F_AS (reverbReturnOffsetY, reverbReturnOffset);
            BIND_F_AS (reverbReturnOffsetZ, reverbReturnOffset);
            BIND_I (reverbCoordinateMode);
            BIND_I (reverbOrientation);
            BIND_I (reverbAngleOn);
            BIND_I (reverbAngleOff);
            BIND_I (reverbPitch);
            BIND_F (reverbHFdamping);
            BIND_BOOL (reverbMiniLatencyEnable);
            BIND_BOOL (reverbLSenable);
            BIND_I (reverbDistanceAttenEnable);

            //------------------------------------------------------------------
            // Reverb / Pre-EQ (per-channel, per-band)
            //------------------------------------------------------------------
            BIND_BOOL (reverbPreEQenable);
            BIND_I (reverbPreEQshape);
            BIND_I (reverbPreEQfreq);
            BIND_F (reverbPreEQgain);
            BIND_F (reverbPreEQq);
            BIND_F (reverbPreEQslope);

            //------------------------------------------------------------------
            // Reverb / Return
            //------------------------------------------------------------------
            BIND_F (reverbDistanceAttenuation);
            BIND_I (reverbCommonAtten);

            //------------------------------------------------------------------
            // Reverb / Algorithm (global)
            //------------------------------------------------------------------
            BIND_I (reverbAlgoType);
            BIND_F (reverbRT60);
            BIND_F (reverbRT60LowMult);
            BIND_F (reverbRT60HighMult);
            BIND_F (reverbCrossoverLow);
            BIND_F (reverbCrossoverHigh);
            BIND_F (reverbDiffusion);
            BIND_F (reverbSDNscale);
            BIND_F (reverbFDNsize);
            BIND_F (reverbIRtrim);
            BIND_F (reverbIRlength);
            BIND_BOOL (reverbPerNodeIR);
            BIND_F (reverbWetLevel);

            //------------------------------------------------------------------
            // Reverb / Pre-Compressor (global)
            //------------------------------------------------------------------
            BIND_BOOL (reverbPreCompBypass);
            BIND_F (reverbPreCompThreshold);
            BIND_F (reverbPreCompRatio);
            BIND_F (reverbPreCompAttack);
            BIND_F (reverbPreCompRelease);

            //------------------------------------------------------------------
            // Reverb / Post-EQ (global, per-band)
            //------------------------------------------------------------------
            BIND_BOOL (reverbPostEQenable);
            BIND_I (reverbPostEQshape);
            BIND_I (reverbPostEQfreq);
            BIND_F (reverbPostEQgain);
            BIND_F (reverbPostEQq);
            BIND_F (reverbPostEQslope);

            //------------------------------------------------------------------
            // Reverb / Post-Expander (global)
            //------------------------------------------------------------------
            BIND_BOOL (reverbPostExpBypass);
            BIND_F (reverbPostExpThreshold);
            BIND_F (reverbPostExpRatio);
            BIND_F (reverbPostExpAttack);
            BIND_F (reverbPostExpRelease);

            return m;
        }();
        return map;
    }

    #undef BIND_F
    #undef BIND_I
    #undef BIND_F_AS
    #undef BIND_I_AS
    #undef BIND_BOOL
}

std::optional<ParamBounds> getBounds (const juce::Identifier& paramId)
{
    const auto& m = bounds();
    auto it = m.find (paramId);
    if (it == m.end())
        return std::nullopt;
    return it->second;
}

bool isInRange (const juce::Identifier& paramId, double value)
{
    auto b = getBounds (paramId);
    if (! b.has_value())
        return true;
    return value >= b->min && value <= b->max;
}

juce::String formatOutOfRangeReason (const juce::Identifier& paramId, double value)
{
    auto b = getBounds (paramId);
    if (! b.has_value())
        return "out of range";
    const int decimals = b->isInt ? 0 : 3;
    return "out of range: " + juce::String (value, decimals)
         + " not in [" + juce::String (b->min, decimals)
         + ", " + juce::String (b->max, decimals) + "]";
}

} // namespace WFSNetwork
