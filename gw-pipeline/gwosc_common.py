#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

import requests


REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_ROOT = REPO_ROOT / "data" / "gwosc"
CACHE_ROOT = DATA_ROOT / "cache"
DETAILS_ROOT = CACHE_ROOT / "details"
PARAMETERS_ROOT = CACHE_ROOT / "parameters"
SKYMAPS_ROOT = CACHE_ROOT / "skymaps"
RESOLVED_ROOT = CACHE_ROOT / "resolved"


def ensure_layout() -> None:
    for path in (
        DATA_ROOT,
        CACHE_ROOT,
        DETAILS_ROOT,
        PARAMETERS_ROOT,
        SKYMAPS_ROOT,
        RESOLVED_ROOT,
    ):
        path.mkdir(parents=True, exist_ok=True)


def read_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)
        handle.write("\n")


def slugify_event_name(name: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "-_." else "_" for ch in name)


def url_digest(url: str) -> str:
    return hashlib.sha1(url.encode("utf-8")).hexdigest()[:16]


def fetch_json(url: str, *, timeout: float = 30.0) -> Any:
    url = url.replace("format=api", "format=json")
    response = requests.get(
        url,
        timeout=timeout,
        headers={"User-Agent": "GTR-gwosc-pipeline/1.0"},
    )
    response.raise_for_status()
    return response.json()


def download_binary(url: str, destination: Path, *, timeout: float = 120.0) -> Path:
    destination.parent.mkdir(parents=True, exist_ok=True)

    with requests.get(
        url,
        timeout=timeout,
        stream=True,
        headers={"User-Agent": "GTR-gwosc-pipeline/1.0"},
    ) as response:
        response.raise_for_status()
        with destination.open("wb") as handle:
            for chunk in response.iter_content(chunk_size=1024 * 1024):
                if chunk:
                    handle.write(chunk)

    return destination
