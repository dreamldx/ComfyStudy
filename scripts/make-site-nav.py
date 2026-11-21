#!/usr/bin/env python3
"""Generate mkdocs.generated.yml (the site navigation) from the tutorial phase READMEs.

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
DOCS = ROOT / "docs" / "tutorial"
OUT = ROOT / "mkdocs.generated.yml"

ROW = re.compile(r"^\|\s*(\d+\.\d+)\s*\|\s*\[(.+?)\]\((.+?\.md)\)")


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
    lines = [f"{indent}- {q(h1(readme))}:", f"{indent}    - {q(folder.name + '/README.md')}"]
    count = 0
    for line in read(readme).split("\n"):
        m = ROW.match(line)
        if not m:
            continue
        number, title, target = m.groups()
        if not (folder / target).is_file():
            raise SystemExit(f"{readme}: chapter file not found: {target}")
        lines.append(f"{indent}    - {q(number + ' ' + title)}: {q(folder.name + '/' + target)}")
        count += 1
    if count == 0:
        raise SystemExit(f"{readme}: no chapter rows found")
    return lines


def main() -> int:
    phases = sorted(
        (d for d in DOCS.iterdir() if d.is_dir() and re.match(r"phase-\d\d-", d.name)),
        key=lambda d: int(d.name.split("-")[1]),
    )
    main_phases = [d for d in phases if int(d.name.split("-")[1]) < 17]
    appendix = [d for d in phases if int(d.name.split("-")[1]) >= 17]

    out = ["INHERIT: mkdocs.yml", "nav:", f"  - {q('首页')}: {q('README.md')}"]
    out.append(f"  - {q('主线')}:")
    for d in main_phases:
        out += phase_entry(d, "    ")
    out.append(f"  - {q('选读附录')}:")
    for d in appendix:
        out += phase_entry(d, "    ")
    reference = sorted((DOCS / "reference").glob("*.md"))
    out.append(f"  - {q('参考')}:")
    for p in reference:
        out.append(f"    - {q(h1(p))}: {q('reference/' + p.name)}")

    OUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {OUT.name}: {len(main_phases)} mainline phases, {len(appendix)} appendix phases, {len(reference)} reference pages")
    return 0


if __name__ == "__main__":
    sys.exit(main())
