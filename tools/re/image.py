"""Helpers for a dumped guest image (nfsc.dump_image): the image as loaded at REX_IMAGE_BASE.

The SDK does not keep the PE header in guest memory, so the code range comes from the
generated code (REX_CODE_BASE / REX_CODE_SIZE in <project>_pch.h).
"""
import pathlib
import re

BASE = 0x82000000


def code_range(generated):
    """(start, end) of the recompiled code, from the generated folder or a file in it."""
    folder = pathlib.Path(generated)
    if folder.is_file():
        folder = folder.parent
    headers = sorted(folder.glob("*_pch.h"))
    if not headers:
        raise ValueError(f"no *_pch.h in {folder}")
    text = headers[0].read_text(encoding="utf-8", errors="replace")
    values = {}
    for name in ("REX_CODE_BASE", "REX_CODE_SIZE"):
        m = re.search(rf"#define {name} (0x[0-9A-Fa-f]+)", text)
        if not m:
            raise ValueError(f"{name} not in {headers[0]}")
        values[name] = int(m.group(1), 16)
    return values["REX_CODE_BASE"], values["REX_CODE_BASE"] + values["REX_CODE_SIZE"]
