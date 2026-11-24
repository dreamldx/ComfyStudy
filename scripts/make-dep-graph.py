#!/usr/bin/env python3
"""Rebuild the dependency table on the site home page (docs/index.md).

Rows are the main-line phases in order; the columns are the quest directions (LLM, diffusion,
platform). A quest sits in the row of the latest main-line chapter it needs, that is, the earliest
point at which it can be started, and within a cell the quests are ordered by that chapter.
Under each quest: the quest branches it continues from ("接"), merges ("合入") or optionally uses
("可选").

The data comes from docs/contrib/reference/quest-links.md: the branch table ("支线分支") and
`<!-- edge: FROM TO KIND -->` comments (KIND is merge or optional) for dependencies the start
column does not express. Titles come from the READMEs.

usage: make-dep-graph.py [--check]   (--check only reports whether docs/index.md is up to date)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
INDEX = DOCS / "index.md"
LINKS = DOCS / "contrib" / "reference" / "quest-links.md"
START, END = "<!-- dep-graph:start -->", "<!-- dep-graph:end -->"
ROW = re.compile(r"^\| ([a-z]+-\d+) \| `[^`]+` \| (.*) \|$")
BRANCH = re.compile(r"分支 `quest/([a-z]+-\d+)-[^`]*` 的末端")
TAG = re.compile(r"`(?:chapter-(\d+)\.(\d+)-[^`]*|(phase-00)-toolchain)`")
COLUMNS = [("llm", "LLM 支线"), ("diffusion", "扩散模型支线"), ("platform", "platform 支线（Linux 版）"), ("paper", "paper 支线（论文解析）")]


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


def load():
    text = read(LINKS)
    table = text.split("## 支线分支", 1)[1].split("## 章节衔接", 1)[0]
    own, starts, merges, optional = {}, {}, {}, {}
    for row in table.split("\n"):
        m = ROW.match(row)
        if not m:
            continue
        name, start = m[1], m[2]
        first, _, merged = start.partition("再合入")
        tags = [(int(a), int(b)) if a else (0, 0) for a, b, _ in TAG.findall(first + merged)]
        if tags:
            own[name] = max(tags)
        starts[name] = BRANCH.findall(first)
        merges[name] = BRANCH.findall(merged)
    for a, b, kind in re.findall(r"<!-- edge: (\S+) (\S+) (merge|optional) -->", text):
        (merges if kind == "merge" else optional).setdefault(b, []).append(a)
    quests = {}
    for d in sorted((DOCS / "contrib").glob("quest-*")):
        m = re.match(r"quest-([a-z]+)-(\d+)-", d.name)
        title = re.sub(r"^支线 [a-z]+-\d+：", "", read(d / "README.md").split("\n", 1)[0].lstrip("# ").strip())
        quests[f"{m[1]}-{m[2]}"] = (m[1], title, d.name)
    phases = []
    for d in sorted((DOCS / "tutorial").glob("phase-*")):
        n = int(re.match(r"phase-(\d+)-", d.name)[1])
        title = re.sub(r"^阶段 \d+：", "", read(d / "README.md").split("\n", 1)[0].lstrip("# ").strip())
        phases.append((n, title, d.name))
    return phases, quests, own, starts, merges, optional


def earliest(name, own, starts):
    """The latest main-line chapter the quest needs, counting the quests it continues from."""
    best = own.get(name, (0, 0))
    for p in starts.get(name, []):
        best = max(best, earliest(p, own, starts))
    return best


def cell(name, quests, merges, optional, starts, at):
    g, title, folder = quests[name]
    line = f"[{name}](contrib/{folder}/README.md) {title}"
    notes = []
    if starts.get(name):
        notes.append("接 " + "、".join(starts[name]))
    if merges.get(name):
        notes.append("合入 " + "、".join(merges[name]))
    if optional.get(name):
        notes.append("可选 " + "、".join(optional[name]))
    notes.append(f"{at[0]}.{at[1]} 起")
    return f"{line}<br>{' · '.join(notes)}"


def table() -> str:
    phases, quests, own, starts, merges, optional = load()
    rows = ["| 主线阶段 | " + " | ".join(label for _, label in COLUMNS) + " |",
            "|---|" + "---|" * len(COLUMNS)]
    for n, title, folder in phases:
        cells = []
        for g, _ in COLUMNS:
            here = sorted((q for q, (gg, _, _) in quests.items() if gg == g and earliest(q, own, starts)[0] == n),
                          key=lambda q: (earliest(q, own, starts), q))
            cells.append("<br><br>".join(cell(q, quests, merges, optional, starts, earliest(q, own, starts)) for q in here) or "—")
        rows.append(f"| **[{n} {title}](tutorial/{folder}/README.md)** | " + " | ".join(cells) + " |")
    return "\n".join(rows)


def block() -> str:
    return (f"{START}\n\n"
            "每一行是一个主线阶段，按阶段顺序；右边三列是三个方向的支线。支线放在它**最早可以开始**的那一行：行是它要求做完的最晚主线阶段，"
            "格子里的“N.M 起”具体到哪一章，同一格里按这个顺序排列。支线名字下面写它和其他支线的关系：“接”是从对方分支的末端切出，"
            "“合入”是要把对方分支的提交合进来，“可选”是可选的依赖；从另一条支线接出来的，所在行不早于它所接的支线。"
            "主线不依赖任何支线。每个依赖的具体章节写在[支线与主线的衔接表](contrib/reference/quest-links.md)里。\n\n"
            + table() + "\n\n" + END)


def main() -> int:
    s = read(INDEX)
    if START not in s:
        s = s.rstrip("\n") + "\n\n## 依赖关系\n\n" + START + "\n" + END + "\n"
    new = re.sub(re.escape(START) + r".*?" + re.escape(END), lambda m: block(), s, count=1, flags=re.S)
    if "--check" in sys.argv:
        print("up to date" if new == s else "docs/index.md is out of date: run scripts/make-dep-graph.py")
        return 0 if new == s else 1
    INDEX.write_text(new, encoding="utf-8", newline="")
    print("updated docs/index.md")
    return 0


if __name__ == "__main__":
    sys.exit(main())
