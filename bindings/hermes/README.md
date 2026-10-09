# Hermes integration

- `SKILL.md`: the `misty-ascii` skill. Installed at `~/.hermes/skills/misty-ascii/SKILL.md`.
- `insights.patch`: makes `/insights` draw with the engine (models table, tool bar chart,
  sessions-per-day line chart, day bars, 24-hour sparkline). Applied locally on E15 on
  2026-10-09 to `hermes-agent/agent/insights.py` (backup: `insights.py.bak-mistyascii-20261009`),
  together with a copy of `bindings/python/misty_ascii.py` at `hermes-agent/agent/misty_ascii.py`.
  Without the binary the report renders exactly as before.
- Binary: `~/.hermes/bin/misty.exe` (portable Cosmopolitan build from `scripts/build-cosmo.sh`).

Re-apply after a Hermes update replaces `agent/insights.py`:

```sh
cd hermes-agent && cp ../misty-ascii/bindings/python/misty_ascii.py agent/ && git apply ../misty-ascii/bindings/hermes/insights.patch
```
