from __future__ import annotations

from pathlib import Path
from threading import RLock

from backend.app.models.motor_parameters import MotorParameterError, MotorParameters


class MotorParameterStore:
    """
    Thread-safe JSON-backed parameter store.

    This class allows runtime updates while keeping the persisted JSON file as
    the single source of truth.
    """

    def __init__(self, path: str | Path, initial: MotorParameters | None = None) -> None:
        self._path = Path(path)
        self._lock = RLock()
        self._parameters = initial or MotorParameters.defaults()

    def load(self) -> MotorParameters:
        with self._lock:
            if self._path.exists():
                self._parameters = MotorParameters.load_json(self._path)
            return self._parameters

    def save(self) -> None:
        with self._lock:
            self._parameters.save_json(self._path)

    def get(self) -> MotorParameters:
        with self._lock:
            return MotorParameters.from_dict(self._parameters.to_dict())

    def update(self, **changes) -> MotorParameters:
        with self._lock:
            self._parameters.update(**changes)
            self._parameters.save_json(self._path)
            return MotorParameters.from_dict(self._parameters.to_dict())

    def replace(self, parameters: MotorParameters) -> MotorParameters:
        with self._lock:
            if not isinstance(parameters, MotorParameters):
                raise MotorParameterError("parameters must be a MotorParameters instance")
            parameters.validate()
            self._parameters = MotorParameters.from_dict(parameters.to_dict())
            self._parameters.save_json(self._path)
            return MotorParameters.from_dict(self._parameters.to_dict())

