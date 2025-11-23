#!/usr/bin/env bash
# Helper script to build lab_5.pdf from a temp directory to avoid '~' in the path.
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd -P)"
tmpdir="$(mktemp -d 2>/dev/null || mktemp -d -t lab5build)"
cleanup() { rm -rf "$tmpdir"; }
trap cleanup EXIT

cp "$script_dir/lab_5.tex" "$tmpdir/"
cp "$script_dir"/image1.png "$script_dir"/image2.png "$tmpdir/" 2>/dev/null || true
cp "$script_dir"/lab_5.m "$tmpdir/" 2>/dev/null || true

cd "$tmpdir"
latexmk -pdf -interaction=nonstopmode lab_5.tex

cp "$tmpdir/lab_5.pdf" "$script_dir/"
echo "PDF built at $script_dir/lab_5.pdf"
