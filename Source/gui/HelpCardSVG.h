#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"

/**
 * SVG illustration strings for help cards.
 * Parsed at runtime with juce::Drawable::createFromSVG().
 * Colors adapted for current theme.
 */
namespace HelpCardSVG
{

/** Adapt SVG colors for the current theme (dark/light) */
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
        // Classless polygons (arrowheads) default to black fill
        result = result.replace("<polygon points=", "<polygon fill=\"#ddd\" points=");
    }
    return result;
}

/** Parse an SVG string into a Drawable, with theme color adaptation */
inline std::unique_ptr<juce::Drawable> parse(const juce::String& svg)
{
    auto adapted = adaptForTheme(svg);
    auto xml = juce::XmlDocument::parse(adapted);
    if (xml == nullptr) return nullptr;
    return juce::Drawable::createFromSVG(*xml);
}

inline const char* parallax1SVG = R"SVG(
<?xml version="1.0" encoding="UTF-8"?>
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

inline const char* parallax2SVG = R"SVG(
<?xml version="1.0" encoding="UTF-8"?>
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

inline const char* tuning1SVG = R"SVG(
<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="0 0 1615.75 1052.79">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0 {
        fill: #fff;
        stroke-width: 6.75px;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6, .st7 {
        stroke-miterlimit: 10;
      }

      .st0, .st3, .st4, .st7 {
        stroke: #000;
      }

      .st1 {
        stroke: #009245;
        stroke-dasharray: 12;
      }

      .st1, .st2, .st3, .st4, .st5, .st6 {
        fill: none;
      }

      .st1, .st2, .st5 {
        stroke-width: 6px;
      }

      .st2 {
        stroke-dasharray: 11.87 11.87;
      }

      .st2, .st5 {
        stroke: blue;
      }

      .st3 {
        stroke-width: 15px;
      }

      .st8 {
        fill: blue;
      }

      .st4 {
        stroke-width: 2.25px;
      }

      .st6 {
        stroke: #666;
        stroke-width: 12px;
      }

      .st7 {
        fill: #ccc;
        stroke-width: 5px;
      }
    </style>
  </defs>
  <g>
    <rect class="st7" x="451.74" y="894.67" width="30.54" height="30.54" transform="translate(-442.93 1376.95) rotate(-90)"/>
    <polygon class="st7" points="482.17 897.32 469.56 909.94 482.17 922.56 482.17 897.32"/>
  </g>
  <line class="st1" x1="486.87" y1="902.49" x2="678.87" y2="825.99"/>
  <circle class="st8" cx="199.75" cy="732.37" r="21.26"/>
  <polyline class="st3" points="1488.87 533.49 660.87 938.49 144.87 938.49"/>
  <g>
    <line class="st5" x1="669.12" y1="817.74" x2="663.21" y2="816.71"/>
    <line class="st2" x1="651.52" y1="814.66" x2="236.33" y2="741.87"/>
    <line class="st5" x1="230.48" y1="740.85" x2="224.57" y2="739.81"/>
  </g>
  <g>
    <rect class="st7" x="474.24" y="261.67" width="30.54" height="30.54" transform="translate(41.98 617.58) rotate(-66.78)"/>
    <polygon class="st7" points="508.42 271.33 491.85 277.94 498.47 294.52 508.42 271.33"/>
  </g>
  <g>
    <line class="st6" x1="548.53" y1="259.15" x2="575.22" y2="365.34"/>
    <line class="st6" x1="508.77" y1="325.59" x2="614.97" y2="298.9"/>
  </g>
  <g>
    <path class="st0" d="M672.2,824.49c0,19.39,10.88,24.3,24.3,24.3s21.52-7.54,24.3-27c2.7-18.9-10.88-35.1-24.3-35.1-9.22,0-17.25,7.42-21.36,18.36-.94,2.49-7.84,13.94-8.34,16.74-.5,2.81,5.4-.33,5.4,2.7Z"/>
    <path class="st4" d="M699.95,808.04c.3,1.19,4.88,5.17.95,11.02"/>
  </g>
</svg>)SVG";

inline const char* tuning2SVG = R"SVG(
<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="0 0 1615.75 1052.79">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0 {
        stroke-width: 15px;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6 {
        stroke-miterlimit: 10;
      }

      .st0, .st1, .st2, .st3, .st4, .st6 {
        fill: none;
      }

      .st0, .st1, .st5 {
        stroke: #000;
      }

      .st7 {
        fill: #fff;
      }

      .st8 {
        fill: blue;
      }

      .st1 {
        stroke-width: 2.25px;
      }

      .st2 {
        stroke: red;
        stroke-dasharray: 12;
      }

      .st2, .st3, .st6 {
        stroke-width: 6px;
      }

      .st3, .st6 {
        stroke: blue;
      }

      .st4 {
        stroke: #666;
        stroke-width: 12px;
      }

      .st5 {
        fill: #ccc;
        stroke-width: 5px;
      }

      .st6 {
        stroke-dasharray: 11.94 11.94;
      }
    </style>
  </defs>
  <g>
    <rect class="st5" x="451.74" y="894.67" width="30.54" height="30.54" transform="translate(-442.93 1376.95) rotate(-90)"/>
    <polygon class="st5" points="482.17 897.32 469.56 909.94 482.17 922.56 482.17 897.32"/>
  </g>
  <g>
    <g>
      <path class="st7" d="M1364,515.42c-14.67,0-20.92-6.26-20.92-20.92,0-.45-.13-4.15-4.33-4.75,1.09-2.22,2.72-5.21,3.77-7.13,1.81-3.31,2.85-5.25,3.28-6.38,3.75-9.97,10.73-16.17,18.2-16.17,4.9,0,10.05,2.7,14.13,7.4,5.58,6.44,8.07,15.13,6.83,23.84-3.03,21.24-12.3,24.1-20.96,24.1Z"/>
      <path d="M1364,463.44c3.86,0,8.19,2.33,11.58,6.24,4.87,5.62,7.13,13.52,6.04,21.16-1.15,8.07-3.31,13.83-6.4,17.13-1.64,1.75-4.59,4.07-11.22,4.07-12.79,0-17.55-4.76-17.55-17.55,0-2.34-.98-4.79-3.05-6.4.75-1.41,1.51-2.81,2.09-3.87,1.93-3.54,2.96-5.46,3.47-6.8,3.25-8.63,9.01-13.98,15.04-13.98M1364,456.69c-9.22,0-17.25,7.42-21.36,18.36-.94,2.49-7.84,13.94-8.34,16.74-.2,1.11.6,1.29,1.65,1.29.5,0,1.06-.04,1.59-.04,1.15,0,2.16.2,2.16,1.45,0,19.39,10.88,24.3,24.3,24.3s21.52-7.54,24.3-27c2.7-18.9-10.88-35.1-24.3-35.1h0Z"/>
    </g>
    <path class="st1" d="M1368.4,489.06c3.92-5.85-.65-9.84-.95-11.02"/>
  </g>
  <line class="st2" x1="504.87" y1="284.49" x2="1346.37" y2="480.99"/>
  <circle class="st8" cx="199.75" cy="732.37" r="21.26"/>
  <polyline class="st0" points="1488.87 533.49 660.87 938.49 144.87 938.49"/>
  <g>
    <line class="st3" x1="1322.37" y1="491.49" x2="1316.5" y2="492.74"/>
    <line class="st6" x1="1304.83" y1="495.22" x2="236.23" y2="722.19"/>
    <line class="st3" x1="230.39" y1="723.43" x2="224.52" y2="724.68"/>
  </g>
  <g>
    <rect class="st5" x="474.24" y="261.67" width="30.54" height="30.54" transform="translate(41.98 617.58) rotate(-66.78)"/>
    <polygon class="st5" points="508.42 271.33 491.85 277.94 498.47 294.52 508.42 271.33"/>
  </g>
  <g>
    <line class="st4" x1="579.91" y1="833.13" x2="525.84" y2="928.35"/>
    <line class="st4" x1="505.26" y1="853.71" x2="600.48" y2="907.78"/>
  </g>
</svg>)SVG";

inline const char* tuning3SVG = R"SVG(
<?xml version="1.0" encoding="UTF-8"?>
<svg id="Layer_1" xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="0 0 1615.75 1052.79">
  <!-- Generator: Adobe Illustrator 29.5.0, SVG Export Plug-In . SVG Version: 2.1.0 Build 137)  -->
  <defs>
    <style>
      .st0 {
        stroke: #009245;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6, .st7 {
        fill: none;
      }

      .st0, .st1, .st2, .st3, .st4, .st5, .st6, .st7, .st8 {
        stroke-miterlimit: 10;
      }

      .st0, .st4, .st5, .st6 {
        stroke-width: 6px;
      }

      .st0, .st5 {
        stroke-dasharray: 12;
      }

      .st1, .st2, .st7 {
        stroke-width: 15px;
      }

      .st1, .st3, .st8 {
        stroke: #000;
      }

      .st9 {
        fill: #fff;
      }

      .st10 {
        fill: blue;
      }

      .st11 {
        fill: #666;
      }

      .st2 {
        stroke-dasharray: 3.03 3.03;
      }

      .st2, .st7 {
        stroke: #666;
      }

      .st3 {
        stroke-width: 2.25px;
      }

      .st4 {
        stroke-dasharray: 11.89 11.89;
      }

      .st4, .st6 {
        stroke: blue;
      }

      .st5 {
        stroke: red;
      }

      .st8 {
        fill: #ccc;
        stroke-width: 5px;
      }
    </style>
  </defs>
  <g>
    <rect class="st8" x="451.74" y="894.67" width="30.54" height="30.54" transform="translate(-442.93 1376.95) rotate(-90)"/>
    <polygon class="st8" points="482.17 897.32 469.55 909.94 482.17 922.56 482.17 897.32"/>
  </g>
  <g>
    <g>
      <path class="st9" d="M944,710.42c-14.67,0-20.93-6.26-20.93-20.93,0-.45-.13-4.15-4.33-4.75,1.09-2.22,2.72-5.21,3.77-7.13,1.81-3.31,2.85-5.24,3.28-6.38,3.76-9.97,10.73-16.17,18.2-16.17,4.9,0,10.05,2.7,14.13,7.4,5.58,6.44,8.07,15.13,6.83,23.84-3.03,21.24-12.3,24.1-20.96,24.1Z"/>
      <path d="M944,658.44c3.86,0,8.19,2.33,11.58,6.24,4.87,5.62,7.13,13.52,6.04,21.16-1.15,8.07-3.31,13.83-6.4,17.13-1.64,1.75-4.59,4.07-11.22,4.07-12.79,0-17.55-4.76-17.55-17.55,0-2.34-.98-4.79-3.05-6.4.75-1.41,1.51-2.81,2.09-3.87,1.93-3.54,2.96-5.46,3.47-6.8,3.25-8.63,9.01-13.98,15.04-13.98M944,651.69c-9.22,0-17.25,7.42-21.36,18.36-.94,2.49-7.84,13.94-8.34,16.74-.2,1.11.6,1.29,1.65,1.29.5,0,1.06-.04,1.59-.04,1.15,0,2.16.2,2.16,1.45,0,19.39,10.88,24.3,24.3,24.3s21.52-7.54,24.3-27c2.7-18.9-10.88-35.1-24.3-35.1h0Z"/>
    </g>
    <path class="st3" d="M948.4,684.06c3.92-5.85-.65-9.84-.95-11.02"/>
  </g>
  <line class="st5" x1="504.87" y1="284.49" x2="926.37" y2="675.99"/>
  <line class="st0" x1="486.87" y1="908.49" x2="920.37" y2="692.49"/>
  <circle class="st10" cx="199.75" cy="732.37" r="21.26"/>
  <polyline class="st1" points="1488.87 533.49 660.87 938.49 144.87 938.49"/>
  <g>
    <line class="st6" x1="225.19" y1="730.12" x2="231.17" y2="729.69"/>
    <line class="st4" x1="243.03" y1="728.83" x2="900.96" y2="681.35"/>
    <line class="st6" x1="906.89" y1="680.93" x2="912.87" y2="680.49"/>
  </g>
  <g>
    <line class="st7" x1="899.7" y1="755.93" x2="901.05" y2="755.26"/>
    <line class="st2" x1="903.76" y1="753.91" x2="1032.84" y2="689.9"/>
    <line class="st7" x1="1034.2" y1="689.23" x2="1035.54" y2="688.56"/>
    <path class="st11" d="M875.37,767.99c13.56-1.43,31.35-1.23,43.97,2.08l-16.44-15.73-2.57-22.61c-5,12.04-15.61,26.34-24.95,36.26Z"/>
    <path class="st11" d="M1059.87,676.49c-9.34,9.93-19.95,24.22-24.95,36.26l-2.57-22.61-16.44-15.73c12.61,3.31,30.41,3.51,43.96,2.08Z"/>
  </g>
  <g>
    <rect class="st8" x="474.24" y="261.67" width="30.54" height="30.54" transform="translate(41.98 617.58) rotate(-66.78)"/>
    <polygon class="st8" points="508.42 271.33 491.85 277.94 498.47 294.52 508.42 271.33"/>
  </g>
</svg>)SVG";

} // namespace HelpCardSVG
