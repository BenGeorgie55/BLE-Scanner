#!/usr/bin/env python3
"""
Generate company_ids_generated.h from Nordic Semiconductor's Bluetooth Numbers DB.

Usage:
  python3 generate_company_db.py

or with an already-downloaded JSON file:
  python3 generate_company_db.py --input company_ids.json

The generated C++ table is compact enough for a normal 4 MB ESP32 build in
addition to the project's curated smart-glasses/known-device tables.
"""

import argparse
import json
import pathlib
import urllib.request

DEFAULT_URL = (
    "https://raw.githubusercontent.com/NordicSemiconductor/"
    "bluetooth-numbers-database/master/v1/company_ids.json"
)


def escape_cpp(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", help="Local company_ids.json instead of downloading")
    parser.add_argument("--output", default="company_ids_generated.h")
    args = parser.parse_args()

    if args.input:
        data = json.loads(pathlib.Path(args.input).read_text(encoding="utf-8"))
        source = str(pathlib.Path(args.input))
    else:
        with urllib.request.urlopen(DEFAULT_URL, timeout=30) as response:
            data = json.loads(response.read().decode("utf-8"))
        source = DEFAULT_URL

    rows = sorted(
        ({"code": int(row["code"]), "name": str(row["name"])} for row in data),
        key=lambda row: row["code"],
    )

    lines = [
        "#pragma once",
        "#include <stdint.h>",
        "",
        "// AUTO-GENERATED. Do not hand-edit if you intend to regenerate.",
        f"// Source: {source}",
        "struct GeneratedCompanyIdEntry { uint16_t companyId; const char* companyName; };",
        "static const GeneratedCompanyIdEntry GENERATED_COMPANY_IDS[] = {",
    ]
    for row in rows:
        if not 0 <= row["code"] <= 0xFFFE:
            continue
        lines.append(
            f'    {{ 0x{row["code"]:04X}, "{escape_cpp(row["name"])}" }},'
        )
    lines.extend(
        [
            "    { 0xFFFF, nullptr }",
            "};",
            "#define GENERATED_COMPANY_ID_COUNT ((uint16_t)((sizeof(GENERATED_COMPANY_IDS) / sizeof(GENERATED_COMPANY_IDS[0])) - 1U))",
            "",
        ]
    )

    pathlib.Path(args.output).write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {args.output} with {len(rows)} source entries")


if __name__ == "__main__":
    main()
