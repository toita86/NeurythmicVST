#!/usr/bin/env python3
"""Generate the legacy-vs-current behaviour diagrams for docs/legacy_vs_current.md.

Run from the repository root:
    python3 docs/diagrams/src/behaviour_timeline.py
"""

from __future__ import annotations

import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle

OUT = Path(__file__).resolve().parent.parent

# Palette shared with the Graphviz diagrams.
LEGACY = "#B26A00"
LEGACY_ROOT = "#E65100"
CURRENT = "#1565C0"
GRID = "#C0A000"
GRID_FILL = "#FFF3C4"
INK = "#333333"
MUTED = "#666666"

plt.rcParams.update(
    {
        "font.family": "DejaVu Sans",
        "font.size": 9,
        "axes.edgecolor": "#BBBBBB",
        "axes.labelcolor": INK,
        "text.color": INK,
        "xtick.color": MUTED,
        "ytick.color": MUTED,
    }
)


def _bar_background(ax: plt.Axes) -> None:
    ax.axvspan(0.0, 2.0, color="#FFF8E1", alpha=0.55, zorder=0)
    ax.axvspan(2.0, 4.0, color="#F4F6F8", alpha=0.55, zorder=0)
    for t in (0.0, 2.0, 4.0):
        ax.axvline(t, color="#9E9E9E", lw=0.9, ls=(0, (4, 3)), zorder=1)
    ax.text(1.0, 1.255, "bar 1", ha="center", va="top", fontsize=9, color=MUTED)
    ax.text(3.0, 1.255, "bar 2", ha="center", va="top", fontsize=9, color=MUTED)


def _stems(ax: plt.Axes, times, height, color, label, lw=2.0, zorder=3):
    ax.vlines(times, 0.0, height, color=color, lw=lw, zorder=zorder)
    (handle,) = ax.plot(
        times,
        np.full(len(times), height),
        "o",
        color=color,
        ms=4.5,
        mec="white",
        mew=0.7,
        zorder=zorder + 1,
        label=label,
    )
    return handle


def _grid_band(ax: plt.Axes, start: float, width: float, label: str | None = None):
    ax.add_patch(
        Rectangle(
            (start, -0.27),
            width,
            0.09,
            facecolor=GRID_FILL,
            edgecolor=GRID,
            lw=0.8,
            zorder=2,
        )
    )
    if label:
        ax.text(
            start + width / 2,
            -0.37,
            label,
            ha="center",
            va="top",
            fontsize=8,
            color="#7A6200",
        )


def behaviour_timeline() -> None:
    fig, (ax1, ax2) = plt.subplots(
        2,
        1,
        figsize=(13.0, 7.8),
        sharex=True,
        gridspec_kw={"hspace": 0.55, "left": 0.06, "right": 0.98, "top": 0.90, "bottom": 0.18},
    )

    # ---- Legacy: root = beat ------------------------------------------------
    _bar_background(ax1)
    legacy_root = np.arange(0.0, 4.0 + 1e-9, 0.5)
    legacy_child = np.arange(0.0, 4.0 + 1e-9, 0.25)
    child_handle = _stems(
        ax1, legacy_child, 0.58, CURRENT, "2× child (4 Hz, one per 16th)", lw=1.6
    )
    root_handle = _stems(
        ax1, legacy_root, 1.0, LEGACY_ROOT, "root (2 Hz, one per beat)", lw=2.4
    )
    for start in np.arange(0.0, 4.0, 0.5):
        _grid_band(ax1, start, 0.5, "32 grid steps / beat" if start == 0.0 else None)

    ax1.text(
        0.0,
        1.345,
        "LEGACY  ·  root = BPM/60 = 2 Hz (beat)  ·  2× child = 4 Hz  ·  32 grid steps per beat",
        ha="left",
        va="top",
        fontsize=10,
        color=LEGACY,
        fontweight="bold",
    )
    ax1.text(
        2.55,
        0.72,
        "legacy root note is queued\n(0.125 s look-ahead)",
        ha="left",
        va="center",
        fontsize=8,
        color=LEGACY_ROOT,
    )
    ax1.set_ylim(-0.47, 1.39)
    ax1.set_yticks([])
    ax1.set_ylabel("legacy", rotation=0, labelpad=28, va="center", fontsize=11, color=LEGACY)

    # ---- Current: root = bar ------------------------------------------------
    _bar_background(ax2)
    current_root = np.array([0.0, 2.0, 4.0])
    current_child = np.arange(0.0, 4.0 + 1e-9, 1.0)
    _stems(
        ax2, current_child, 0.58, CURRENT, "2× child (1 Hz, downbeat + mid-bar)", lw=2.0
    )
    _stems(ax2, current_root, 1.0, LEGACY_ROOT, "root (0.5 Hz, one per bar)", lw=3.0)
    for start in (0.0, 2.0):
        _grid_band(ax2, start, 2.0, "32 grid steps / bar" if start == 0.0 else None)

    ax2.text(
        0.0,
        1.345,
        "CURRENT  ·  root = BPM/(60 × 4) = 0.5 Hz (bar)  ·  2× child = 1 Hz  ·  32 grid steps per bar",
        ha="left",
        va="top",
        fontsize=10,
        color=CURRENT,
        fontweight="bold",
    )
    ax2.text(
        2.0,
        1.10,
        "forced at bar",
        ha="center",
        va="bottom",
        fontsize=8,
        color=LEGACY_ROOT,
    )
    ax2.set_ylim(-0.47, 1.39)
    ax2.set_yticks([])
    ax2.set_xlim(0.0, 4.0)
    ax2.set_xticks(np.arange(0.0, 4.0 + 1e-9, 0.5))
    ax2.set_xlabel("time (seconds) — 120 BPM, 4/4, two bars", fontsize=9)
    ax2.set_ylabel("current", rotation=0, labelpad=28, va="center", fontsize=11, color=CURRENT)

    # One shared legend below both rows; avoids fighting the row titles.
    fig.legend(
        [child_handle, root_handle],
        ["2× child", "root"],
        loc="lower center",
        bbox_to_anchor=(0.5, 0.095),
        ncol=2,
        frameon=False,
        fontsize=9,
        handlelength=1.4,
    )

    fig.suptitle(
        "Legacy vs current rhythmic behaviour",
        fontsize=15,
        fontweight="bold",
        y=0.975,
    )
    fig.text(
        0.5,
        0.025,
        "Legacy phases are shown aligned at t = 0 for comparison; the legacy engine free-runs without host transport. "
        "The current 2× child fires on the downbeat and mid-bar, matching the root-as-bar model.",
        ha="center",
        va="bottom",
        fontsize=8,
        color=MUTED,
    )

    fig.savefig(OUT / "behaviour_timeline.svg", bbox_inches="tight")
    fig.savefig(OUT / "behaviour_timeline.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def event_trigger_timing() -> None:
    fig, ax = plt.subplots(figsize=(12.0, 3.6))
    t = np.linspace(-0.05, 1.55, 1600)
    y = np.sin(2.0 * math.pi * t)
    ax.plot(t, y, color="#444444", lw=1.8, zorder=2)
    ax.axhline(0.0, color="#BBBBBB", lw=0.8, zorder=0)

    # One rising zero crossing and the following first peak.
    zc = 0.0
    peak = 0.25
    ax.axvline(zc, color=LEGACY_ROOT, lw=1.4, ls=(0, (4, 3)), zorder=1)
    ax.axvline(peak, color=CURRENT, lw=1.4, ls=(0, (4, 3)), zorder=1)
    ax.plot([zc], [0.0], "o", color=LEGACY_ROOT, ms=7, zorder=4)
    ax.plot([peak], [1.0], "o", color=CURRENT, ms=7, zorder=4)

    ax.annotate(
        "Legacy: positive zero crossing\nfireOnPeak = false  →  timing reference",
        xy=(zc, 0.0),
        xytext=(zc + 0.08, -0.72),
        ha="left",
        va="top",
        fontsize=9,
        color=LEGACY_ROOT,
        arrowprops=dict(arrowstyle="->", color=LEGACY_ROOT, lw=1.0),
    )
    ax.annotate(
        "Current default: first peak\nFire on Peak = on  →  amplitude velocity",
        xy=(peak, 1.0),
        xytext=(peak + 0.10, 0.72),
        ha="left",
        va="bottom",
        fontsize=9,
        color=CURRENT,
        arrowprops=dict(arrowstyle="->", color=CURRENT, lw=1.0),
    )
    ax.annotate(
        "",
        xy=(peak, 1.18),
        xytext=(zc, 1.18),
        arrowprops=dict(arrowstyle="<->", color="#6A1B9A", lw=1.3),
    )
    ax.text(
        (zc + peak) / 2,
        1.23,
        "up to ¼ cycle later\n(free / unquantised path)",
        ha="center",
        va="bottom",
        fontsize=8.5,
        color="#6A1B9A",
    )

    ax.set_xlim(-0.05, 1.55)
    ax.set_ylim(-0.95, 1.45)
    ax.set_yticks([-1, 0, 1])
    ax.set_xlabel("one oscillator cycle (normalised)", fontsize=9)
    ax.set_title(
        "Event trigger point: legacy zero-crossing vs current first peak",
        fontsize=14,
        fontweight="bold",
        pad=12,
    )
    ax.text(
        1.54,
        -0.92,
        "Host-synced current mode forces the root at the bar; this difference is mainly audible in the free path. The Fire on Peak switch selects either mode.",
        ha="right",
        va="bottom",
        fontsize=8,
        color=MUTED,
    )
    ax.spines[["top", "right"]].set_visible(False)

    fig.tight_layout()
    fig.savefig(OUT / "event_trigger_timing.svg", bbox_inches="tight")
    fig.savefig(OUT / "event_trigger_timing.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    behaviour_timeline()
    event_trigger_timing()
    print(f"wrote diagrams to {OUT}")
