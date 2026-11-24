#!/usr/bin/env python3
"""Generate mkdocs.generated.yml (the site navigation) from the tutorial and quest phase READMEs.

The phase README tables are the single source for chapter order and titles, so the
navigation never drifts from them. Chapters are listed in table order, which avoids the
lexicographic problem of file names ("11.10" sorts before "11.2").

Usage: python scripts/make-site-nav.py
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
TUT = DOCS / "tutorial"
CON = DOCS / "contrib"
OUT = ROOT / "mkdocs.generated.yml"

ROW = re.compile(r"^\|\s*((?:diffusion-|llm-|platform-|paper-)?\d+\.\d+)\s*\|\s*\[(.+?)\]\((.+?\.md)\)")


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8").replace("\r\n", "\n")


def h1(path: Path) -> str:
    for line in read(path).split("\n"):
        if line.startswith("# "):
            return line[2:].strip()
    raise SystemExit(f"{path}: no level-1 heading")


def q(text: str) -> str:
    # JSON strings are valid YAML scalars and handle quotes and CJK safely.
    # Backticks are dropped: nav labels are plain text, not Markdown.
    return json.dumps(text.replace("`", ""), ensure_ascii=False)


def phase_entry(folder: Path, indent: str) -> list[str]:
    readme = folder / "README.md"
    rel = f"{folder.parent.name}/{folder.name}"
    lines = [f"{indent}- {q(h1(readme))}:", f"{indent}    - {q(rel + '/README.md')}"]
    count = 0
    for line in read(readme).split("\n"):
        m = ROW.match(line)
        if not m:
            continue
        number, title, target = m.groups()
        if not (folder / target).is_file():
            raise SystemExit(f"{readme}: chapter file not found: {target}")
        lines.append(f"{indent}    - {q(number + ' ' + title)}: {q(rel + '/' + target)}")
        count += 1
    if count == 0:
        raise SystemExit(f"{readme}: no chapter rows found")
    return lines


def main() -> int:
    def number(d: Path) -> int:
        return int(re.search(r"-(\d+)-", d.name).group(1))

    main_phases = sorted((d for d in TUT.iterdir() if d.is_dir() and d.name.startswith("phase-")), key=number)
    diffusion = sorted((d for d in CON.iterdir() if d.is_dir() and d.name.startswith("quest-diffusion-")), key=number)
    llm = sorted((d for d in CON.iterdir() if d.is_dir() and d.name.startswith("quest-llm-")), key=number)
    platform = sorted((d for d in CON.iterdir() if d.is_dir() and d.name.startswith("quest-platform-")), key=number)
    paper = sorted((d for d in CON.iterdir() if d.is_dir() and d.name.startswith("quest-paper-")), key=number)
    quest = diffusion + llm + platform + paper

    out = ["INHERIT: mkdocs.yml", "nav:", f"  - {q('首页')}: {q('index.md')}"]
    out.append(f"  - {q('主线')}:")
    for d in main_phases:
        out += phase_entry(d, "    ")
    out.append(f"  - {q('支线')}:")
    out.append(f"    - {q('支线总览')}: {q('contrib/README.md')}")
    out.append(f"    - {q('扩散模型方向')}:")
    for d in diffusion:
        out += phase_entry(d, "      ")
    out.append(f"    - {q('LLM 方向')}:")
    for d in llm:
        out += phase_entry(d, "      ")
    out.append(f"    - {q('platform 方向（Linux 版）')}:")
    for d in platform:
        out += phase_entry(d, "      ")
    out.append(f"    - {q('paper 方向（论文解析）')}:")
    for d in paper:
        out += phase_entry(d, "      ")
    reference = sorted((TUT / "reference").glob("*.md")) + sorted((CON / "reference").glob("*.md"))
    out.append(f"  - {q('参考')}:")
    for p in reference:
        out.append(f"    - {q(h1(p))}: {q(p.parent.parent.name + '/reference/' + p.name)}")

    OUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {OUT.name}: {len(main_phases)} mainline phases, {len(quest)} quest phases, {len(reference)} reference pages")
    return 0


if __name__ == "__main__":
    sys.exit(main())
