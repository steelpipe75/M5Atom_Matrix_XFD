#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
input_file="$script_dir/index.html"
output_file="$script_dir/index_html.h"

python3 - "$input_file" "$output_file" <<'PY'
import sys
from pathlib import Path

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])

html_content = input_path.read_text(encoding="utf-8")
header_content = f'''#pragma once

const char index_html[] PROGMEM = R"rawliteral(
{html_content}
)rawliteral";
'''

output_path.write_text(header_content, encoding="utf-8-sig")
print("index_html.h has been generated successfully!")
PY
