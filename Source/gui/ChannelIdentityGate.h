#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <functional>
#include <vector>

#include "../WfsParameters.h"
#include "../WFSLogger.h"
#include "../Localization/LocalizationManager.h"
#include "../Parameters/InputChannelDescription.h"
#include "../Parameters/InputChannelIdentity.h"
#include "../Parameters/WFSFileManager.h"

/**
    The operator-facing half of the channel identity gate.

    WFSFileManager can tell, before applying a file, that the file describes a
    different channel list than the session and that loading it would cross
    parameter sets by number and hardware inputs by position. What it cannot do
    is ask. This asks: preflight; if safe, load at once; if not, name what
    differs, offer the reconciliation that fits (renumber by position, renumber
    by hardware fingerprint, or rearrange), then re-check and load - or load
    anyway with the consequences listed, or cancel. Nothing here is silent and
    nothing depends on state the operator cannot see: every channel is named by
    number, name and type, and every automatic suggestion is a list of pairs to
    confirm.

    Three-button dialogs return 1 / 2 / 0 for buttons 0 / 1 / 2 (JUCE's
    ((X + 1) % N) rule); toIndex() below turns that back into a button index so
    no callback here ever reasons about the raw code.
*/
namespace ChannelIdentityGate
{
    using Diff = InputChannelIdentityDiff;
    using Rel  = InputChannelIdentityDiff::Relation;
    using Kind = WFSFileManager::LoadKind;

    struct Context
    {
        juce::Component* parent = nullptr;                  // the dialog's associated component
        WfsParameters*   parameters = nullptr;
        std::function<void()> afterStructuralChange;         // the reconfiguration seam
        std::function<void (const juce::String&)> showStatus;
    };

    //==========================================================================
    namespace detail
    {
        inline bool& dialogOpen() { static bool open = false; return open; }

        inline juce::String L (const char* key)
        {
            return LOC (juce::String ("systemConfig.dialogs.channelIdentity.") + key);
        }

        inline juce::String hwText (const std::vector<int>& hw)
        {
            return hw.empty() ? juce::String ("-") : InputChannelIdentityDetail::hwInputsToString (hw);
        }

        inline juce::String capped (const juce::StringArray& lines, int max = 12)
        {
            juce::StringArray out;
            for (int i = 0; i < lines.size() && i < max; ++i)
                out.add (lines[i]);
            if (lines.size() > max)
                out.add (L ("lineMore").replace ("{n}", juce::String (lines.size() - max)));
            return out.joinIntoString ("\n");
        }

        inline int toIndex (int juceCode, int numButtons)
        {
            return numButtons <= 0 ? 0 : (juceCode - 1 + numButtons) % numButtons;
        }

        inline void status (const Context& ctx, const juce::String& text)
        {
            if (ctx.showStatus) ctx.showStatus (text);
        }

        /** The reconciliation that fits this diff, if any. */
        struct Fix
        {
            enum Kind { none, relabel, reorder } kind = none;
            std::vector<int> numbers;
            juce::String buttonLabel, offerText;
        };

        inline juce::String pairLines (const WFSValueTreeState& vts, const std::vector<int>& numbersBySlot)
        {
            juce::StringArray lines;
            for (size_t i = 0; i < numbersBySlot.size(); ++i)
                if (vts.getInputChannelNumber ((int) i) != numbersBySlot[i])
                    lines.add (L ("pairLine")
                                   .replace ("{channel}", describeInputChannel (vts, (int) i))
                                   .replace ("{newNumber}", juce::String (numbersBySlot[i])));
            return capped (lines);
        }

        inline Fix pickFix (const Diff& d, const WFSValueTreeState& vts)
        {
            Fix f;
            if (d.hardwareRelabel)
            {
                f.kind = Fix::relabel; f.numbers = *d.hardwareRelabel;
                f.buttonLabel = L ("takeNumbersHw");
                f.offerText   = L ("relabelOfferHw") + "\n" + pairLines (vts, f.numbers);
            }
            else if (d.relation == Rel::positionalTypesMatch)
            {
                f.kind = Fix::relabel; f.numbers = d.fileNumbersBySlot;
                f.buttonLabel = L ("takeNumbers");
                f.offerText   = L ("relabelOffer") + "\n" + pairLines (vts, f.numbers);
            }
            else if (d.relation == Rel::orderOnly)
            {
                f.kind = Fix::reorder; f.numbers = d.fileNumbersInOrder;
                f.buttonLabel = L ("rearrangeFirst");
                f.offerText   = L ("reorderOffer");
            }
            return f;
        }

        inline juce::Result applyFix (const Fix& f, WFSValueTreeState& vts, const juce::String& reason)
        {
            switch (f.kind)
            {
                case Fix::relabel: return vts.assignInputChannelNumbersBySlot (f.numbers, reason);
                case Fix::reorder: return vts.reorderInputChannelsToNumbers (f.numbers, reason);
                default:           return juce::Result::fail ("no fix");
            }
        }

        /** What "Load anyway" would do, one line per affected channel. */
        inline juce::String consequences (const Diff& d, Kind kind)
        {
            juce::StringArray lines;
            for (const auto& r : d.retyped)
                lines.add (L ("lineRetyped")
                               .replace ("{channel}", describeInputChannel (r.live))
                               .replace ("{newType}", LOC (r.fileStereo ? "systemConfig.channelList.stereo"
                                                                        : "systemConfig.channelList.mono")));
            for (const auto& r : d.removed)
                lines.add (L ("lineRemoved").replace ("{channel}", describeInputChannel (r)));
            for (const auto& r : d.added)
                lines.add (L ("lineAdded")
                               .replace ("{number}", juce::String (r.number))
                               .replace ("{type}", LOC (r.stereo ? "systemConfig.channelList.stereo"
                                                                 : "systemConfig.channelList.mono")));
            if (d.relation == Rel::conflicting || (d.relation == Rel::orderOnly && kind == Kind::inputConfig))
                lines.add (L ("lineRowsCross"));
            for (const auto& p : d.patchDiffers)
                lines.add (L ("linePatch")
                               .replace ("{channel}", describeInputChannel (p.live))
                               .replace ("{file}", hwText (p.fileHwInputs))
                               .replace ("{live}", hwText (p.live.hwInputs)));
            return capped (lines);
        }

        inline juce::String noIdentityText (const Diff& d, const WFSValueTreeState& vts, const juce::String& fileName)
        {
            const int liveCount = vts.getNumInputChannels();
            juce::String delta;
            if (liveCount > d.fileLegacyCount)
            {
                // setNumInputChannels shrinks by HIGHEST NUMBER - that rule, not
                // the display-order one, is what a legacy load runs.
                std::vector<std::pair<int, int>> byNumber;   // number, slot
                for (int s = 0; s < liveCount; ++s)
                    byNumber.push_back ({ vts.getInputChannelNumber (s), s });
                std::sort (byNumber.begin(), byNumber.end(), [] (auto& a, auto& b) { return a.first > b.first; });
                juce::StringArray names;
                const int n = liveCount - d.fileLegacyCount;
                for (int i = 0; i < n && i < (int) byNumber.size(); ++i)
                    names.add (describeInputChannel (vts, byNumber[(size_t) i].second));
                delta = L ("noIdentityRemove").replace ("{n}", juce::String (n)).replace ("{channels}", names.joinIntoString (", "));
            }
            else if (liveCount < d.fileLegacyCount)
                delta = L ("noIdentityAppend").replace ("{n}", juce::String (d.fileLegacyCount - liveCount));
            else
                delta = L ("noIdentityKeep");

            return L ("noIdentity")
                       .replace ("{file}", fileName)
                       .replace ("{count}", juce::String (d.fileLegacyCount))
                       .replace ("{liveCount}", juce::String (liveCount))
                       .replace ("{delta}", delta);
        }

        /** One dialog shape for everything here. `onButton` receives the 0-based
            index of the button pressed; escape/dismiss maps to the last one. */
        inline void show (const Context& ctx, const juce::String& title, const juce::String& message,
                          const juce::StringArray& buttons, std::function<void (int)> onButton)
        {
            auto options = juce::MessageBoxOptions()
                               .withIconType (juce::MessageBoxIconType::WarningIcon)
                               .withTitle (title)
                               .withMessage (message)
                               .withAssociatedComponent (ctx.parent);
            for (const auto& b : buttons)
                options = options.withButton (b);

            dialogOpen() = true;
            const int n = buttons.size();
            juce::AlertWindow::showAsync (options, [n, onButton] (int code)
            {
                dialogOpen() = false;
                if (onButton) onButton (toIndex (code, n));
            });
        }
    }

    //==========================================================================
    /** Load `file` of `kind`, asking first if its channel list differs. */
    inline void confirmThenLoad (Context ctx, const juce::File& file, Kind kind, std::function<void()> doLoad)
    {
        using namespace detail;
        if (ctx.parameters == nullptr || ! doLoad) return;
        auto& fm  = ctx.parameters->getFileManager();
        auto& vts = ctx.parameters->getValueTreeState();

        const Diff diff = fm.preflightChannelIdentity (file, kind);
        if (fm.isChannelIdentitySafe (diff, kind))
        {
            doLoad();
            return;
        }
        if (dialogOpen())
        {
            status (ctx, L ("cancelled"));
            return;
        }

        const juce::String fileName = file.getFileName();
        juce::String body = L ("intro").replace ("{file}", fileName) + "\n\n";

        Fix fix;
        if (diff.relation == Rel::fileHasNoIdentity)
            body += noIdentityText (diff, vts, fileName);
        else
        {
            fix = pickFix (diff, vts);
            if (fix.kind != Fix::none)
                body += fix.offerText + "\n\n";
            const juce::String cons = consequences (diff, kind);
            body += (diff.relation == Rel::identical ? L ("patchOnly") : L ("loadAnyway"))
                    + "\n" + (cons.isEmpty() ? juce::String ("  -") : cons);
        }
        body += "\n\n" + L ("notUndoable");

        juce::StringArray buttons;
        if (fix.kind != Fix::none) buttons.add (fix.buttonLabel);
        buttons.add (L (diff.relation == Rel::identical ? "proceed" : "loadAnywayButton"));
        buttons.add (LOC ("common.cancel"));

        show (ctx, L ("title"), body, buttons,
              [ctx, file, kind, doLoad, fix, hasFix = fix.kind != Fix::none] (int index)
              {
                  auto& fm2  = ctx.parameters->getFileManager();
                  auto& vts2 = ctx.parameters->getValueTreeState();
                  const int fixIdx = hasFix ? 0 : -1, loadIdx = hasFix ? 1 : 0;

                  if (index == fixIdx)
                  {
                      const auto r = applyFix (fix, vts2, "reconciled to " + file.getFileName());
                      if (r.failed())
                      {
                          status (ctx, L ("fixFailed").replace ("{error}", r.getErrorMessage()));
                          return;
                      }
                      if (ctx.afterStructuralChange) ctx.afterStructuralChange();

                      // Re-check rather than trust: the fix must have made the
                      // load safe, or something the operator did not see is
                      // still in the way and gets its own dialog.
                      const Diff again = fm2.preflightChannelIdentity (file, kind);
                      if (! fm2.isChannelIdentitySafe (again, kind)
                          && again.relation != Rel::identical && again.relation != Rel::orderOnly)
                      {
                          status (ctx, L ("stillDiffers").replace ("{file}", file.getFileName()));
                          WFSLogger::getInstance().logWarning ("Channel identity still differs after reconciliation: "
                                                               + WFSFileManager::summariseChannelIdentityDiff (again));
                          return;
                      }
                      fm2.grantChannelIdentityClearance (file);
                      doLoad();
                  }
                  else if (index == loadIdx)
                  {
                      WFSLogger::getInstance().logWarning ("Load anyway confirmed for " + file.getFileName()
                                                           + " despite channel list mismatch");
                      fm2.grantChannelIdentityClearance (file);
                      doLoad();
                  }
                  else
                      status (ctx, L ("cancelled"));
              });
    }

    /** Complete project load: the pair is what can be wrong. */
    inline void confirmThenLoadProject (Context ctx, const juce::File& systemFile, const juce::File& inputsFile,
                                        std::function<void()> doLoad)
    {
        using namespace detail;
        if (ctx.parameters == nullptr || ! doLoad) return;
        auto& fm = ctx.parameters->getFileManager();

        const Diff diff = fm.preflightProjectChannelIdentity (systemFile, inputsFile);
        if (fm.isChannelIdentitySafe (diff, Kind::projectPair))
        {
            doLoad();
            return;
        }
        if (dialogOpen())
        {
            status (ctx, L ("cancelled"));
            return;
        }

        const juce::String cons = consequences (diff, Kind::projectPair);
        const juce::String body = L ("pairMismatch") + "\n" + (cons.isEmpty() ? juce::String ("  -") : cons)
                                  + "\n\n" + L ("pairAdvice") + "\n\n" + L ("notUndoable");

        show (ctx, L ("title"), body, { L ("loadAnywayButton"), LOC ("common.cancel") },
              [ctx, systemFile, doLoad] (int index)
              {
                  if (index == 0)
                  {
                      WFSLogger::getInstance().logWarning ("Load anyway confirmed for project despite system/inputs channel list mismatch");
                      ctx.parameters->getFileManager().grantChannelIdentityClearance (systemFile);
                      doLoad();
                  }
                  else
                      status (ctx, L ("cancelled"));
              });
    }

    /** Manual snapshot recall: warn when the stored fingerprint disagrees with
        the live patch, offer the hardware-derived renumber when one exists.
        Cue-driven recalls do not come through here - a cue must never block. */
    inline void confirmThenRecall (Context ctx, const juce::String& snapshotName, std::function<void()> doRecall)
    {
        using namespace detail;
        if (ctx.parameters == nullptr || ! doRecall) return;
        auto& fm  = ctx.parameters->getFileManager();
        auto& vts = ctx.parameters->getValueTreeState();

        const Diff diff = fm.preflightSnapshotChannelIdentity (snapshotName);
        if (diff.patchDiffers.empty())
        {
            doRecall();
            return;
        }
        if (dialogOpen())
        {
            status (ctx, L ("cancelled"));
            return;
        }

        juce::StringArray lines;
        for (const auto& p : diff.patchDiffers)
            lines.add (L ("linePatch")
                           .replace ("{channel}", describeInputChannel (p.live))
                           .replace ("{file}", hwText (p.fileHwInputs))
                           .replace ("{live}", hwText (p.live.hwInputs)));

        juce::String body = LOC ("inputs.dialogs.snapshotIdentity.intro").replace ("{name}", snapshotName)
                            + "\n" + capped (lines) + "\n\n";

        const bool hasFix = diff.hardwareRelabel.has_value();
        if (hasFix)
            body += L ("relabelOfferHw") + "\n" + pairLines (vts, *diff.hardwareRelabel) + "\n\n";
        else
            body += LOC ("inputs.dialogs.snapshotIdentity.noFix") + "\n\n";
        body += LOC ("inputs.dialogs.snapshotIdentity.proceedNote");

        juce::StringArray buttons;
        if (hasFix) buttons.add (LOC ("inputs.dialogs.snapshotIdentity.fixNumbers"));
        buttons.add (L ("proceed"));
        buttons.add (LOC ("common.cancel"));

        show (ctx, LOC ("inputs.dialogs.snapshotIdentity.title"), body, buttons,
              [ctx, snapshotName, doRecall, hasFix, numbers = diff.hardwareRelabel.value_or (std::vector<int>())] (int index)
              {
                  const int fixIdx = hasFix ? 0 : -1, goIdx = hasFix ? 1 : 0;
                  if (index == fixIdx)
                  {
                      const auto r = ctx.parameters->getValueTreeState().assignInputChannelNumbersBySlot (
                          numbers, "matched to snapshot " + snapshotName + " by hardware inputs");
                      if (r.failed())
                      {
                          status (ctx, L ("fixFailed").replace ("{error}", r.getErrorMessage()));
                          return;
                      }
                      if (ctx.afterStructuralChange) ctx.afterStructuralChange();
                      doRecall();
                  }
                  else if (index == goIdx)
                  {
                      WFSLogger::getInstance().logWarning ("Snapshot '" + snapshotName
                                                           + "' recalled despite a hardware-input fingerprint mismatch (operator confirmed)");
                      doRecall();
                  }
                  else
                      status (ctx, L ("cancelled"));
              });
    }

    /** Arrange window "From file...": adopt a saved channel list WITHOUT loading
        anything else. Identical -> says so. Order -> rearrange. Numbers ->
        renumber (by position or by hardware fingerprint). Conflicting -> the
        list of differences and nothing else, because adopting a conflicting
        list by number is the crossing itself. */
    inline void reconcileFromFile (Context ctx, const juce::File& file)
    {
        using namespace detail;
        if (ctx.parameters == nullptr) return;
        auto& fm  = ctx.parameters->getFileManager();
        auto& vts = ctx.parameters->getValueTreeState();

        juce::XmlDocument doc (file);
        std::unique_ptr<juce::XmlElement> root (doc.getDocumentElement (true));
        Kind kind;
        if (root != nullptr && root->hasTagName ("SystemConfig"))      kind = Kind::systemConfig;
        else if (root != nullptr && root->hasTagName ("InputConfig"))  kind = Kind::inputConfig;
        else
        {
            status (ctx, L ("notAConfig").replace ("{file}", file.getFileName()));
            return;
        }

        const Diff diff = fm.preflightChannelIdentity (file, kind);
        const juce::String fileName = file.getFileName();

        if (diff.relation == Rel::identical)
        {
            status (ctx, L ("alreadyMatches").replace ("{file}", fileName));
            return;
        }
        if (diff.relation == Rel::fileHasNoIdentity)
        {
            status (ctx, L ("noIdentityShort").replace ("{file}", fileName));
            return;
        }
        if (dialogOpen())
            return;

        const Fix fix = pickFix (diff, vts);
        if (fix.kind == Fix::none)
        {
            // Conflicting: show, do not adopt.
            show (ctx, L ("title"),
                  L ("intro").replace ("{file}", fileName) + "\n\n" + L ("conflictOnly") + "\n"
                      + consequences (diff, kind),
                  { LOC ("common.ok") }, nullptr);
            return;
        }

        show (ctx, L ("title"),
              L ("intro").replace ("{file}", fileName) + "\n\n" + fix.offerText + "\n\n" + L ("notUndoable"),
              { fix.buttonLabel, LOC ("common.cancel") },
              [ctx, file, fix] (int index)
              {
                  if (index != 0)
                  {
                      status (ctx, L ("cancelled"));
                      return;
                  }
                  const auto r = applyFix (fix, ctx.parameters->getValueTreeState(), "arranged from " + file.getFileName());
                  if (r.failed())
                  {
                      status (ctx, L ("fixFailed").replace ("{error}", r.getErrorMessage()));
                      return;
                  }
                  if (ctx.afterStructuralChange) ctx.afterStructuralChange();
                  status (ctx, L ("fixed").replace ("{file}", file.getFileName()));
              });
    }
}
