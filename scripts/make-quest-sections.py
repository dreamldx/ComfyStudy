#!/usr/bin/env python3
"""Write section 10 ("支线") of every chapter, and split the old section 9.

Every chapter ends with
  ## 9. 练习
  ## 10. 支线        (the list, or "本章节没有可以开启的支线。" when there is nothing to list)
  ## 11. 下一章预告

Section 10 of a mainline chapter lists the quest chapters that can be started right after it (the
"可开始于" column of docs/contrib/reference/quest-links.md). Section 10 of a quest chapter lists the
quest chapters of other quests that need it first (hard prerequisites named in the notes column:
"依赖 ...", "需先完成支线 ...", "用 ... 的 ...").

The script is idempotent: run it again after the table changes.
usage: make-quest-sections.py [--check]
"""
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
LINKS = DOCS / "contrib" / "reference" / "quest-links.md"
REF_DIR = LINKS.parent
CHECK = "--check" in sys.argv
NEXT_RE = re.compile(r"^(下一步|下一章|下一阶段|\*\*阶段 \d+ 到这里结束)", re.M)
REF = re.compile(r"(diffusion|llm|platform|paper)-0*(\d+)\.(\d+)(?:\s*(?:–|至)\s*(?:(?:diffusion|llm|platform)-0*\d+\.)?(\d+))?")


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


def goal_of(path: Path) -> str:
    m = re.search(r"^- 一句话目标：(.*)$", read(path), re.M)
    return m[1].strip() if m else ""


def load_rows():
    text = read(LINKS)
    sec = text.split("## 章节衔接", 1)[1].split("\n## ", 1)[0]
    rows = []
    for line in sec.split("\n"):
        if not line.startswith("| ["):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
        m = re.match(r"\[([a-z]+-\d+\.\d+) (.*?)\]\((.*?)\)", cells[0])
        main = re.search(r"\]\((.*?)\)", cells[2])
        rows.append({
            "id": m[1], "title": m[2], "file": (REF_DIR / m[3]).resolve(),
            "quest": cells[1], "main": (REF_DIR / main[1]).resolve() if main else None,
            "note": cells[3] if len(cells) > 3 else "",
        })
    return rows


def quest_name(folder: str) -> str:
    m = re.match(r"quest-([a-z]+)-(\d+)-", folder)
    return f"{m[1]}-{m[2]}"


def prerequisites(note: str):
    """Quest chapter ids that the note names as hard prerequisites."""
    out = []
    for seg in re.split(r"[；;]", note):
        seg = seg.strip()
        if not seg.startswith(("依赖", "需先完成", "用 ")):
            continue
        for m in REF.finditer(seg):
            d, q, a, b = m[1], int(m[2]), int(m[3]), m[4]
            for n in range(a, int(b) + 1) if b else [a]:
                out.append(f"{d}-{q}.{n}")
    return out


def entry(row, from_file: Path) -> str:
    rel = os.path.relpath(row["file"], from_file.parent).replace("\\", "/")
    text = f"- [{row['id']} {row['title']}]({rel})（支线 {quest_name(row['file'].parent.name)}：{row['quest']}）"
    goal = goal_of(row["file"])
    if goal:
        text += f"：{goal}"
    if row["note"]:
        text += f"（{row['note']}）"
    return text


def build():
    rows = load_rows()
    by_main, by_prereq = {}, {}
    ids = {r["id"]: r for r in rows}
    for r in rows:
        if r["main"]:
            by_main.setdefault(r["main"], []).append(r)
        for p in prerequisites(r["note"]):
            if p in ids and quest_name(ids[p]["file"].parent.name) != quest_name(r["file"].parent.name):
                by_prereq.setdefault(ids[p]["file"], []).append(r)
    return by_main, by_prereq


def split_sections(text: str):
    """(head, practice, quest_section, next_section) of a chapter text."""
    m = re.search(r"^## 9\. 练习[^\n]*\n", text, re.M)
    head, body = text[:m.start()], text[m.end():]
    parts = re.split(r"^## (10|11)\. [^\n]*\n", body, flags=re.M)
    first = parts[0]
    named = {parts[i]: parts[i + 1] for i in range(1, len(parts) - 1, 2)}
    if "11" in named:                                  # already converted
        return head, first.strip("\n"), named.get("10"), named["11"].strip("\n")
    first = re.split(r"^### 可选延伸（支线）", first, flags=re.M)[0]   # the old hand-made list
    m2 = NEXT_RE.search(first)
    if m2:
        return head, first[:m2.start()].strip("\n"), None, first[m2.start():].strip("\n")
    return head, first.strip("\n"), None, "待写。"


def main() -> int:
    by_main, by_prereq = build()
    files = sorted(list((DOCS / "tutorial").glob("phase-*/[0-9]*.md")) + list((DOCS / "contrib").glob("quest-*/*-[0-9]*.md")))
    changed = 0
    for f in files:
        if f.name == "README.md":
            continue
        text = read(f)
        if "## 9. 练习" not in text:
            continue
        head, practice, _old10, nxt = split_sections(text)
        items = by_main.get(f.resolve(), []) if f.parent.parent.name == "tutorial" else by_prereq.get(f.resolve(), [])
        parts = [head.rstrip("\n") + "\n", "## 9. 练习\n", practice + "\n"]
        if items:
            if f.parent.parent.name == "tutorial":
                intro = "做到这里，下面这些支线内容就可以开始了（可选，主线不依赖它们）："
            else:
                intro = "下面这些其他支线的章节需要先做完本章："
            parts += ["## 10. 支线\n", intro + "\n", "\n".join(entry(r, f) for r in items) + "\n"]
        else:
            parts += ["## 10. 支线\n", "本章节没有可以开启的支线。\n"]
        parts += ["## 11. 下一章预告\n", nxt + "\n"]
        new = "\n".join(parts)
        if new != text:
            changed += 1
            if not CHECK:
                f.write_text(new, encoding="utf-8", newline="")
    print(("would change" if CHECK else "changed"), changed, "of", len(files), "chapters")
    return 1 if CHECK and changed else 0


if __name__ == "__main__":
    sys.exit(main())
