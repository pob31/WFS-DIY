#!/usr/bin/env python3
"""Regenerate the translation-proofreading checklists in Documentation/proofreading.

Run from the repo root:  python tools/gen_proofreading_checklists.py

The checklists list every translated *prose* key (help text, tooltips, status /
dialog messages, dialog and help-card titles) with the English source and the
locale's translation, plus a sign-off checkbox. UI control labels and domain
terms live only in en.json and are English by design, so they are not listed.

Logic (see Documentation/proofreading/README.md):
  * The prose-key set is the union of keys present across the locale files
    (Resources/lang/<locale>.json), intersected with en.json so every listed
    entry has an English source.
  * For each locale, emit the English source and the translation, or
    "(missing - falls back to English)" where the locale omits a key.
  * Group by a 2-level-capped namespace (keys 3+ deep group under their first
    two components; the leaf is the remainder). Sections and entries sort
    alphabetically.
  * Each file's How-to-use preamble is preserved verbatim; only the "Total keys"
    count is refreshed and the entry body is regenerated.

Line endings follow the working tree (CRLF via the default text-mode write;
git normalises to LF on commit under core.autocrlf).
"""

import json
import re
import pathlib

REPO = pathlib.Path(__file__).resolve().parents[1]
LANG = REPO / "Resources" / "lang"
PROOF = REPO / "Documentation" / "proofreading"

LOCALES = ["fr", "de", "es", "it", "pt", "ja", "ko", "zh"]
MISSING = "(missing — falls back to English)"


def flatten(d, prefix=""):
    out = {}
    for k, v in d.items():
        p = f"{prefix}.{k}" if prefix else k
        if isinstance(v, dict):
            out.update(flatten(v, p))
        else:
            out[p] = v
    return out


def esc(v):
    # Values render on one line; real newlines/tabs become their literal escape.
    s = str(v)
    return (s.replace("\r\n", "\n").replace("\r", "\n")
             .replace("\n", "\\n").replace("\t", "\\t"))


def section_of(k):
    parts = k.split(".")
    return ".".join(parts[:2]) if len(parts) > 2 else ".".join(parts[:-1])


def leaf_of(k):
    return k[len(section_of(k)) + 1:]


def main():
    en = flatten(json.loads((LANG / "en.json").read_text(encoding="utf-8")))
    loc_maps = {l: flatten(json.loads((LANG / f"{l}.json").read_text(encoding="utf-8")))
                for l in LOCALES}

    union = set().union(*[set(m) for m in loc_maps.values()])
    canonical = sorted(k for k in union if k in en)

    sections = {}
    for k in canonical:
        sections.setdefault(section_of(k), []).append(k)
    for s in sections:
        sections[s].sort(key=leaf_of)
    ordered = sorted(sections)

    total = len(canonical)
    for loc in LOCALES:
        m = loc_maps[loc]
        # Preserve the preamble up to the first backtick-quoted section header.
        text = (PROOF / f"{loc}.md").read_text(encoding="utf-8")
        cut = re.search(r"^## `", text, flags=re.M)
        pre = (text[:cut.start()] if cut else text).rstrip()
        pre = re.sub(r"Total keys:\s*\d+", f"Total keys: {total}", pre, count=1)

        body = []
        for s in ordered:
            body.append(f"## `{s}`\n")
            for k in sections[s]:
                body.append(f"- **`{leaf_of(k)}`**")
                body.append(f"  - EN: {esc(en[k])}")
                body.append(f"  - {loc.upper()}: {esc(m[k]) if k in m else MISSING}")
                body.append("  - [ ] OK    Fix: ")
                body.append("")
        content = pre + "\n\n" + "\n".join(body).rstrip() + "\n"
        (PROOF / f"{loc}.md").write_text(content, encoding="utf-8")

        missing = sum(1 for k in canonical if k not in m)
        print(f"{loc}: {total - missing} translated, {missing} missing")

    print(f"canonical prose keys: {total}")


if __name__ == "__main__":
    main()
