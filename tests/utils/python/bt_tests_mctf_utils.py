# SPDX-FileCopyrightText: 2026 Philippe Proulx <pproulx@efficios.com>
# SPDX-License-Identifier: MIT

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import abc
import logging
import pathlib
import tempfile
from typing import (
    Any,
    Dict,
    List,
    Type,
    Tuple,
    ClassVar,
    Optional,
    FrozenSet,
)

import mctf
import pytest
import moultipart

_logger = logging.getLogger(__name__)


# Base class for exceptions which an `MCtfTestItem` subclass raises to
# signal a test failure.
#
# format_output() returns a human-readable failure description used
# by MCtfTestItem.repr_failure().
class MCtfTestError(Exception):
    def __init__(self, mctf_path: pathlib.Path) -> None:
        super().__init__()
        self._mctf_path = mctf_path

    @property
    def mctf_path(self) -> pathlib.Path:
        return self._mctf_path

    def format_output(self) -> str:
        raise NotImplementedError


# Raised by an `MCtfTestItem` subclass to signal that the actual
# output of a test didn't match the expected output.
#
# `expected_label` and `actual_label` are short descriptions appended
# to the `✅` and `❌` markers respectively (without a trailing colon).
class MCtfMismatchError(MCtfTestError):
    def __init__(
        self,
        mctf_path: pathlib.Path,
        expected_label: str,
        expected: str,
        actual_label: str,
        actual: str,
    ) -> None:
        super().__init__(mctf_path)
        self._expected_label = expected_label
        self._expected = expected
        self._actual_label = actual_label
        self._actual = actual

    def format_output(self) -> str:
        return "\n".join(
            [
                f"📄 Test file: `{self._mctf_path}`",
                "",
                f"✅ {self._expected_label}:",
                self._expected,
                "",
                f"❌ {self._actual_label}:",
                self._actual,
            ]
        )


# Raised by an `MCtfTestItem` subclass to signal that the graph was
# expected to raise an error but completed without raising one.
#
# `expected_label` is a short descriptions appended `✅` marker (without
# a trailing colon).
class MCtfNoErrorError(MCtfTestError):
    def __init__(
        self,
        mctf_path: pathlib.Path,
        expected_label: str,
        expected: str,
    ) -> None:
        super().__init__(mctf_path)
        self._expected_label = expected_label
        self._expected = expected

    def format_output(self) -> str:
        return "\n".join(
            [
                f"📄 Test file: `{self._mctf_path}`",
                "",
                f"✅ {self._expected_label}:",
                self._expected,
                "",
                "❌ Actual: the graph completed without raising an error",
            ]
        )


# Abstract base for a pytest test item built from a `.mctf` file.
#
# A subclass must set `_allowed_non_ctf_parts` (the set of `@`-prefixed
# header names accepted in the `.mctf` file) and may set
# `_required_non_ctf_parts` (those that must be present).
#
# The subclass implements _check() to run the actual assertion logic
# against the generated trace directory.
#
# See collect_mctf_file().
class MCtfTestItem(pytest.Item):
    # Allowed non-CTF parts (subclasses must override).
    _allowed_non_ctf_parts: ClassVar[FrozenSet[str]] = frozenset()

    # Required non-CTF parts (subclasses may override).
    _required_non_ctf_parts: ClassVar[FrozenSet[str]] = frozenset()

    def __init__(self, *, mctf_path: pathlib.Path, **kwargs: Any) -> None:
        super().__init__(**kwargs)  # pyright: ignore[reportUnknownMemberType]
        self._mctf_path = mctf_path

    @property
    def mctf_path(self) -> pathlib.Path:
        return self._mctf_path

    def _validate_parts(self, non_ctf_parts: Dict[str, moultipart.Part]) -> None:
        for header_info in non_ctf_parts:
            assert (
                header_info in self._allowed_non_ctf_parts
            ), f"Unexpected non-CTF part `{header_info}` in `{self._mctf_path}`"

        for required in self._required_non_ctf_parts:
            assert (
                required in non_ctf_parts
            ), f"Missing `{required}` part in `{self._mctf_path}`"

    def runtest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            trace_dir = pathlib.Path(tmp_dir) / self._mctf_path.stem
            non_ctf_parts = mctf.generate(str(self._mctf_path), str(trace_dir), False)
            self._validate_parts(non_ctf_parts)
            self._check(trace_dir, non_ctf_parts)

    # A subclass implements the actual assertion logic.
    #
    # `trace_dir` is the materialized CTF trace directory;
    # `non_ctf_parts` is the dictionary of `@`-prefixed parts read from
    # the `.mctf` file.
    @abc.abstractmethod
    def _check(
        self,
        trace_dir: pathlib.Path,
        non_ctf_parts: Dict[str, moultipart.Part],
    ) -> None: ...

    def reportinfo(self) -> Tuple[Any, None, str]:
        return self.path, None, self.name

    def repr_failure(
        self,
        excinfo: "pytest.ExceptionInfo[BaseException]",
        style: Any = None,
    ) -> Any:
        if isinstance(excinfo.value, MCtfTestError):
            return excinfo.value.format_output()

        return super().repr_failure(excinfo, style)


# Generic pytest collector that produces a single `MCtfTestItem`
# instance per `.mctf` file.
#
# The concrete item class is bound at construction time, therefore
# `conftest.py` files don't need to subclass this.
#
# See collect_mctf_file().
class _MCtfTestFile(pytest.File):
    def __init__(
        self,
        *,
        item_cls: Type[MCtfTestItem],
        log_label: str = "mctf",
        **kwargs: Any,
    ) -> None:
        super().__init__(**kwargs)  # pyright: ignore[reportUnknownMemberType]
        self._item_cls = item_cls
        self._log_label = log_label

    def collect(self) -> List[MCtfTestItem]:
        _logger.info(f"Adding {self._log_label} test from `{self.path}`")

        return [
            self._item_cls.from_parent(  # pyright: ignore[reportUnknownMemberType]
                name="test",
                parent=self,
                mctf_path=self.path,
            )
        ]


# Call this from your own pytest_collect_file() hook to handle
# an `.mctf` file producing a single `item_cls` test item.
#
# Returns `None` (skip) if `file_path` doesn't have the `.mctf` suffix.
#
# `log_label` is a short string used in the collection log message
# ("Adding {log_label} test from `…`").
def collect_mctf_file(
    file_path: pathlib.Path,
    parent: pytest.Collector,
    item_cls: Type[MCtfTestItem],
    log_label: str = "mctf",
) -> Optional[_MCtfTestFile]:
    if file_path.suffix != ".mctf":
        return

    return _MCtfTestFile.from_parent(  # pyright: ignore[reportUnknownMemberType]
        parent=parent,
        path=file_path,
        item_cls=item_cls,
        log_label=log_label,
    )
