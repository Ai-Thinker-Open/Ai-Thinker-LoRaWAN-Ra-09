#!/usr/bin/env python3
"""Validate product entry points, documentation pairs and IDE references."""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa"

PAIRS = [
    (ROOT / "README.md", ROOT / "README.zh.md", "中文-README-blue", "English-README-green"),
    (ROOT / "docs/CODE_ENTRY.md", ROOT / "docs/CODE_ENTRY.zh.md", "中文-文档-blue", "English-Docs-green"),
    (ROOT / "docs/ARCHITECTURE.md", ROOT / "docs/ARCHITECTURE.zh.md", "中文-文档-blue", "English-Docs-green"),
    (ROOT / "docs/VALIDATION.md", ROOT / "docs/VALIDATION.zh.md", "中文-文档-blue", "English-Docs-green"),
    (APP / "README.md", APP / "README.zh.md", "中文-文档-blue", "English-Docs-green"),
]

REQUIRED = [
    APP / "GNUmakefile",
    APP / "Core/Src/main.c",
    APP / "SubGHz_Phy/App/app_subghz_phy.c",
    APP / "SubGHz_Phy/App/subghz_phy_app.c",
    APP / "SubGHz_Phy/App/lora_transparent_at.c",
    APP / "SubGHz_Phy/App/subg_command.c",
    APP / "STM32CubeIDE/.project",
    APP / "STM32CubeIDE/.cproject",
    APP / "MDK-ARM/SubGHz_Phy_PingPong.uvprojx",
    ROOT / "Package_license.md",
]


def fail(message: str, errors: list[str]) -> None:
    errors.append(message)


def main() -> int:
    errors: list[str] = []

    for path in REQUIRED:
        if not path.is_file():
            fail(f"missing required file: {path.relative_to(ROOT)}", errors)

    for english, chinese, en_badge, zh_badge in PAIRS:
        for path, badge in ((english, en_badge), (chinese, zh_badge)):
            if not path.is_file():
                fail(f"missing documentation pair: {path.relative_to(ROOT)}", errors)
                continue
            first_line = path.read_text(encoding="utf-8").splitlines()[0]
            if badge not in first_line:
                fail(f"incorrect language badge: {path.relative_to(ROOT)}", errors)

    project_path = APP / "STM32CubeIDE/.project"
    if project_path.is_file():
        try:
            project = ET.parse(project_path)
        except ET.ParseError as exc:
            fail(f"invalid STM32CubeIDE .project XML: {exc}", errors)
        else:
            locations = "\n".join(node.text or "" for node in project.findall(".//locationURI"))
            for required_name in (
                "lora_transparent_at.c",
                "subg_command.c",
                "startup_stm32wl55xx_cm4.s",
                "STM32WL55JCIX_FLASH.ld",
            ):
                if required_name not in locations:
                    fail(f"STM32CubeIDE project omits {required_name}", errors)

    for path in ROOT.rglob("*"):
        if path.is_file() and "Zone.Identifier" in path.name:
            fail(f"Windows alternate-stream artifact: {path.relative_to(ROOT)}", errors)

    markdown_link = re.compile(r"(?<!!)\[[^]]+\]\(([^)]+)\)")
    for english, chinese, *_ in PAIRS:
        for path in (english, chinese):
            if not path.is_file():
                continue
            for target in markdown_link.findall(path.read_text(encoding="utf-8")):
                target = target.split("#", 1)[0]
                if not target or "://" in target or target.startswith("mailto:"):
                    continue
                if not (path.parent / target).resolve().exists():
                    fail(f"broken local link in {path.relative_to(ROOT)}: {target}", errors)

    if errors:
        print("Repository validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"Repository validation passed: {len(PAIRS)} bilingual documentation pairs checked.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
