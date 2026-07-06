from __future__ import annotations

from dataclasses import dataclass, asdict
from datetime import datetime, timezone
from logging import Logger, getLogger, basicConfig, INFO
from threading import RLock
from typing import Any


def configure_logging() -> None:
    basicConfig(
        level=INFO,
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )


@dataclass(frozen=True)
class LogEntry:
    timestamp: str
    level: str
    event: str
    detail: dict[str, Any]


class EventLogBuffer:
    def __init__(self, capacity: int = 1000) -> None:
        self.capacity = capacity
        self._lock = RLock()
        self._entries: list[LogEntry] = []

    def append(self, level: str, event: str, detail: dict[str, Any] | None = None) -> None:
        entry = LogEntry(
            timestamp=datetime.now(timezone.utc).isoformat(),
            level=level,
            event=event,
            detail=detail or {},
        )
        with self._lock:
            self._entries.append(entry)
            if len(self._entries) > self.capacity:
                del self._entries[: len(self._entries) - self.capacity]

    def list(self) -> list[dict[str, Any]]:
        with self._lock:
            return [asdict(entry) for entry in self._entries]

