from __future__ import annotations

from dataclasses import asdict, dataclass, fields
from math import isfinite
from pathlib import Path
from typing import Any, Mapping
import json


class MotorParameterError(ValueError):
    """Raised when a motor parameter set fails validation."""


@dataclass
class MotorParameters:
    """
    Runtime-editable motor parameter set for BLDC/PMSM twin configuration.

    The class is intentionally simulation-agnostic. It only owns parameter
    storage, validation, and JSON serialization.
    """

    Rs: float
    Ld: float
    Lq: float
    PolePairs: int
    FluxLinkage: float
    Kt: float
    Ke: float
    RotorInertia: float
    Friction: float
    LoadTorque: float
    Vdc: float

    def __post_init__(self) -> None:
        self.validate()

    @classmethod
    def defaults(cls) -> "MotorParameters":
        return cls(
            Rs=0.5,
            Ld=0.001,
            Lq=0.001,
            PolePairs=4,
            FluxLinkage=0.02,
            Kt=0.1,
            Ke=0.1,
            RotorInertia=0.01,
            Friction=0.1,
            LoadTorque=0.0,
            Vdc=24.0,
        )

    @classmethod
    def from_dict(cls, data: Mapping[str, Any]) -> "MotorParameters":
        missing = [field.name for field in fields(cls) if field.name not in data]
        if missing:
            raise MotorParameterError(f"missing motor parameters: {', '.join(missing)}")
        return cls(
            Rs=float(data["Rs"]),
            Ld=float(data["Ld"]),
            Lq=float(data["Lq"]),
            PolePairs=int(data["PolePairs"]),
            FluxLinkage=float(data["FluxLinkage"]),
            Kt=float(data["Kt"]),
            Ke=float(data["Ke"]),
            RotorInertia=float(data["RotorInertia"]),
            Friction=float(data["Friction"]),
            LoadTorque=float(data["LoadTorque"]),
            Vdc=float(data["Vdc"]),
        )

    @classmethod
    def load_json(cls, path: str | Path) -> "MotorParameters":
        with Path(path).open("r", encoding="utf-8") as handle:
            data = json.load(handle)
        if not isinstance(data, dict):
            raise MotorParameterError("motor parameter file must contain a JSON object")
        return cls.from_dict(data)

    def save_json(self, path: str | Path) -> None:
        self.validate()
        target = Path(path)
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("w", encoding="utf-8") as handle:
            json.dump(asdict(self), handle, indent=2, sort_keys=True)
            handle.write("\n")

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)

    def update(self, **changes: Any) -> None:
        for key, value in changes.items():
            if not hasattr(self, key):
                raise MotorParameterError(f"unknown motor parameter: {key}")
            setattr(self, key, value)
        self.validate()

    def validate(self) -> None:
        self._require_finite_non_negative("Rs", self.Rs)
        self._require_finite_non_negative("Ld", self.Ld)
        self._require_finite_non_negative("Lq", self.Lq)
        self._require_positive_int("PolePairs", self.PolePairs)
        self._require_finite_non_negative("FluxLinkage", self.FluxLinkage)
        self._require_finite_non_negative("Kt", self.Kt)
        self._require_finite_non_negative("Ke", self.Ke)
        self._require_finite_non_negative("RotorInertia", self.RotorInertia)
        self._require_finite_non_negative("Friction", self.Friction)
        self._require_finite("LoadTorque", self.LoadTorque)
        self._require_finite_positive("Vdc", self.Vdc)

    @staticmethod
    def _require_finite(name: str, value: float) -> None:
        if not isinstance(value, (int, float)) or not isfinite(float(value)):
            raise MotorParameterError(f"{name} must be a finite number")

    @classmethod
    def _require_finite_non_negative(cls, name: str, value: float) -> None:
        cls._require_finite(name, value)
        if float(value) < 0.0:
            raise MotorParameterError(f"{name} must be greater than or equal to 0")

    @classmethod
    def _require_finite_positive(cls, name: str, value: float) -> None:
        cls._require_finite(name, value)
        if float(value) <= 0.0:
            raise MotorParameterError(f"{name} must be greater than 0")

    @staticmethod
    def _require_positive_int(name: str, value: int) -> None:
        if not isinstance(value, int):
            raise MotorParameterError(f"{name} must be an integer")
        if value <= 0:
            raise MotorParameterError(f"{name} must be greater than 0")

