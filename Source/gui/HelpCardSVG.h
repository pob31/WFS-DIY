#pragma once
// Regenerated: 2026-03-29 20:03:47

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

namespace HelpCardSVG
{

inline juce::String adaptForTheme(const juce::String& svg)
{
    auto result = svg;
    bool isDark = ColorScheme::get().background.getBrightness() < 0.5f;
    if (isDark)
    {
        result = result.replace("stroke: #000", "stroke: #ddd")
                       .replace("stroke:#000", "stroke:#ddd")
                       .replace("fill: #000", "fill: #ddd")
                       .replace("fill:#000", "fill:#ddd");
        result = result.replace("stroke: blue", "stroke: #6699ff")
                       .replace("stroke:blue", "stroke:#6699ff")
                       .replace("fill: blue", "fill: #6699ff")
                       .replace("fill:blue", "fill:#6699ff")
                       .replace("stroke: #009245", "stroke: #33cc77")
                       .replace("stroke:#009245", "stroke:#33cc77");
        result = result.replace("<polygon points=", "<polygon fill=\"#ddd\" points=");
    }
    return result;
}

/** Localize text labels in the signal flow SVG */
inline juce::String localizeSignalFlow(const juce::String& svg)
{
    auto result = svg;
    auto& loc = LocalizationManager::getInstance();
    result = result.replace(">Microphones<", ">" + loc.get("signalFlow.microphones") + "<")
                   .replace(">Soundtracks<", ">" + loc.get("signalFlow.soundtracks") + "<")
                   .replace(">OSC commands<", ">" + loc.get("signalFlow.oscCommands") + "<")
                   .replace(">Console<", ">" + loc.get("signalFlow.console") + "<")
                   .replace(">Direct outs<", ">" + loc.get("signalFlow.directOuts") + "<")
                   .replace(">post fader<", ">" + loc.get("signalFlow.postFader") + "<")
                   .replace(">WFS processor<", ">" + loc.get("signalFlow.wfsProcessor") + "<")
                   .replace(">Inputs<", ">" + loc.get("signalFlow.inputs") + "<")
                   .replace(">Outputs<", ">" + loc.get("signalFlow.outputs") + "<")
                   .replace(">QLab / Ableton Live<", ">" + loc.get("signalFlow.qlabAbleton") + "<")
                   .replace(">positions and<", ">" + loc.get("signalFlow.positions") + "<")
                   .replace(">other parameters<", ">" + loc.get("signalFlow.otherParams") + "<")
                   .replace(">scenes<", ">" + loc.get("signalFlow.scenes") + "<")
                   .replace(">recall<", ">" + loc.get("signalFlow.recall") + "<")
                   .replace(">Remote<", ">" + loc.get("signalFlow.remote") + "<")
                   .replace(">remote<", ">" + loc.get("signalFlow.remoteLC") + "<")
                   .replace(">control<", ">" + loc.get("signalFlow.control") + "<")
                   .replace(">Speaker arrays<", ">" + loc.get("signalFlow.speakerArrays") + "<");
    return result;
}

inline std::unique_ptr<juce::Drawable> parse(const juce::String& svg)
{
    auto adapted = adaptForTheme(svg);
    auto xml = juce::XmlDocument::parse(adapted);
    if (xml == nullptr) return nullptr;
    return juce::Drawable::createFromSVG(*xml);
}

inline std::unique_ptr<juce::Drawable> parseSignalFlow(const juce::String& svg)
{
    auto localized = localizeSignalFlow(svg);
    auto adapted = adaptForTheme(localized);

    // Center-axis texts: add text-anchor middle, center on middle arrow x=446
    // Direct outs + post fader (multiline — center both tspans)
    adapted = adapted.replace("translate(395.56 399)\"><tspan x=\"0\"",
                               "translate(446 399)\"><tspan x=\"0\" text-anchor=\"middle\"");
    adapted = adapted.replace("x=\"3.52\" y=\"28.8\">post fader",
                               "x=\"0\" y=\"28.8\" text-anchor=\"middle\">post fader");
    // Outputs
    adapted = adapted.replace("translate(406.41 635)\"><tspan x=\"0\"",
                               "translate(446 635)\"><tspan x=\"0\" text-anchor=\"middle\"");
    // Inputs
    adapted = adapted.replace("translate(415.35 542)\"><tspan x=\"0\"",
                               "translate(446 542)\"><tspan x=\"0\" text-anchor=\"middle\"");
    // Speaker arrays
    adapted = adapted.replace("translate(372.24 921.07)\"><tspan x=\"0\"",
                               "translate(446 921.07)\"><tspan x=\"0\" text-anchor=\"middle\"");

    // Font size: shrink from 24px to 20px so Console fits
    adapted = adapted.replace("font-size: 24px", "font-size: 20px");

    // WFS processor: larger (26px) and centered in box (center x=445, center y=588)
    adapted = adapted.replace("translate(373.07 592)\"><tspan x=\"0\"",
                               "translate(445 590)\"><tspan x=\"0\" text-anchor=\"middle\" font-size=\"26\"");

    // Theme-specific color adjustments
    bool isDark = ColorScheme::get().background.getBrightness() < 0.5f;
    if (isDark)
    {
        // Lighten blue text/strokes for dark backgrounds
        adapted = adapted.replace("fill: blue", "fill: #6699ff");
        adapted = adapted.replace("stroke: blue", "stroke: #6699ff");

        // Black strokes/fills → light
        adapted = adapted.replace("stroke: #000", "stroke: #ddd");
        adapted = adapted.replace("fill: #000", "fill: #ddd");

        // st16 texts: add explicit light fill (Console stays dark — on white rect)
        adapted = adapted.replace("translate(60.24 131.6)\">",  "translate(60.24 131.6)\" fill=\"#ddd\">")   // QLab/Ableton
                         .replace("translate(169.23 738.6)\">", "translate(169.23 738.6)\" fill=\"#ddd\">")  // Remote
                         .replace("translate(446 921.07)\">", "translate(446 921.07)\" fill=\"#ddd\">");     // Speaker arrays

        // Classless polygons (arrowheads) → light
        adapted = adapted.replace("<polygon points=", "<polygon fill=\"#ddd\" points=");

        // Dark boxes (#333) → slightly lighter for contrast
        adapted = adapted.replace("fill: #333", "fill: #444");
    }
    else
    {
        // Light mode: explicit colors for visibility on light background
        // st16 texts (no fill defined) → force black
        adapted = adapted.replace("translate(60.24 131.6)\">",  "translate(60.24 131.6)\" fill=\"#000\">")   // QLab/Ableton
                         .replace("translate(169.23 738.6)\">", "translate(169.23 738.6)\" fill=\"#000\">")  // Remote
                         .replace("translate(446 921.07)\">", "translate(446 921.07)\" fill=\"#000\">");     // Speaker arrays

        // Ensure lines and strokes are dark
        adapted = adapted.replace("stroke: #000", "stroke: #222");

        // Keep blue as-is (visible on light), keep red as-is
        // WFS processor white text on dark box — keep as designed
    }

    auto xml = juce::XmlDocument::parse(adapted);
    if (xml == nullptr) return nullptr;
    return juce::Drawable::createFromSVG(*xml);
}

inline const char* parallax1SVG = R"SVG(<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 860 483.75">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0 {
        stroke-dasharray: 8.21 8.21;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6, .st7, .st8, .st9, .st10, .st11, .st12, .st13, .st14, .st15, .st16, .st17, .st18, .st19 {
        stroke-miterlimit: 10;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6, .st8, .st9, .st10, .st11, .st12, .st13, .st14, .st15, .st16, .st17, .st18, .st19 {
        fill: none;
      }

      .st0, .st1, .st2, .st3, .st5, .st6, .st9, .st10, .st11, .st12, .st14, .st15, .st16, .st18, .st19 {
        stroke-width: 3px;
      }

      .st0, .st1, .st2, .st12, .st15, .st18 {
        stroke: red;
      }

      .st1 {
        stroke-dasharray: 7.93 7.93;
      }

      .st2 {
        stroke-dasharray: 8.27 8.27;
      }

      .st3 {
        stroke-dasharray: 8.26 8.26;
      }

      .st3, .st4, .st5, .st6, .st9, .st11, .st13, .st14, .st17, .st19 {
        stroke: blue;
      }

      .st20 {
        fill: #fff;
      }

      .st5 {
        stroke-dasharray: 7.7 7.7;
      }

      .st21 {
        fill: url(#radial-gradient);
      }

      .st6 {
        stroke-dasharray: 7.75 7.75;
      }

      .st7 {
        fill: #ccc;
      }

      .st7, .st8, .st10, .st16 {
        stroke: #000;
      }

      .st8 {
        stroke-width: 2px;
      }

      .st9 {
        stroke-dasharray: 8.33 8.33;
      }

      .st10 {
        stroke-dasharray: 7.9 7.9;
      }

      .st11 {
        stroke-dasharray: 8;
      }

      .st12 {
        stroke-dasharray: 8.2 8.2;
      }

      .st13 {
        stroke-width: .75px;
      }

      .st15 {
        stroke-dasharray: 7.82 7.82;
      }

      .st17 {
        stroke-width: .5px;
      }

      .st19 {
        stroke-dasharray: 8.23 8.23;
      }
    </style>
    <radialGradient id="radial-gradient" cx="478.59" cy="44.58" fx="478.59" fy="44.58" r="14.17" gradientUnits="userSpaceOnUse">
      <stop offset=".47" stop-color="#fff"/>
      <stop offset="1" stop-color="blue"/>
    </radialGradient>
  </defs>
  <g>
    <rect class="st7" x="256.43" y="228.45" width="20.36" height="20.36" transform="translate(156.67 -101.87) rotate(30.28)"/>
    <polygon class="st7" points="268.78 251.6 265.75 240.09 254.25 243.12 268.78 251.6"/>
  </g>
  <g>
    <rect class="st7" x="336.38" y="228.57" width="20.36" height="20.36"/>
    <polygon class="st7" points="354.97 248.85 346.56 240.44 338.15 248.85 354.97 248.85"/>
  </g>
  <g>
    <rect class="st7" x="416.38" y="228.57" width="20.36" height="20.36"/>
    <polygon class="st7" points="434.97 248.85 426.56 240.44 418.15 248.85 434.97 248.85"/>
  </g>
  <g>
    <rect class="st7" x="496.38" y="228.57" width="20.36" height="20.36"/>
    <polygon class="st7" points="514.97 248.85 506.56 240.44 498.15 248.85 514.97 248.85"/>
  </g>
  <g>
    <rect class="st7" x="576.4" y="228.7" width="20.36" height="20.36" transform="translate(-37.39 347.53) rotate(-32.04)"/>
    <polygon class="st7" points="599.07 242.99 587.48 240.32 584.81 251.91 599.07 242.99"/>
  </g>
  <g>
    <line class="st16" x1="259.24" y1="251.74" x2="257.22" y2="255.19"/>
    <line class="st10" x1="253.24" y1="262.02" x2="175.52" y2="395.14"/>
    <line class="st16" x1="173.53" y1="398.55" x2="171.51" y2="402"/>
  </g>
  <g>
    <line class="st16" x1="346.8" y1="254.28" x2="346.8" y2="258.28"/>
    <line class="st10" x1="346.8" y1="266.19" x2="346.8" y2="420.33"/>
    <line class="st16" x1="346.8" y1="424.28" x2="346.8" y2="428.28"/>
  </g>
  <g>
    <line class="st16" x1="426.8" y1="254.78" x2="426.8" y2="258.78"/>
    <line class="st10" x1="426.8" y1="266.69" x2="426.8" y2="420.83"/>
    <line class="st16" x1="426.8" y1="424.78" x2="426.8" y2="428.78"/>
  </g>
  <g>
    <line class="st16" x1="506.8" y1="255.28" x2="506.8" y2="259.28"/>
    <line class="st10" x1="506.8" y1="267.19" x2="506.8" y2="421.33"/>
    <line class="st16" x1="506.8" y1="425.28" x2="506.8" y2="429.28"/>
  </g>
  <g>
    <line class="st16" x1="595.82" y1="253.19" x2="597.94" y2="256.58"/>
    <line class="st10" x1="602.13" y1="263.28" x2="683.9" y2="393.95"/>
    <line class="st16" x1="685.99" y1="397.3" x2="688.12" y2="400.7"/>
  </g>
  <g>
    <line class="st18" x1="453.27" y1="239.8" x2="453.79" y2="235.83"/>
    <line class="st12" x1="454.86" y1="227.7" x2="477.75" y2="52.82"/>
    <line class="st18" x1="478.28" y1="48.75" x2="478.8" y2="44.78"/>
  </g>
  <g>
    <line class="st14" x1="427.8" y1="438.78" x2="428.31" y2="434.81"/>
    <line class="st5" x1="429.29" y1="427.17" x2="452.27" y2="247.58"/>
    <line class="st14" x1="452.76" y1="243.76" x2="453.27" y2="239.8"/>
  </g>
  <g>
    <line class="st18" x1="492.26" y1="241.24" x2="491.99" y2="237.25"/>
    <line class="st0" x1="491.43" y1="229.05" x2="479.35" y2="52.87"/>
    <line class="st18" x1="479.07" y1="48.77" x2="478.8" y2="44.78"/>
  </g>
  <g>
    <line class="st14" x1="505.8" y1="438.78" x2="505.53" y2="434.79"/>
    <line class="st3" x1="504.96" y1="426.55" x2="492.82" y2="249.35"/>
    <line class="st14" x1="492.54" y1="245.23" x2="492.26" y2="241.24"/>
  </g>
  <g>
    <line class="st18" x1="412.3" y1="248.92" x2="413.54" y2="245.12"/>
    <line class="st2" x1="416.1" y1="237.26" x2="476.28" y2="52.52"/>
    <line class="st18" x1="477.56" y1="48.59" x2="478.8" y2="44.78"/>
  </g>
  <g>
    <line class="st14" x1="349.8" y1="440.78" x2="351.04" y2="436.98"/>
    <line class="st6" x1="353.44" y1="429.61" x2="409.86" y2="256.41"/>
    <line class="st14" x1="411.06" y1="252.73" x2="412.3" y2="248.92"/>
  </g>
  <g>
    <line class="st18" x1="591.94" y1="235.89" x2="589.9" y2="232.45"/>
    <line class="st1" x1="585.86" y1="225.63" x2="482.86" y2="51.64"/>
    <line class="st18" x1="480.84" y1="48.22" x2="478.8" y2="44.78"/>
  </g>
  <g>
    <line class="st14" x1="700.8" y1="419.78" x2="698.76" y2="416.34"/>
    <line class="st19" x1="694.57" y1="409.26" x2="596.07" y2="242.87"/>
    <line class="st14" x1="593.97" y1="239.33" x2="591.94" y2="235.89"/>
  </g>
  <g>
    <line class="st18" x1="296.77" y1="259.59" x2="299.35" y2="256.54"/>
    <line class="st15" x1="304.4" y1="250.58" x2="473.69" y2="50.82"/>
    <line class="st18" x1="476.21" y1="47.83" x2="478.8" y2="44.78"/>
  </g>
  <g>
    <line class="st14" x1="167.8" y1="411.78" x2="170.39" y2="408.73"/>
    <line class="st9" x1="175.77" y1="402.38" x2="291.49" y2="265.82"/>
    <line class="st14" x1="294.18" y1="262.65" x2="296.77" y2="259.59"/>
  </g>
  <path class="st11" d="M296.51,259.59c-6.1-5.22-12.51-10.1-19.18-14.61"/>
  <path class="st11" d="M411.31,248.77c-17.33-5.53-35.63-8.89-54.57-9.76"/>
  <path class="st11" d="M453.18,240.53c-5.42-.7-10.9-1.19-16.44-1.47"/>
  <path class="st11" d="M496.92,241.01c-.66.03-1.32.06-1.98.1-.71.04-1.43.08-2.14.13"/>
  <g>
    <circle class="st21" cx="478.59" cy="44.58" r="14.17"/>
    <circle class="st4" cx="478.59" cy="44.58" r="19.32"/>
    <circle class="st13" cx="478.59" cy="44.58" r="24.53"/>
    <circle class="st17" cx="478.59" cy="44.58" r="27"/>
  </g>
  <g>
    <g>
      <path class="st20" d="M700.86,434.04c-4.52,0-8.64-2.28-11.03-6.11-3.24-5.18-2.41-12.06,1.89-16.27l-1.39-5.65c-.1-.42.07-.86.44-1.09.16-.1.35-.15.53-.15.23,0,.46.08.64.23l4.47,3.72c1.39-.5,2.84-.75,4.33-.75,4.55,0,8.71,2.3,11.13,6.16,3.8,6.08,1.95,14.11-4.12,17.92-2.07,1.3-4.45,1.98-6.88,1.98Z"/>
      <path d="M691.3,405.78l4.92,4.1c1.46-.6,2.99-.9,4.52-.9,4.03,0,7.99,2.04,10.28,5.69,3.52,5.62,1.81,13.02-3.81,16.54-1.98,1.24-4.18,1.83-6.35,1.83-4,0-7.91-2-10.18-5.64-3.15-5.04-2.15-11.59,2.16-15.41l-1.53-6.22M691.3,403.78c-.37,0-.73.1-1.06.3-.73.46-1.09,1.33-.88,2.17l1.26,5.11c-4.27,4.56-5.02,11.69-1.64,17.1,2.58,4.12,7.02,6.58,11.88,6.58,2.62,0,5.18-.74,7.41-2.13,6.54-4.09,8.54-12.75,4.44-19.29-2.6-4.15-7.08-6.63-11.98-6.63-1.41,0-2.79.21-4.12.63l-4.04-3.36c-.37-.31-.82-.46-1.28-.46h0Z"/>
    </g>
    <path class="st8" d="M688.03,426.64s2.8,2.04,1.86,4.38"/>
    <path class="st8" d="M711.49,411.96s.61,3.41,3.12,3.59"/>
  </g>
  <g>
    <g>
      <path class="st20" d="M346.8,465.28c-7.17,0-13-5.83-13-13,0-6.11,4.36-11.5,10.23-12.79l1.82-5.53c.14-.41.52-.69.95-.69s.81.28.95.69l1.82,5.53c5.88,1.28,10.23,6.67,10.23,12.79,0,7.17-5.83,13-13,13Z"/>
      <path d="M346.8,434.28l2,6.08c5.68.95,10,5.97,10,11.92,0,6.63-5.37,12-12,12s-12-5.37-12-12c0-5.95,4.32-10.96,10-11.92l2-6.08M346.8,432.28c-.86,0-1.63.55-1.9,1.38l-1.64,5c-6.04,1.6-10.46,7.25-10.46,13.63,0,7.72,6.28,14,14,14s14-6.28,14-14c0-6.38-4.42-12.03-10.46-13.63l-1.64-5c-.27-.82-1.04-1.38-1.9-1.38h0Z"/>
    </g>
    <path class="st8" d="M332.96,450.23s1.3,3.22-.75,4.7"/>
    <path class="st8" d="M360.64,450.23s-1.3,3.22.75,4.7"/>
  </g>
  <g>
    <g>
      <path class="st20" d="M426.8,465.78c-7.17,0-13-5.83-13-13,0-6.11,4.36-11.5,10.23-12.79l1.82-5.53c.14-.41.52-.69.95-.69s.81.28.95.69l1.82,5.53c5.88,1.28,10.23,6.67,10.23,12.79,0,7.17-5.83,13-13,13Z"/>
      <path d="M426.8,434.78l2,6.08c5.68.95,10,5.97,10,11.92,0,6.63-5.37,12-12,12s-12-5.37-12-12c0-5.95,4.32-10.96,10-11.92l2-6.08M426.8,432.78c-.86,0-1.63.55-1.9,1.38l-1.64,5c-6.04,1.6-10.46,7.25-10.46,13.63,0,7.72,6.28,14,14,14s14-6.28,14-14c0-6.38-4.42-12.03-10.46-13.63l-1.64-5c-.27-.82-1.04-1.38-1.9-1.38h0Z"/>
    </g>
    <path class="st8" d="M412.96,450.73s1.3,3.22-.75,4.7"/>
    <path class="st8" d="M440.64,450.73s-1.3,3.22.75,4.7"/>
  </g>
  <g>
    <g>
      <path class="st20" d="M506.8,466.28c-7.17,0-13-5.83-13-13,0-6.11,4.36-11.5,10.23-12.79l1.82-5.53c.13-.41.52-.69.95-.69s.82.28.95.69l1.82,5.53c5.88,1.28,10.23,6.67,10.23,12.79,0,7.17-5.83,13-13,13Z"/>
      <path d="M506.8,435.28l2,6.08c5.68.95,10,5.97,10,11.92,0,6.63-5.37,12-12,12s-12-5.37-12-12c0-5.95,4.32-10.96,10-11.92l2-6.08M506.8,433.28c-.86,0-1.63.55-1.9,1.38l-1.64,5c-6.04,1.6-10.46,7.25-10.46,13.63,0,7.72,6.28,14,14,14s14-6.28,14-14c0-6.38-4.42-12.03-10.46-13.63l-1.64-5c-.27-.82-1.04-1.38-1.9-1.38h0Z"/>
    </g>
    <path class="st8" d="M492.96,451.23s1.3,3.22-.75,4.7"/>
    <path class="st8" d="M520.64,451.23s-1.3,3.22.75,4.7"/>
  </g>
  <g>
    <g>
      <path class="st20" d="M159.4,435.73c-2.29,0-4.55-.61-6.54-1.78-6.19-3.61-8.29-11.59-4.67-17.78,2.34-4.01,6.69-6.51,11.33-6.51,1.35,0,2.68.21,3.95.62l4.35-3.86c.19-.17.42-.25.66-.25.17,0,.35.04.5.14.37.22.56.65.47,1.07l-1.22,5.69c4.43,4.07,5.47,10.92,2.39,16.2-2.32,3.98-6.63,6.45-11.24,6.45Z"/>
      <path d="M168.49,407.19h0M168.49,407.19l-1.34,6.26c4.42,3.68,5.63,10.2,2.63,15.33-2.23,3.82-6.25,5.95-10.37,5.95-2.05,0-4.14-.53-6.04-1.64-5.72-3.34-7.65-10.69-4.31-16.41,2.24-3.84,6.31-6.01,10.47-6.01,1.4,0,2.82.25,4.18.76l4.79-4.24M168.49,405.19s-.03,0-.05,0c-.49.01-.94.2-1.28.51l-3.93,3.48c-1.2-.34-2.44-.51-3.71-.51-5,0-9.67,2.68-12.2,7-1.89,3.23-2.4,7-1.45,10.62.95,3.62,3.25,6.65,6.48,8.53,2.14,1.25,4.58,1.91,7.05,1.91,4.96,0,9.6-2.66,12.1-6.94,3.22-5.51,2.25-12.62-2.16-17.04l1.08-5.04c.04-.17.07-.34.07-.52,0-1.1-.89-2-1.99-2h-.01ZM168.49,409.19h0,0Z"/>
    </g>
    <path class="st8" d="M148.5,413.98s-.5,3.43-3.01,3.68"/>
    <path class="st8" d="M172.4,427.94s-2.74,2.12-1.72,4.43"/>
  </g>
</svg>)SVG";

inline const char* parallax2SVG = R"SVG(<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 860 483.75">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0, .st1 {
        fill: #fff;
      }

      .st2, .st3, .st4, .st1, .st5, .st6, .st7, .st8, .st9, .st10 {
        stroke-miterlimit: 10;
      }

      .st2, .st4, .st5, .st6, .st7, .st8, .st9, .st10 {
        fill: none;
      }

      .st2, .st6, .st9, .st10 {
        stroke: blue;
      }

      .st11 {
        fill: url(#radial-gradient);
      }

      .st3 {
        fill: #ccc;
      }

      .st3, .st4, .st1, .st5, .st7 {
        stroke: #000;
      }

      .st4, .st6, .st8 {
        stroke-dasharray: 8;
        stroke-width: 3px;
      }

      .st1, .st5 {
        stroke-width: 2px;
      }

      .st7 {
        stroke-width: 4px;
      }

      .st8 {
        stroke: red;
      }

      .st9 {
        stroke-width: .75px;
      }

      .st10 {
        stroke-width: .5px;
      }
    </style>
    <radialGradient id="radial-gradient" cx="192.55" cy="277.75" fx="192.55" fy="277.75" r="14.17" gradientUnits="userSpaceOnUse">
      <stop offset=".47" stop-color="#fff"/>
      <stop offset="1" stop-color="blue"/>
    </radialGradient>
  </defs>
  <g>
    <rect class="st3" x="390.59" y="89.84" width="20.36" height="20.36" transform="translate(139.04 419.57) rotate(-64.69)"/>
    <polygon class="st3" points="413.51 96.73 402.31 100.74 406.32 111.94 413.51 96.73"/>
  </g>
  <g>
    <rect class="st3" x="390.54" y="377.95" width="20.36" height="20.36" transform="translate(12.59 788.85) rotate(-90)"/>
    <polygon class="st3" points="410.83 379.72 402.42 388.13 410.83 396.54 410.83 379.72"/>
  </g>
  <g>
    <path class="st1" d="M512.35,327.67c0,8.62,4.84,10.8,10.8,10.8s9.56-3.35,10.8-12-4.84-15.6-10.8-15.6c-4.1,0-7.66,3.3-9.49,8.16-.42,1.11-3.48,6.19-3.71,7.44s2.4-.15,2.4,1.2Z"/>
    <path class="st5" d="M524.68,320.35c.13.53,2.17,2.3.42,4.9"/>
  </g>
  <g>
    <g>
      <path class="st0" d="M669.15,255.47c-6.78,0-9.8-3.02-9.8-9.8,0-.39-.14-1.65-1.96-1.65-.08,0-.15,0-.23,0,.43-.99,1.38-2.73,2.01-3.88.77-1.41,1.25-2.3,1.43-2.77,1.75-4.63,5.02-7.51,8.56-7.51,2.32,0,4.75,1.26,6.66,3.46,2.58,2.97,3.73,6.98,3.15,11-1.12,7.81-4.05,11.14-9.81,11.14Z"/>
      <path d="M669.15,230.87c2.03,0,4.18,1.14,5.9,3.12,2.39,2.75,3.45,6.47,2.92,10.2-1.29,9.04-5,10.28-8.82,10.28-6.25,0-8.8-2.55-8.8-8.8,0-.77-.39-1.97-1.69-2.44.45-.88,1.01-1.9,1.39-2.6.81-1.49,1.28-2.36,1.48-2.9,1.59-4.23,4.51-6.86,7.62-6.86M669.15,228.87c-4.1,0-7.66,3.3-9.49,8.16-.42,1.11-3.48,6.19-3.71,7.44-.09.49.27.57.73.57.22,0,.47-.02.71-.02.51,0,.96.09.96.65,0,8.62,4.84,10.8,10.8,10.8s9.56-3.35,10.8-12c1.2-8.4-4.84-15.6-10.8-15.6h0Z"/>
    </g>
    <path class="st5" d="M671.11,243.25c1.74-2.6-.29-4.37-.42-4.9"/>
  </g>
  <line class="st4" x1="410.96" y1="104.17" x2="657.96" y2="238.17"/>
  <line class="st6" x1="385.95" y1="304.71" x2="506.96" y2="323.17"/>
  <polyline class="st8" points="365.47 263.45 192.96 277.17 385.95 304.71"/>
  <line class="st6" x1="655.96" y1="242.17" x2="365.47" y2="263.45"/>
  <line class="st4" x1="413.96" y1="381.17" x2="511.96" y2="330.17"/>
  <path class="st6" d="M387.58,304.17c-1.06,6.84-1.61,13.86-1.61,21,0,18.11,3.54,35.39,9.96,51.19"/>
  <path class="st6" d="M366.6,263.96c-.42-6.54-.63-13.14-.63-19.79,0-47.21,10.76-91.91,29.97-131.77"/>
  <g>
    <circle class="st11" cx="192.55" cy="277.75" r="14.17"/>
    <g>
      <circle class="st2" cx="192.55" cy="277.75" r="19.32"/>
      <circle class="st9" cx="192.55" cy="277.75" r="24.53"/>
      <circle class="st10" cx="192.55" cy="277.75" r="27"/>
    </g>
  </g>
  <line class="st4" x1="413.96" y1="388.17" x2="545.96" y2="388.17"/>
  <line class="st4" x1="413.96" y1="104.58" x2="692.46" y2="104.58"/>
  <g>
    <line class="st7" x1="684.96" y1="226.82" x2="684.96" y2="104.58"/>
    <polygon points="694.94 223.9 684.96 241.17 674.99 223.9 694.94 223.9"/>
  </g>
  <g>
    <line class="st7" x1="542.96" y1="388.17" x2="542.96" y2="339.02"/>
    <polygon points="552.94 341.94 542.96 324.67 532.99 341.94 552.94 341.94"/>
  </g>
</svg>)SVG";

inline juce::String get_tuning123SVG()
{
    static const char* parts[] = {
        R"SVG(<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="0 0 1000 186">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0 {
        stroke-width: 1.69px;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6, .st7, .st8, .st9, .st10, .st11, .st12 {
        stroke-miterlimit: 10;
      }

      .st0, .st13 {
        fill: #fff;
      }

      .st0, .st6, .st9, .st10 {
        stroke: #000;
      }

      .st1 {
        stroke-dasharray: 2.97 2.97;
      }

      .st1, .st2, .st3, .st4, .st5, .st7, .st8, .st9, .st10, .st11, .st12 {
        fill: none;
      }

      .st1, .st2, .st5, .st7, .st8, .st12 {
        stroke-width: 1.5px;
      }

      .st1, .st2, .st5, .st12 {
        stroke: blue;
      }

      .st3 {
        stroke-dasharray: .76 .76;
      }

      .st3, .st4, .st11 {
        stroke: #666;
      }

      .st3, .st9, .st11 {
        stroke-width: 3.75px;
      }

      .st14 {
        fill: blue;
      }

      .st4 {
        stroke-width: 3px;
      }

      .st15 {
        fill: #666;
      }

      .st5 {
        stroke-dasharray: 2.97 2.97;
      }

      .st6 {
        fill: #ccc;
        stroke-width: 1.25px;
      }

      .st7 {
        stroke: #009245;
      }

      .st7, .st8 {
        stroke-dasharray: 3;
      }

      .st8 {
        stroke: red;
      }

      .st10 {
        stroke-width: .56px;
      }

      .st12 {
        stroke-dasharray: 2.98 2.98;
      }
    </style>
  </defs>
  <g>
    <g>
      <rect class="st6" x="408.78" y="170.08" width="7.63" height="7.63" transform="translate(238.7 586.5) rotate(-90)"/>
      <polygon class="st6" points="416.39 170.74 413.24 173.9 416.39 177.05 416.39 170.74"/>
    </g>
    <g>
      <g>
        <path class="st13" d="M636.85,75.27c-3.67,0-5.23-1.56-5.23-5.23,0-.11-.03-1.04-1.08-1.19.27-.56.68-1.3.94-1.78.45-.83.71-1.31.82-1.59.94-2.49,2.68-4.04,4.55-4.04,1.22,0,2.51.67,3.53,1.85,1.4,1.61,2.02,3.78,1.71,5.96-.76,5.31-3.07,6.03-5.24,6.03Z"/>
        <path d="M636.85,62.27c.97,0,2.05.58,2.89,1.56,1.22,1.4,1.78,3.38,1.51,5.29-.7,4.89-2.64,5.3-4.4,5.3-3.2,0-4.39-1.19-4.39-4.39,0-.59-.25-1.2-.76-1.6.19-.35.38-.7.52-.97.48-.89.74-1.36.87-1.7.81-2.16,2.25-3.5,3.76-3.5M636.85,60.59c-2.31,0-4.31,1.86-5.34,4.59-.23.62-1.96,3.48-2.08,4.19-.05.28.15.32.41.32.13,0,.27-.01.4-.01.29,0,.54.05.54.36,0,4.85,2.72,6.08,6.07,6.08s5.38-1.89,6.08-6.75c.67-4.72-2.72-8.77-6.08-8.77h0Z"/>
      </g>
      <path class="st10" d="M637.95,68.68c.98-1.46-.16-2.46-.24-2.76"/>
    </g>
    <line class="st8" x1="422.07" y1="17.54" x2="632.44" y2="66.66"/>
    <circle class="st14" cx="345.79" cy="129.51" r="5.31"/>
    <polyline class="st9" points="668.07 79.79 461.07 181.04 332.07 181.04"/>
    <g>
      <line class="st2" x1="626.44" y1="69.29" x2="624.97" y2="69.6"/>
      <line class="st12" x1="622.05" y1="70.22" x2="354.9" y2="126.96"/>
      <line class="st2" x1="353.44" y1="127.27" x2="351.98" y2="127.58"/>
    </g>
    <g>
      <rect class="st6" x="414.41" y="11.83" width="7.63" height="7.63" transform="translate(238.92 393.81) rotate(-66.78)"/>
      <polygon class="st6" points="422.95 14.24 418.81 15.9 420.46 20.04 422.95 14.24"/>
    </g>
    <g>
      <line class="st4" x1="440.82" y1="154.7" x2="427.31" y2="178.5"/>
      <line class="st4" x1="422.16" y1="159.84" x2="445.97" y2="173.36"/>
    </g>
  </g>
  <g>
    <g>
      <rect class="st6" x="76.78" y="170.08" width="7.63" height="7.63" transform="translate(-93.3 254.5) rotate(-90)"/>
      <polygon class="st6" points="84.39 170.74 81.24 173.9 84.39 177.05 84.39 170.74"/>
    </g>
    <line class="st7" x1="85.57" y1="172.04" x2="133.57" y2="152.91"/>
    <circle class="st14" cx="13.79" cy="129.51" r="5.31"/>
    <polyline class="st9" points="336.07 79.79 129.07 181.04 .07 181.04"/>
    <g>
      <line class="st2" x1="131.13" y1="150.85" x2="129.65" y2="150.59"/>
      <line class="st5" x1="126.73" y1="150.08" x2="22.93" y2="131.88"/>
      <line class="st2" x1="21.47" y1="131.63" x2="19.99" y2="131.37"/>
    </g>
    <g>
      <rect class="st6" x="82.41" y="11.83" width="7.63" height="7.63" transform="translate(37.84 88.72) rotate(-66.78)"/>
      <polygon class="st6" points="90.95 14.24 86.81 15.9 88.46 20.04 90.95 14.24"/>
    </g>
    <g>
      <line class="st4" x1="100.98" y1="11.2" x2="107.65" y2="37.75"/>
      <line class="st4" x1="91.04" y1="27.81" x2="117.59" y2="21.14"/>
    </g>
    <g>
      <path class="st0" d="M131.9,152.54c0,4.85,2.72,6.08,6.07,6.08s5.38-1.89,6.07-6.75-2.72-8.77-6.07-8.77c-2.31,0-4.31,1.86-5.34,4.59-.23.62-1.96,3.48-2.08,4.19s1.35-.08,1.35.67Z"/>
      <path class="st10" d="M138.83,148.42c.08.3,1.22,1.29.24,2.76"/>
    </g>
  </g>
  <g>
    <g>
      <rect class="st6" x="740.78" y="170.08" width="7.63" height="7.63" transform="translate(570.7 918.5) rotate(-90)"/>
      <polygon class="st6" points="748.39 170.74 745.24 173.9 748.39 177.05 748.39 170.74"/>
    </g>
    <g>
      <g>
        <path class="st13" d="M863.85,124.02c-3.67,0-5.23-1.56-5.23-5.23,0-.11-.03-1.04-1.08-1.19.27-.56.68-1.3.94-1.78.45-.83.71-1.31.82-1.59.94-2.49,2.68-4.04,4.55-4.04,1.22,0,2.51.67,3.53,1.85,1.4,1.61,2.02,3.78,1.71,5.96-.76,5.31-3.07,6.03-5.24,6.03Z"/>
        <path d="M863.85,111.02c.97,0,2.05.58,2.89,1.56,1.22,1.4,1.78,3.38,1.51,5.29-.7,4.89-2.64,5.3-4.4,5.3-3.2,0-4.39-1.19-4.39-4.39,0-.59-.25-1.2-.76-1.6.19-.35.38-.7.52-.97.48-.89.74-1.36.87-1.7.81-2.16,2.25-3.5,3.76-3.5M863.85,109.34c-2.31,0-4.31,1.86-5.34,4.59-.23.62-1.96,3.48-2.08,4.19-.05.28.15.32.41.32.13,0,.27-.01.4-.01.29,0,.54.05.54.36,0,4.85,2.72,6.07,6.08,6.07s5.38-1.89,6.08-6.75c.67-4.72-2.72-8.77-6.08-8.77h0Z"/>
      </g>
      <path class="st10" d="M864.95,117.43c.98-1.46-.16-2.46-.24-2.76"/>
    </g>
    <line class="st8" x1="754.07" y1="17.54" x2="859.44" y2="115.41"/>
    <line class="st7" x1="749.57" y1="173.54" x2="857.94" y2="119.54"/>
    <circle class="st14" cx="677.79" cy="129.51" r="5.31"/>
    <polyline class="st9" points="1000.07 79.79 793.07 181.04 664.07 181.04"/>
    <g>
      <line class="st2" x1="684.14" y1="128.94" x2="685.64" y2="128.84"/>
      <line class="st1" x1="688.6" y1="128.62" x2="853.09" y2="116.75"/>
      <line class="st2" x1="854.57" y1="116.65" x2="856.07" y2="116.54"/>
    </g>
    <g>
      <line class="st11" x1="852.77" y1="135.4" x2="853.11" y2="135.23"/>
      <line class="st3" x1="853.79" y1="134.89" x2="886.06" y2="118.89"/>
      <line class="st11" x1="886.4" y1="118.72" x2="886.73" y2="118.55"/>
      <path class="st15" d="M846.69,138.41c3.39-.36,7.84-.31,10.99.52l-4.11-3.93-.64-5.65c-1.25,3.01-3.9,6.58-6.24,9.06Z"/>
      <path class="st15" d="M892.82,115.54c-2.34,2.48-4.99,6.05-6.24,9.07l-.64-5.65-4.11-3.93c3.15.83,7.6.88,10.99.52Z"/>
    </g>
    <g>
      <rect class="st6" x="746.41" y="11.83" width="7.63" height="7.63" transform="translate(440.01 698.91) rotate(-66.78)"/>
      <polygon class="st6" points="754.95 14.24 750.81 15.9 752.46 20.04 754.95 14.24"/>
    </g>
  </g>
  <metadata><?xpacket begin="﻿" id="W5M0MpCehiHzreSzNTczkc9d"?>
<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP Core 9.1-c003 1.000000, 0000/00/00-00:00:00        ">
   <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
      <rdf:Description rdf:about=""
            xmlns:dc="http://purl.org/dc/elements/1.1/"
            xmlns:xmp="http://ns.adobe.com/xap/1.0/"
            xmlns:xmpGImg="http://ns.adobe.com/xap/1.0/g/img/"
            xmlns:xmpMM="http://ns.adobe.com/xap/1.0/mm/"
            xmlns:stRef="http://ns.adobe.com/xap/1.0/sType/ResourceRef#"
            xmlns:stEvt="http://ns.adobe.com/xap/1.0/sType/ResourceEvent#"
            xmlns:illustrator="http://ns.adobe.com/illustrator/1.0/"
            xmlns:xmpTPg="http://ns.adobe.com/xap/1.0/t/pg/"
            xmlns:stDim="http://ns.adobe.com/xap/1.0/sType/Dimensions#"
            xmlns:xmpG="http://ns.adobe.com/xap/1.0/g/"
            xmlns:pdf="http://ns.adobe.com/pdf/1.3/">
         <dc:format>image/svg+xml</dc:format>
         <dc:title>
            <rdf:Alt>
               <rdf:li xml:lang="x-default">WFS_tuning123</rdf:li>
            </rdf:Alt>
         </dc:title>
         <xmp:CreatorTool>Adobe Illustrator 29.5 (Windows)</xmp:CreatorTool>
         <xmp:CreateDate>2026-03-29T18:18:33+02:00</xmp:CreateDate>
         <xmp:ModifyDate>2026-03-29T18:18:34+02:00</xmp:ModifyDate>
         <xmp:MetadataDate>2026-03-29T18:18:34+02:00</xmp:MetadataDate>
         <xmp:Thumbnails>
            <rdf:Alt>
               <rdf:li rdf:parseType="Resource">
                  <xmpGImg:width>256</xmpGImg:width>
                  <xmpGImg:height>44</xmpGImg:height>
                  <xmpGImg:format>JPEG</xmpGImg:format>
                  <xmpGImg:image>/9j/4AAQSkZJRgABAgEAAAAAAAD/7QAsUGhvdG9zaG9wIDMuMAA4QklNA+0AAAAAABAAAAAAAAEA&#xA;AQAAAAAAAQAB/+4ADkFkb2JlAGTAAAAAAf/bAIQABgQEBAUEBgUFBgkGBQYJCwgGBggLDAoKCwoK&#xA;DBAMDAwMDAwQDA4PEA8ODBMTFBQTExwbGxscHx8fHx8fHx8fHwEHBwcNDA0YEBAYGhURFRofHx8f&#xA;Hx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8f/8AAEQgALAEAAwER&#xA;AAIRAQMRAf/EAaIAAAAHAQEBAQEAAAAAAAAAAAQFAwIGAQAHCAkKCwEAAgIDAQEBAQEAAAAAAAAA&#xA;AQACAwQFBgcICQoLEAACAQMDAgQCBgcDBAIGAnMBAgMRBAAFIRIxQVEGE2EicYEUMpGhBxWxQiPB&#xA;UtHhMxZi8CRygvElQzRTkqKyY3PCNUQnk6OzNhdUZHTD0uIIJoMJChgZhJRFRqS0VtNVKBry4/PE&#xA;1OT0ZXWFlaW1xdXl9WZ2hpamtsbW5vY3R1dnd4eXp7fH1+f3OEhYaHiImKi4yNjo+Ck5SVlpeYmZ&#xA;qbnJ2en5KjpKWmp6ipqqusra6voRAAICAQIDBQUEBQYECAMDbQEAAhEDBCESMUEFURNhIgZxgZEy&#xA;obHwFMHR4SNCFVJicvEzJDRDghaSUyWiY7LCB3PSNeJEgxdUkwgJChgZJjZFGidkdFU38qOzwygp&#xA;0+PzhJSktMTU5PRldYWVpbXF1eX1RlZmdoaWprbG1ub2R1dnd4eXp7fH1+f3OEhYaHiImKi4yNjo&#xA;+DlJWWl5iZmpucnZ6fkqOkpaanqKmqq6ytrq+v/aAAwDAQACEQMRAD8A9U4q7FUr8waFBq9tEkrS&#xA;BraVbiFEkMQeSM1VXcK7KrdCyjkBWmKoPRodfTVr6O7liitvUS6WCKN3BEycSqzuV6PGzEcAd/fF&#xA;WQYq7FXYq7FXYq7FXYq7FXYqlEHlu3g1i81aG6uUub1VWZOSGOiVpRSh33G5qQAANtsBDKMgOlo6&#xA;mopT4ophsCCGiI8TUeoD9wwbsvQe8fb+p3110H7+2ljoKllAlFa0oBGWf/hceLvCfDB5Efd9+32r&#xA;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)SVG",
        R"SVG(f6L&#xA;5nn9T4/337frft+rX7fqfHXrvirIcVdirsVdirsVf//Z</xmpGImg:image>
               </rdf:li>
            </rdf:Alt>
         </xmp:Thumbnails>
         <xmpMM:RenditionClass>default</xmpMM:RenditionClass>
         <xmpMM:OriginalDocumentID>uuid:65E6390686CF11DBA6E2D887CEACB407</xmpMM:OriginalDocumentID>
         <xmpMM:DocumentID>xmp.did:8b3fea68-61ec-314f-b9f2-1064485d2cfd</xmpMM:DocumentID>
         <xmpMM:InstanceID>xmp.iid:8b3fea68-61ec-314f-b9f2-1064485d2cfd</xmpMM:InstanceID>
         <xmpMM:DerivedFrom rdf:parseType="Resource">
            <stRef:instanceID>uuid:a429e487-90d5-4175-9827-17188713bb74</stRef:instanceID>
            <stRef:documentID>xmp.did:d4bc3e88-3d17-be46-b1b1-3c0ad6b0463b</stRef:documentID>
            <stRef:originalDocumentID>uuid:65E6390686CF11DBA6E2D887CEACB407</stRef:originalDocumentID>
            <stRef:renditionClass>default</stRef:renditionClass>
         </xmpMM:DerivedFrom>
         <xmpMM:History>
            <rdf:Seq>
               <rdf:li rdf:parseType="Resource">
                  <stEvt:action>saved</stEvt:action>
                  <stEvt:instanceID>xmp.iid:5b971c06-ab2e-2a48-a83d-8de7f26dfc3e</stEvt:instanceID>
                  <stEvt:when>2019-02-27T21:05:22+01:00</stEvt:when>
                  <stEvt:softwareAgent>Adobe Illustrator CC 22.1 (Windows)</stEvt:softwareAgent>
                  <stEvt:changed>/</stEvt:changed>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <stEvt:action>saved</stEvt:action>
                  <stEvt:instanceID>xmp.iid:8b3fea68-61ec-314f-b9f2-1064485d2cfd</stEvt:instanceID>
                  <stEvt:when>2026-03-29T18:18:34+02:00</stEvt:when>
                  <stEvt:softwareAgent>Adobe Illustrator 29.5 (Windows)</stEvt:softwareAgent>
                  <stEvt:changed>/</stEvt:changed>
               </rdf:li>
            </rdf:Seq>
         </xmpMM:History>
         <illustrator:StartupProfile>Web</illustrator:StartupProfile>
         <illustrator:CreatorSubTool>Adobe Illustrator</illustrator:CreatorSubTool>
         <xmpTPg:NPages>1</xmpTPg:NPages>
         <xmpTPg:HasVisibleTransparency>False</xmpTPg:HasVisibleTransparency>
         <xmpTPg:HasVisibleOverprint>False</xmpTPg:HasVisibleOverprint>
         <xmpTPg:MaxPageSize rdf:parseType="Resource">
            <stDim:w>1000.000000</stDim:w>
            <stDim:h>186.000000</stDim:h>
            <stDim:unit>Pixels</stDim:unit>
         </xmpTPg:MaxPageSize>
         <xmpTPg:PlateNames>
            <rdf:Seq>
               <rdf:li>Cyan</rdf:li>
               <rdf:li>Magenta</rdf:li>
               <rdf:li>Yellow</rdf:li>
               <rdf:li>Black</rdf:li>
            </rdf:Seq>
         </xmpTPg:PlateNames>
         <xmpTPg:SwatchGroups>
            <rdf:Seq>
               <rdf:li rdf:parseType="Resource">
                  <xmpG:groupName>Default Swatch Group</xmpG:groupName>
                  <xmpG:groupType>0</xmpG:groupType>
                  <xmpG:Colorants>
                     <rdf:Seq>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>White</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>Black</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Red</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Yellow</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Green</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Cyan</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Blue</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Magenta</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=193 G=39 B=45</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>193</xmpG:red>
                           <xmpG:green>39</xmpG:green>
                           <xmpG:blue>45</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=237 G=28 B=36</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>237</xmpG:red>
                           <xmpG:green>28</xmpG:green>
                           <xmpG:blue>36</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=241 G=90 B=36</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>241</xmpG:red>
                           <xmpG:green>90</xmpG:green>
                           <xmpG:blue>36</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=247 G=147 B=30</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>247</xmpG:red>
                           <xmpG:green>147</xmpG:green>
                           <xmpG:blue>30</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=251 G=176 B=59</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>251</xmpG:red>
                           <xmpG:green>176</xmpG:green>
                           <xmpG:blue>59</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=252 G=238 B=33</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>252</xmpG:red>
                           <xmpG:green>238</xmpG:green>
                           <xmpG:blue>33</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=217 G=224 B=33</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>217</xmpG:red>
                           <xmpG:green>224</xmpG:green>
                           <xmpG:blue>33</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=140 G=198 B=63</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>140</xmpG:red>
                           <xmpG:green>198</xmpG:green>
                           <xmpG:blue>63</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=57 G=181 B=74</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>57</xmpG:red>
                           <xmpG:green>181</xmpG:green>
                           <xmpG:blue>74</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=146 B=69</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>146</xmpG:green>
                           <xmpG:blue>69</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=104 B=55</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>104</xmpG:green>
                           <xmpG:blue>55</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=34 G=181 B=115</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>34</xmpG:red>
                           <xmpG:green>181</xmpG:green>
                           <xmpG:blue>115</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=169 B=157</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>169</xmpG:green>
                           <xmpG:blue>157</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=41 G=171 B=226</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>41</xmpG:red>
                           <xmpG:green>171</xmpG:green>
                           <xmpG:blue>226</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=113 B=188</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>113</xmpG:green>
                           <xmpG:blue>188</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=46 G=49 B=146</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>46</xmpG:red>
                           <xmpG:green>49</xmpG:green>
                           <xmpG:blue>146</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=27 G=20 B=100</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>27</xmpG:red>
                           <xmpG:green>20</xmpG:green>
                           <xmpG:blue>100</xmpG:blue>
                     )SVG",
        R"SVG(   </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=102 G=45 B=145</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>102</xmpG:red>
                           <xmpG:green>45</xmpG:green>
                           <xmpG:blue>145</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=147 G=39 B=143</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>147</xmpG:red>
                           <xmpG:green>39</xmpG:green>
                           <xmpG:blue>143</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=158 G=0 B=93</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>158</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>93</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=212 G=20 B=90</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>212</xmpG:red>
                           <xmpG:green>20</xmpG:green>
                           <xmpG:blue>90</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=237 G=30 B=121</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>237</xmpG:red>
                           <xmpG:green>30</xmpG:green>
                           <xmpG:blue>121</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=199 G=178 B=153</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>199</xmpG:red>
                           <xmpG:green>178</xmpG:green>
                           <xmpG:blue>153</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=153 G=134 B=117</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>153</xmpG:red>
                           <xmpG:green>134</xmpG:green>
                           <xmpG:blue>117</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=115 G=99 B=87</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>115</xmpG:red>
                           <xmpG:green>99</xmpG:green>
                           <xmpG:blue>87</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=83 G=71 B=65</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>83</xmpG:red>
                           <xmpG:green>71</xmpG:green>
                           <xmpG:blue>65</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=198 G=156 B=109</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>198</xmpG:red>
                           <xmpG:green>156</xmpG:green>
                           <xmpG:blue>109</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=166 G=124 B=82</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>166</xmpG:red>
                           <xmpG:green>124</xmpG:green>
                           <xmpG:blue>82</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=140 G=98 B=57</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>140</xmpG:red>
                           <xmpG:green>98</xmpG:green>
                           <xmpG:blue>57</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=117 G=76 B=36</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>117</xmpG:red>
                           <xmpG:green>76</xmpG:green>
                           <xmpG:blue>36</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=96 G=56 B=19</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>96</xmpG:red>
                           <xmpG:green>56</xmpG:green>
                           <xmpG:blue>19</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=66 G=33 B=11</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>66</xmpG:red>
                           <xmpG:green>33</xmpG:green>
                           <xmpG:blue>11</xmpG:blue>
                        </rdf:li>
                     </rdf:Seq>
                  </xmpG:Colorants>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <xmpG:groupName>Grays</xmpG:groupName>
                  <xmpG:groupType>1</xmpG:groupType>
                  <xmpG:Colorants>
                     <rdf:Seq>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=0 B=0</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=26 G=26 B=26</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>26</xmpG:red>
                           <xmpG:green>26</xmpG:green>
                           <xmpG:blue>26</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=51 G=51 B=51</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>51</xmpG:red>
                           <xmpG:green>51</xmpG:green>
                           <xmpG:blue>51</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=77 G=77 B=77</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>77</xmpG:red>
                           <xmpG:green>77</xmpG:green>
                           <xmpG:blue>77</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=102 G=102 B=102</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>102</xmpG:red>
                           <xmpG:green>102</xmpG:green>
                           <xmpG:blue>102</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=128 G=128 B=128</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>128</xmpG:red>
                           <xmpG:green>128</xmpG:green>
                           <xmpG:blue>128</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=153 G=153 B=153</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>153</xmpG:red>
                           <xmpG:green>153</xmpG:green>
                           <xmpG:blue>153</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=179 G=179 B=179</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>179</xmpG:red>
                           <xmpG:green>179</xmpG:green>
                           <xmpG:blue>179</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=204 G=204 B=204</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>204</xmpG:red>
                           <xmpG:green>204</xmpG:green>
                           <xmpG:blue>204</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=230 G=230 B=230</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>230</xmpG:red>
                           <xmpG:green>230</xmpG:green>
                           <xmpG:blue>230</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=242 G=242 B=242</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>242</xmpG:red>
                           <xmpG:green>242</xmpG:green>
                           <xmpG:blue>242</xmpG:blue>
                        </rdf:li>
                     </rdf:Seq>
                  </xmpG:Colorants>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <xmpG:groupName>Web Color Group</xmpG:groupName>
                  <xmpG:groupType>1</xmpG:groupType>
                  <xmpG:Colorants>
                     <rdf:Seq>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=63 G=169 B=245</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>63</xmpG:red>
                           <xmpG:green>169</xmpG:green>
                           <xmpG:blue>245</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=122 G=201 B=67</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>122</xmpG:red>
                           <xmpG:green>201</xmpG:green>
                           <xmpG:blue>67</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=255 G=147 B=30</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>147</xmpG:green>
                           <xmpG:blue>30</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=255 G=29 B=37</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>29</xmpG:green>
                           <xmpG:blue>37</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Re)SVG",
        R"SVG(source">
                           <xmpG:swatchName>R=255 G=123 B=172</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>123</xmpG:green>
                           <xmpG:blue>172</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=189 G=204 B=212</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>189</xmpG:red>
                           <xmpG:green>204</xmpG:green>
                           <xmpG:blue>212</xmpG:blue>
                        </rdf:li>
                     </rdf:Seq>
                  </xmpG:Colorants>
               </rdf:li>
            </rdf:Seq>
         </xmpTPg:SwatchGroups>
         <pdf:Producer>Adobe PDF library 17.00</pdf:Producer>
      </rdf:Description>
   </rdf:RDF>
</x:xmpmeta>
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                           
<?xpacket end="w"?>
  </metadata>
</svg>)SVG"
    };
    static juce::String s;
    if (s.isEmpty())
        for (auto* p : parts)
            s += juce::String(juce::CharPointer_UTF8(p));
    return s;
}

inline juce::String get_signalFlowSVG()
{
    static const char* parts[] = {
        R"SVG(<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="0 0 613 928">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0, .st1, .st2, .st3, .st4, .st5 {
        stroke-miterlimit: 10;
      }

      .st0, .st2, .st6, .st7, .st8 {
        stroke: #000;
      }

      .st0, .st8, .st4, .st5 {
        fill: none;
      }

      .st1 {
        stroke: #fff;
      }

      .st1, .st9, .st7, .st10 {
        fill: #fff;
      }

      .st2, .st11 {
        fill: #333;
      }

      .st12, .st3, .st13 {
        fill: blue;
      }

      .st3, .st4 {
        stroke: blue;
      }

      .st3, .st4, .st5 {
        stroke-width: 3px;
      }

      .st14, .st15 {
        fill: red;
      }

      .st13, .st16, .st15, .st10 {
        font-family: Calibri, Calibri;
        font-size: 24px;
      }

      .st6, .st7, .st8 {
        stroke-linejoin: bevel;
      }

      .st5 {
        stroke: red;
      }
    </style>
  </defs>
  <g>
    <rect class="st11" x="345.4" y="147.4" width="204.2" height="228.2"/>
    <path d="M549.2,147.8v227.4h-203.4v-227.4h203.4M550,147h-205v229h205v-229h0Z"/>
  </g>
  <g>
    <rect class="st11" x="345.4" y="247.4" width="204.2" height="128.2"/>
    <path d="M549.2,247.8v127.4h-203.4v-127.4h203.4M550,247h-205v129h205v-129h0Z"/>
  </g>
  <g>
    <rect class="st9" x="407.73" y="162.48" width="92.04" height="63.04"/>
    <path d="M499.29,162.96v62.08h-91.08v-62.08h91.08M500.25,162h-93v64h93v-64h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="393" cy="181" r="4.5"/>
    <path d="M393,177c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M393,176c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="376.5" cy="181" r="4.5"/>
    <path d="M376.5,177c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M376.5,176c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="360" cy="181" r="4.5"/>
    <path d="M360,177c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M360,176c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="384.75" cy="207" r="4.5"/>
    <path d="M384.75,203c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M384.75,202c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="368.25" cy="207" r="4.5"/>
    <path d="M368.25,203c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M368.25,202c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="518" cy="167" r="4.5"/>
    <path d="M518,163c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M518,162c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="518" cy="194" r="4.5"/>
    <path d="M518,190c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M518,189c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="518" cy="221" r="4.5"/>
    <path d="M518,217c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M518,216c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="534" cy="180" r="4.5"/>
    <path d="M534,176c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M534,175c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <circle class="st9" cx="534" cy="207" r="4.5"/>
    <path d="M534,203c2.21,0,4,1.79,4,4s-1.79,4-4,4-4-1.79-4-4,1.79-4,4-4M534,202c-2.76,0-5,2.24-5,5s2.24,5,5,5,5-2.24,5-5-2.24-5-5-5h0Z"/>
  </g>
  <g>
    <g>
      <line class="st1" x1="354.5" y1="273.5" x2="354.5" y2="356.5"/>
      <rect class="st1" x="352" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="371.41" y1="356.5" x2="371.41" y2="273.5"/>
      <rect class="st1" x="368.91" y="294.5" width="5" height="10" transform="translate(742.82 599) rotate(180)"/>
    </g>
    <g>
      <line class="st1" x1="388.32" y1="273.5" x2="388.32" y2="356.5"/>
      <rect class="st1" x="385.82" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="405.23" y1="273.5" x2="405.23" y2="356.5"/>
      <rect class="st1" x="402.73" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="439.05" y1="356.5" x2="439.05" y2="273.5"/>
      <rect class="st1" x="436.55" y="294.5" width="5" height="10" transform="translate(878.09 599) rotate(180)"/>
    </g>
    <g>
      <line class="st1" x1="472.86" y1="273.5" x2="472.86" y2="356.5"/>
      <rect class="st1" x="470.36" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="489.77" y1="356.5" x2="489.77" y2="273.5"/>
      <rect class="st1" x="487.27" y="294.5" width="5" height="10" transform="translate(979.55 599) rotate(180)"/>
    </g>
    <g>
      <line class="st1" x1="506.68" y1="273.5" x2="506.68" y2="356.5"/>
      <rect class="st1" x="504.18" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="523.59" y1="273.5" x2="523.59" y2="356.5"/>
      <rect class="st1" x="521.09" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="540.5" y1="356.5" x2="540.5" y2="273.5"/>
      <rect class="st1" x="538" y="294.5" width="5" height="10" transform="translate(1081 599) rotate(180)"/>
    </g>
    <g>
      <line class="st1" x1="422.14" y1="273.5" x2="422.14" y2="356.5"/>
      <rect class="st1" x="419.64" y="325.5" width="5" height="10"/>
    </g>
    <g>
      <line class="st1" x1="455.95" y1="356.5" x2="455.95" y2="273.5"/>
      <rect class="st1" x="453.45" y="294.5" width="5" height="10" transform="translate(911.91 599) rotate(180)"/>
    </g>
  </g>
  <text class="st16" transform="translate(414.97 202)"><tspan x="0" y="0">Console</tspan></text>
  <g>
    <rect class="st12" x="241.5" y="12.67" width="34" height="9"/>
    <circle class="st12" cx="236" cy="17.17" r="7.5"/>
    <line class="st4" x1="228.5" y1="9.67" x2="228.5" y2="24.67"/>
  </g>
  <g>
    <rect class="st12" x="241.5" y="41.67" width="34" height="9"/>
    <circle class="st12" cx="236" cy="46.17" r="7.5"/>
    <line class="st4" x1="228.5" y1="38.67" x2="228.5" y2="53.67"/>
  </g>
  <text class="st13" transform="translate(78.99 39)"><tspan x="0" y="0">Microphones</tspan></text>
  <text class="st13" transform="translate(95.71 98)"><tspan x="0" y="0">Soundtracks</tspan></text>
  <text class="st15" transform="translate(79.71 165)"><tspan x="0" y="0">OSC commands</tspan></text>
  <g>
    <polyline class="st4" points="298 17 517.5 17 517.5 114.89"/>
    <path class="st12" d="M517.5,133c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <polyline class="st4" points="298 46 481.5 46 481.5 114.89"/>
    <path class="st12" d="M481.5,133c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <polyline class="st4" points="298 71 444.5 71 444.5 114.89"/>
    <path class="st12" d="M444.5,133c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <polyline class="st4" points="298 84.5 407.5 84.5 407.5 114.89"/>
    <path class="st12" d="M407.5,133c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <polyline class="st4" points="298 98 370.99 98 370.99 114.89"/>
    <path class="st12" d="M370.99,133c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="410.25" y1="436" x2="410.25" y2="498.89"/>
    <path class="st12" d="M410.25,517c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="446" y1="436" x2="446" y2="498.89"/>
    <path class="st12" d="M446,517c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="374.5" y1="385" x2="374.5" y2="523.89"/>
    <path class="st12" d="M374.5,542c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="517.5" y1="385" x2="517.5" y2="523.89"/>
    <path class="st12" d="M517.5,542c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="481.75" y1="436" x2="481.75" y2="498.89"/>
    <path class="st12" d="M481.75,517c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="398.33" y1="624" x2="398.33" y2="740.89"/>
    <path class="st12" d="M398.33,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="422.17" y1="646" x2="422.17" y2="740.89"/>
    <path class="st12" d="M422.17,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="446" y1="646" x2="446" y2="740.89"/>
    <path class="st12" d="M446,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="469.83" y1="646" x2="469.83" y2="740.89"/>
    <path class="st12" d="M469.83,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="374.5" y1="624" x2="374.5" y2="740.89"/>
    <path class="st12" d="M374.5,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="517.5" y1="624" x2="517.5" y2="740.89"/>
    <path class="st12" d="M517.5,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <g>
    <line class="st4" x1="493.67" y1="624" x2="493.67" y2="740.89"/>
    <path class="st12" d="M493.67,759c-3.16-8.52-8.55-19.09-14.27-25.64l14.27,5.16,14.26-5.16c-5.71,6.55-11.11,17.12-14.26,25.64Z"/>
  </g>
  <text class="st13" transform="translate(395.56 399)"><tspan x="0" y="0">Direct outs</tspan><tspan x="3.52" y="28.8">post fader</tspan></text>
  <text class="st13" transform="translate(406.41 635)"><tspan x="0" y="0">Outputs</tspan></text>
  <rect class="st2" x="340.5" y="551.5" width="209.5" height="63"/>
  <text class="st10" transform="translate(373.07 592)"><tspan x="0" y="0">WFS processor</tspan></text>
  <text class="st13" transform="translate(415.35 542)"><tspan x="0" y="0">Inputs</tspan></text>
  <text class="st16" transform="translate(60.24 131.6)"><tspan x="0" y="0">QLab / Ableton Live</tspan></text>
  <rect class="st0" x="24" y="65" width="264" height="110.5"/>
  <g>
    <polyline class="st5" points="193 194 193 238 301.89 238"/>
    <path class="st14" d="M320,238c-8.52,3.16-19.09,8.55-25.64,14.27l5.16-14.27-5.16-14.26c6.55,5.71,17.12,11.11,25.64,14.26Z"/>
  </g>
  <text class="st15" transform="translate(134.88 542)"><tspan x="0" y="0">positions and</tspan><tspan x="-20.77" y="28.8">other parameters</tspan></text>
  <text class="st15" transform="translate(211.34 231)"><tspan x="0" y="0">scenes</tspan><tspan x="6.22" y="28.8">recall</tspan></text>
  <text class="st16" transform="translate(169.23 738.6)"><tspan x="0" y="0">Remote</tspan></text>
  <path class="st2" d="M131,674h153.5v110.5h-153.5v-110.5ZM138.67,778.98h138.15v-99.45h-138.15v99.45Z"/>
  <g>
    <polyline class="st5" points="187 640.89 187 614 302.89 614"/>
    <path class="st14" d="M187,659c3.16-8.52,8.55-19.09,14.27-25.64l-14.27,5.16-14.26-5.16c5.71,6.55,11.11,17.12,14.26,25.64Z"/>
    <path class="st14" d="M321,614c-8.52,3.16-19.09,8.55-25.64,14.27l5.16-14.27-5.16-14.26c6.55,5.71,17.12,11.11,25.64,14.26Z"/>
  </g>
  <text class="st15" transform="translate(210.99 607)"><tspan x="0" y="0">remote</tspan><tspan x="1.07" y="28.8">control</tspan></text>
  <path class="st8" d="M943.5,543"/>
  <rect class="st2" x="340.45" y="780.36" width="67" height="67" transform="translate(1187.81 439.91) rotate(90)"/>
  <polygon class="st7" points="407.45 847.36 385.45 825.36 362.45 825.36 340.45 847.36 407.45 847.36"/>
  <rect class="st7" x="370.45" y="810.36" width="7" height="19" transform="translate(1193.81 445.91) rotate(90)"/>
  <path class="st6" d="M385.97,825.72c-3.08,1.63-7.33,2.64-12.02,2.64s-8.85-.99-11.92-2.58"/>
  <path class="st8" d="M415.95,855.42c-10.78,5.71-25.68,9.24-42.13,9.24s-31.02-3.45-41.77-9.05"/>
  <path class="st8" d="M430.63,867.85c-14.55,7.7-34.65,12.47-56.85,12.47s-41.86-4.66-56.37-12.21"/>
  <path class="st8" d="M450.44,884.63c-19.64,10.4-46.76,16.82-76.72,16.82s-56.49-6.29-76.07-16.48"/>
  <rect class="st2" x="483.45" y="780.36" width="67" height="67" transform="translate(1330.81 296.91) rotate(90)"/>
  <polygon class="st7" points="550.45 847.36 528.45 825.36 505.45 825.36 483.45 847.36 550.45 847.36"/>
  <rect class="st7" x="513.45" y="810.36" width="7" height="19" transform="translate(1336.81 302.91) rotate(90)"/>
  <path class="st6" d="M528.97,825.72c-3.08,1.63-7.33,2.64-12.02,2.64s-8.85-.99-11.92-2.58"/>
  <path class="st8" d="M558.95,855.42c-10.78,5.71-25.68,9.24-42.13,9.24s-31.02-3.45-41.77-9.05"/>
  <path class="st8" d="M573.63,867.85c-14.55,7.7-34.65,12.47-56.85,12.47s-41.86-4.66-56.37-12.21"/>
  <path class="st8" d="M593.44,884.63c-19.64,10.4-46.76,16.82-76.72,16.82s-56.49-6.29-76.07-16.48"/>
  <text class="st16" transform="translate(372.24 921.07)"><tspan x="0" y="0">Speaker arrays</tspan></text>
  <g>
    <polyline class="st5" points="98.5 213.11 98.5 549.88 302.89 549.88"/>
    <path class="st14" d="M98.5,195c-3.16,8.52-8.55,19.09-14.27,25.64l14.27-5.16,14.26,5.16c-5.71-6.55-11.11-17.12-14.26-25.64Z"/>
    <path class="st14" d="M321,549.88c-8.52,3.16-19.09,8.55-25.64,14.27l5.16-14.27-5.16-14.26c6.55,5.71,17.12,11.11,25.64,14.26Z"/>
  </)SVG",
        R"SVG(g>
  <g>
    <line class="st4" x1="239.5" y1="79.5" x2="271.5" y2="79.5"/>
    <path class="st3" d="M239.5,105.5c-7.18,0-13-5.82-13-13s5.82-13,13-13,13,5.82,13,13-5.82,13-13,13ZM246.5,92.5c0-3.87-3.13-7-7-7s-7,3.13-7,7,3.13,7,7,7,7-3.13,7-7Z"/>
    <path class="st3" d="M271.5,105.5c-7.18,0-13-5.82-13-13s5.82-13,13-13,13,5.82,13,13-5.82,13-13,13ZM278.5,92.5c0-3.87-3.13-7-7-7s-7,3.13-7,7,3.13,7,7,7,7-3.13,7-7Z"/>
  </g>
  <metadata><?xpacket begin="﻿" id="W5M0MpCehiHzreSzNTczkc9d"?>
<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP Core 9.1-c003 1.000000, 0000/00/00-00:00:00        ">
   <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
      <rdf:Description rdf:about=""
            xmlns:dc="http://purl.org/dc/elements/1.1/"
            xmlns:xmp="http://ns.adobe.com/xap/1.0/"
            xmlns:xmpGImg="http://ns.adobe.com/xap/1.0/g/img/"
            xmlns:xmpMM="http://ns.adobe.com/xap/1.0/mm/"
            xmlns:stRef="http://ns.adobe.com/xap/1.0/sType/ResourceRef#"
            xmlns:stEvt="http://ns.adobe.com/xap/1.0/sType/ResourceEvent#"
            xmlns:illustrator="http://ns.adobe.com/illustrator/1.0/"
            xmlns:xmpTPg="http://ns.adobe.com/xap/1.0/t/pg/"
            xmlns:stDim="http://ns.adobe.com/xap/1.0/sType/Dimensions#"
            xmlns:stFnt="http://ns.adobe.com/xap/1.0/sType/Font#"
            xmlns:xmpG="http://ns.adobe.com/xap/1.0/g/"
            xmlns:pdf="http://ns.adobe.com/pdf/1.3/">
         <dc:format>image/svg+xml</dc:format>
         <dc:title>
            <rdf:Alt>
               <rdf:li xml:lang="x-default">signalFlow</rdf:li>
            </rdf:Alt>
         </dc:title>
         <xmp:CreatorTool>Adobe Illustrator 29.5 (Windows)</xmp:CreatorTool>
         <xmp:CreateDate>2026-03-29T19:32:00+02:00</xmp:CreateDate>
         <xmp:ModifyDate>2026-03-29T19:32:02+02:00</xmp:ModifyDate>
         <xmp:MetadataDate>2026-03-29T19:32:02+02:00</xmp:MetadataDate>
         <xmp:Thumbnails>
            <rdf:Alt>
               <rdf:li rdf:parseType="Resource">
                  <xmpGImg:width>256</xmpGImg:width>
                  <xmpGImg:height>256</xmpGImg:height>
                  <xmpGImg:format>JPEG</xmpGImg:format>
                  <xmpGImg:image>/9j/4AAQSkZJRgABAgEAAAAAAAD/7QAsUGhvdG9zaG9wIDMuMAA4QklNA+0AAAAAABAAAAAAAAEA&#xA;AQAAAAAAAQAB/+4ADkFkb2JlAGTAAAAAAf/bAIQABgQEBAUEBgUFBgkGBQYJCwgGBggLDAoKCwoK&#xA;DBAMDAwMDAwQDA4PEA8ODBMTFBQTExwbGxscHx8fHx8fHx8fHwEHBwcNDA0YEBAYGhURFRofHx8f&#xA;Hx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8f/8AAEQgBAAEAAwER&#xA;AAIRAQMRAf/EAaIAAAAHAQEBAQEAAAAAAAAAAAQFAwIGAQAHCAkKCwEAAgIDAQEBAQEAAAAAAAAA&#xA;AQACAwQFBgcICQoLEAACAQMDAgQCBgcDBAIGAnMBAgMRBAAFIRIxQVEGE2EicYEUMpGhBxWxQiPB&#xA;UtHhMxZi8CRygvElQzRTkqKyY3PCNUQnk6OzNhdUZHTD0uIIJoMJChgZhJRFRqS0VtNVKBry4/PE&#xA;1OT0ZXWFlaW1xdXl9WZ2hpamtsbW5vY3R1dnd4eXp7fH1+f3OEhYaHiImKi4yNjo+Ck5SVlpeYmZ&#xA;qbnJ2en5KjpKWmp6ipqqusra6voRAAICAQIDBQUEBQYECAMDbQEAAhEDBCESMUEFURNhIgZxgZEy&#xA;obHwFMHR4SNCFVJicvEzJDRDghaSUyWiY7LCB3PSNeJEgxdUkwgJChgZJjZFGidkdFU38qOzwygp&#xA;0+PzhJSktMTU5PRldYWVpbXF1eX1RlZmdoaWprbG1ub2R1dnd4eXp7fH1+f3OEhYaHiImKi4yNjo&#xA;+DlJWWl5iZmpucnZ6fkqOkpaanqKmqq6ytrq+v/aAAwDAQACEQMRAD8A9U4qp3UxgtppgORiRn49&#xA;K8RWmShG5Ad6vJYPz2uJEsXk0n0Y/qctzqrEk8JrdomkghqUB/cTpIGJpR1986mXs2BxATv1AR9x&#xA;uif84EV5HyavETyX84LGGbhJpzGIusazpcQOkj0g5iIhv3nFrtPs9VDN0G+FHsGRG0t+7hlt9XPu&#xA;+k8+tBPiIC3/ADytJLl2fTH+oelbrFOk0TgXM63coVpA3p+l6dl8Tj7Dcg3TL5ezkhH6/VZ2o8hw&#xA;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&#xA;67x030Zf6g/3cFPR3nX/AJQ3)SVG",
        R"SVG(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&#xA;7FXYq7FXYq7FXYq//9k=</xmpGImg:image>
               </rdf:li>
            </rdf:Alt>
         </xmp:Thumbnails>
         <xmpMM:RenditionClass>default</xmpMM:RenditionClass>
         <xmpMM:OriginalDocumentID>uuid:65E6390686CF11DBA6E2D887CEACB407</xmpMM:OriginalDocumentID>
         <xmpMM:DocumentID>xmp.did:782e4d74-0322-c143-86ee-def17aa4502e</xmpMM:DocumentID>
         <xmpMM:InstanceID>xmp.iid:a0efde4d-aa77-4440-95f1-2253e9d46b98</xmpMM:InstanceID>
         <xmpMM:DerivedFrom rdf:parseType="Resource">
            <stRef:instanceID>uuid:72f342b5-3509-47aa-92b7-706b9c91ee89</stRef:instanceID>
            <stRef:documentID>xmp.did:8d32a0a9-ee5c-954b-81cb-172e8371f4b7</stRef:documentID>
            <stRef:originalDocumentID>uuid:65E6390686CF11DBA6E2D887CEACB407</stRef:originalDocumentID>
            <stRef:renditionClass>default</stRef:renditionClass>
         </xmpMM:DerivedFrom>
         <xmpMM:History>
            <rdf:Seq>
               <rdf:li rdf:parseType="Resource">
                  <stEvt:action>saved</stEvt:action>
                  <stEvt:instanceID>xmp.iid:f9b0fa30-ac4b-a443-b9eb-4cf4400d6c76</stEvt:instanceID>
                  <stEvt:when>2021-07-22T18:26:15+02:00</stEvt:when>
                  <stEvt:softwareAgent>Adobe Illustrator CC 22.1 (Windows)</stEvt:softwareAgent>
                  <stEvt:changed>/</stEvt:changed>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <stEvt:action>saved</stEvt:action>
                  <stEvt:instanceID>xmp.iid:782e4d74-0322-c143-86ee-def17aa4502e</stEvt:instanceID>
                  <stEvt:when>2026-03-29T19:32:02+02:00</stEvt:when>
                  <stEvt:softwareAgent>Adobe Illustrator 29.5 (Windows)</stEvt:softwareAgent>
                  <stEvt:changed>/</stEvt:changed>
               </rdf:li>
            </rdf:Seq>
         </xmpMM:History>
         <illustrator:StartupProfile>Web</illustrator:StartupProfile>
         <illustrator:CreatorSubTool>Adobe Illustrator</illustrator:CreatorSubTool>
         <xmpTPg:NPages>1</xmpTPg:NPages>
         <xmpTPg:HasVisibleTransparency>False</xmpTPg:HasVisibleTransparency>
         <xmpTPg:HasVisibleOverprint>False</xmpTPg:HasVisibleOverprint>
         <xmpTPg:MaxPageSize rdf:parseType="Resource">
            <stDim:w>613.000000</stDim:w>
            <stDim:h>928.000000</stDim:h>
            <stDim:unit>Pixels</stDim:unit>
         </xmpTPg:MaxPageSize>
         <xmpTPg:Fonts>
            <rdf:Bag>
               <rdf:li rdf:parseType="Resource">
                  <stFnt:fontName>Calibri</stFnt:fontName>
                  <stFnt:fontFamily>Calibri</stFnt:fontFamily>
                  <stFnt:fontFace>Regular</stFnt:fontFace>
                  <stFnt:fontType>TrueType</stFnt:fontType>
                  <stFnt:versionString>Version 6.27</stFnt:versionString>
                  <stFnt:composite>False</stFnt:composite>
                  <stFnt:fontFileName>calibri.ttf</stFnt:fontFileName>
               </rdf:li>
            </rdf:Bag>
         </xmpTPg:Fonts>
         <xmpTPg:PlateNames>
            <rdf:Seq>
               <rdf:li>Cyan</rdf:li>
               <rdf:li>Magenta</rdf:li>
               <rdf:li>Yellow</rdf:li>
               <rdf:li>Black</rdf:li>
            </rdf:Seq>
         </xmpTPg:PlateNames>
         <xmpTPg:SwatchGroups>
            <rdf:Seq>
               <rdf:li rdf:parseType="Resource">
                  <xmpG:groupName>Default Swatch Group</xmpG:groupName>
                  <xmpG:groupType>0</xmpG:groupType>
                  <xmpG:Colorants>
                     <rdf:Seq>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>White</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:)SVG",
        R"SVG(li rdf:parseType="Resource">
                           <xmpG:swatchName>Black</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Red</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Yellow</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Green</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Cyan</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>255</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Blue</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>RGB Magenta</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>255</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=193 G=39 B=45</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>193</xmpG:red>
                           <xmpG:green>39</xmpG:green>
                           <xmpG:blue>45</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=237 G=28 B=36</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>237</xmpG:red>
                           <xmpG:green>28</xmpG:green>
                           <xmpG:blue>36</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=241 G=90 B=36</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>241</xmpG:red>
                           <xmpG:green>90</xmpG:green>
                           <xmpG:blue>36</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=247 G=147 B=30</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>247</xmpG:red>
                           <xmpG:green>147</xmpG:green>
                           <xmpG:blue>30</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=251 G=176 B=59</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>251</xmpG:red>
                           <xmpG:green>176</xmpG:green>
                           <xmpG:blue>59</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=252 G=238 B=33</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>252</xmpG:red>
                           <xmpG:green>238</xmpG:green>
                           <xmpG:blue>33</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=217 G=224 B=33</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>217</xmpG:red>
                           <xmpG:green>224</xmpG:green>
                           <xmpG:blue>33</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=140 G=198 B=63</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>140</xmpG:red>
                           <xmpG:green>198</xmpG:green>
                           <xmpG:blue>63</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=57 G=181 B=74</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>57</xmpG:red>
                           <xmpG:green>181</xmpG:green>
                           <xmpG:blue>74</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=146 B=69</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>146</xmpG:green>
                           <xmpG:blue>69</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=104 B=55</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>104</xmpG:green>
                           <xmpG:blue>55</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=34 G=181 B=115</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>34</xmpG:red>
                           <xmpG:green>181</xmpG:green>
                           <xmpG:blue>115</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=169 B=157</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>169</xmpG:green>
                           <xmpG:blue>157</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=41 G=171 B=226</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>41</xmpG:red>
                           <xmpG:green>171</xmpG:green>
                           <xmpG:blue>226</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=113 B=188</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>113</xmpG:green>
                           <xmpG:blue>188</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=46 G=49 B=146</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>46</xmpG:red>
                           <xmpG:green>49</xmpG:green>
                           <xmpG:blue>146</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=27 G=20 B=100</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>27</xmpG:red>
                           <xmpG:green>20</xmpG:green>
                           <xmpG:blue>100</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=102 G=45 B=145</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>102</xmpG:red>
                           <xmpG:green>45</xmpG:green>
                           <xmpG:blue>145</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=147 G=39 B=143</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>147</xmpG:red>
                           <xmpG:green>39</xmpG:green>
                           <xmpG:blue>143</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=158 G=0 B=93</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>158</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>93</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=212 G=20 B=90</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>212</xmpG:red>
                           <xmpG:green>20</xmpG:green>
                           <xmpG:blue>90</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=237 G=30 B=121</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>237</xmpG:red>
                           <xmpG:green>30</xmpG:green>
                           <xmpG:blue>121</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=199 G=178 B=153</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>199</xmpG:red>
                           <xmpG:green>178</xmpG:green>
                           <xmpG:blue>153</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=153 G=134 B=117</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>153</xmpG:red>
                           <xmpG:green>134</xmpG:green>
                           <xmpG:blue>117</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=115 G=99 B=87</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>115</xmpG:red>
                           <xmpG:green>99</xmpG:green>
       )SVG",
        R"SVG(                    <xmpG:blue>87</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=83 G=71 B=65</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>83</xmpG:red>
                           <xmpG:green>71</xmpG:green>
                           <xmpG:blue>65</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=198 G=156 B=109</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>198</xmpG:red>
                           <xmpG:green>156</xmpG:green>
                           <xmpG:blue>109</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=166 G=124 B=82</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>166</xmpG:red>
                           <xmpG:green>124</xmpG:green>
                           <xmpG:blue>82</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=140 G=98 B=57</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>140</xmpG:red>
                           <xmpG:green>98</xmpG:green>
                           <xmpG:blue>57</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=117 G=76 B=36</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>117</xmpG:red>
                           <xmpG:green>76</xmpG:green>
                           <xmpG:blue>36</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=96 G=56 B=19</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>96</xmpG:red>
                           <xmpG:green>56</xmpG:green>
                           <xmpG:blue>19</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=66 G=33 B=11</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>66</xmpG:red>
                           <xmpG:green>33</xmpG:green>
                           <xmpG:blue>11</xmpG:blue>
                        </rdf:li>
                     </rdf:Seq>
                  </xmpG:Colorants>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <xmpG:groupName>Grays</xmpG:groupName>
                  <xmpG:groupType>1</xmpG:groupType>
                  <xmpG:Colorants>
                     <rdf:Seq>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=0 G=0 B=0</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>0</xmpG:red>
                           <xmpG:green>0</xmpG:green>
                           <xmpG:blue>0</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=26 G=26 B=26</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>26</xmpG:red>
                           <xmpG:green>26</xmpG:green>
                           <xmpG:blue>26</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=51 G=51 B=51</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>51</xmpG:red>
                           <xmpG:green>51</xmpG:green>
                           <xmpG:blue>51</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=77 G=77 B=77</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>77</xmpG:red>
                           <xmpG:green>77</xmpG:green>
                           <xmpG:blue>77</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=102 G=102 B=102</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>102</xmpG:red>
                           <xmpG:green>102</xmpG:green>
                           <xmpG:blue>102</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=128 G=128 B=128</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>128</xmpG:red>
                           <xmpG:green>128</xmpG:green>
                           <xmpG:blue>128</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=153 G=153 B=153</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>153</xmpG:red>
                           <xmpG:green>153</xmpG:green>
                           <xmpG:blue>153</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=179 G=179 B=179</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>179</xmpG:red>
                           <xmpG:green>179</xmpG:green>
                           <xmpG:blue>179</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=204 G=204 B=204</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>204</xmpG:red>
                           <xmpG:green>204</xmpG:green>
                           <xmpG:blue>204</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=230 G=230 B=230</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>230</xmpG:red>
                           <xmpG:green>230</xmpG:green>
                           <xmpG:blue>230</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=242 G=242 B=242</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>242</xmpG:red>
                           <xmpG:green>242</xmpG:green>
                           <xmpG:blue>242</xmpG:blue>
                        </rdf:li>
                     </rdf:Seq>
                  </xmpG:Colorants>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <xmpG:groupName>Web Color Group</xmpG:groupName>
                  <xmpG:groupType>1</xmpG:groupType>
                  <xmpG:Colorants>
                     <rdf:Seq>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=63 G=169 B=245</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>63</xmpG:red>
                           <xmpG:green>169</xmpG:green>
                           <xmpG:blue>245</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=122 G=201 B=67</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>122</xmpG:red>
                           <xmpG:green>201</xmpG:green>
                           <xmpG:blue>67</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=255 G=147 B=30</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>147</xmpG:green>
                           <xmpG:blue>30</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=255 G=29 B=37</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>29</xmpG:green>
                           <xmpG:blue>37</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=255 G=123 B=172</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>255</xmpG:red>
                           <xmpG:green>123</xmpG:green>
                           <xmpG:blue>172</xmpG:blue>
                        </rdf:li>
                        <rdf:li rdf:parseType="Resource">
                           <xmpG:swatchName>R=189 G=204 B=212</xmpG:swatchName>
                           <xmpG:mode>RGB</xmpG:mode>
                           <xmpG:type>PROCESS</xmpG:type>
                           <xmpG:red>189</xmpG:red>
                           <xmpG:green>204</xmpG:green>
                           <xmpG:blue>212</xmpG:blue>
                        </rdf:li>
                     </rdf:Seq>
                  </xmpG:Colorants>
               </rdf:li>
            </rdf:Seq>
         </xmpTPg:SwatchGroups>
         <pdf:Producer>Adobe PDF library 17.00</pdf:Producer>
      </rdf:Description>
   </rdf:RDF>
</x:xmpmeta>
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                           
<?xpacket end="w"?>
  </metadata>
</svg>)SVG"
    };
    static juce::String s;
    if (s.isEmpty())
        for (auto* p : parts)
            s += juce::String(juce::CharPointer_UTF8(p));
    return s;
}

} // namespace HelpCardSVG
