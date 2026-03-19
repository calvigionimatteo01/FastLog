# FastLog

FastLog is a lightweight command-line tool written in C to quickly analyze log files.

Built with a minimal and pragmatic approach: no external dependencies, fast parsing, and clear output.

---

## Features

- Count IP occurrences (first token per line)
- Filter lines by substring (e.g. ERROR)
- Show top N results
- Output in plain text or JSON

---

## Example Usage

```bash
./fastlog file.log
./fastlog file.log --filter ERROR
./fastlog file.log --top 5
./fastlog file.log --json

## Ready-to-use Binary

Don't want to compile?

Download a ready-to-use version here:

👉 ziocalvi271.gumroad.com/l/FastLog

Includes:
- precompiled binary
- sample log file
- quick usage guide
