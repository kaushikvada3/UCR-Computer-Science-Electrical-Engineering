#!/usr/bin/env bash
# Build helper to avoid '~' in the path for latexmk.
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd -P)"
tmpdir="$(mktemp -d 2>/dev/null || mktemp -d -t hw8build)"
cleanup() { rm -rf "$tmpdir"; }
trap cleanup EXIT

cp "$script_dir/homework_8.tex" "$tmpdir/"
cd "$tmpdir"
latexmk -pdf -interaction=nonstopmode homework_8.tex
cp "$tmpdir/homework_8.pdf" "$script_dir/"
echo "PDF built at $script_dir/homework_8.pdf"
