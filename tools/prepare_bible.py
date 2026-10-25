#!/usr/bin/env python3
"""Convert JFAAL Almeida 1911 JSON into indexed microSD files."""
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path


def main() -> int:
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("data/1911-JFAAtualizadaLivre.json")
    output_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("data/sdcard")
    output_dir.mkdir(parents=True, exist_ok=True)

    bible = json.loads(source.read_text(encoding="utf-8"))
    books = bible["books"]
    if len(books) != 66:
        raise ValueError(f"Esperados 66 livros, encontrados {len(books)}")

    chapter_records: list[tuple[int, int, int, int]] = []
    verse_records: list[tuple[int, int]] = []
    text_path = output_dir / "BIBLIA.TXT"
    chapter_path = output_dir / "CAPITULOS.BIN"
    verse_path = output_dir / "VERSICULOS.BIN"
    # Write transactionally so a failed conversion never leaves mixed indexes.
    for path in (text_path, chapter_path, verse_path):
        path.unlink(missing_ok=True)
    with text_path.open("wb") as text_file:
        for book_index, book in enumerate(books):
            for chapter in book["chapters"]:
                if len(chapter["verses"]) > 0xFFFF or chapter["chapter"] > 0xFF:
                    raise ValueError(f"Capítulo inválido: {chapter.get('name')}")
                first_verse = len(verse_records)
                for verse in chapter["verses"]:
                    encoded = verse["text"].replace("\r", " ").replace("\n", " ").encode("utf-8")
                    if not encoded or len(encoded) > 0xFFFF:
                        raise ValueError(f"Versículo vazio ou muito longo: {verse.get('name')}")
                    offset = text_file.tell()
                    text_file.write(encoded)
                    text_file.write(b"\n")
                    verse_records.append((offset, len(encoded)))
                chapter_records.append((first_verse, len(chapter["verses"]), book_index, chapter["chapter"]))

    if len(chapter_records) > 0xFFFF or len(verse_records) > 0xFFFFFFFF:
        raise ValueError("Índice excede limites do firmware")

    with chapter_path.open("wb") as index_file:
        for first_verse, count, book_index, book_chapter in chapter_records:
            index_file.write(struct.pack("<IHBB", first_verse, count, book_index, book_chapter))

    with verse_path.open("wb") as index_file:
        for offset, length in verse_records:
            index_file.write(struct.pack("<IH", offset, length))

    print(f"Livros: {len(books)}; capítulos: {len(chapter_records)}; versículos: {len(verse_records)}")
    for path in sorted(output_dir.iterdir()):
        if path.is_file():
            print(f"{path}: {path.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
