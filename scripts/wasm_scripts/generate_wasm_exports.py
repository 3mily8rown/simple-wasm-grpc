#!/usr/bin/env python3
import os
import re
import sys
from subprocess import check_output, CalledProcessError

def demangle(sym):
    return check_output(["c++filt", sym]).decode().strip()

# Makes name uppercase, replace non-alphanumeric with underscore
def sanitize_symbol(name):
    
    return re.sub(r'\\W+', '_', name).upper()

"""
would need to not normalise to fully support overloading 
but then callers are required to know the original cpp types of the wasm functions not just the wasm type
types in mangling_utils need to correspond to find a function on lookup
"""
def normalize_type(raw: str) -> str:
    # Remove const, &, *, spaces
    raw = raw.strip()
    raw = re.sub(r'\bconst\b', '', raw)
    raw = raw.replace('&', '').replace('*', '').replace(' ', '')
    raw = raw.lower()  # Normalize case

    type_map = {
        # All i32 equivalents
        "char": "int",
        "unsignedchar": "int",
        "signedchar": "int",
        "short": "int",
        "unsignedshort": "int",
        "int": "int",
        "unsignedint": "int",
        "int32_t": "int",
        "uint32_t": "int",
        "int8_t": "int",
        "uint8_t": "int",
        "int16_t": "int",
        "uint16_t": "int",

        # i64
        "long": "int64",
        "unsignedlong": "int64",
        "int64_t": "int64",
        "uint64_t": "int64",

        # f32 and f64
        "float": "float",
        "double": "double"      
    }   

    # Lookup normalized type
    return type_map.get(raw, raw)

def main():
    if len(sys.argv) != 3:
        print("generate_wasm_exports.py <input.wasm> <output.h>", file=sys.stderr)
        sys.exit(1)

    wasm_in = sys.argv[1]
    hdr_out = sys.argv[2]

    base = os.path.splitext(os.path.basename(wasm_in))[0]
    sym = f"{sanitize_symbol(base)}_EXPORT_NAMES"

    try:
        dump = check_output(["wasm-objdump", "-x", wasm_in], stderr=sys.stderr).decode("utf-8")
    except CalledProcessError as e:
        print(f"Error running wasm-objdump: {e}", file=sys.stderr)
        sys.exit(2)

    pattern = re.compile(
        r'(?:Export\[\s*\d+\]|\s*-\s*)\s*func\[\d+\]\s+<[^>]+>\s+->\s+"([^"]+)"'
    )

    names = []
    for line in dump.splitlines():
        m = pattern.search(line)
        if m:
            names.append(m.group(1))

    with open(hdr_out, "w") as f:
        f.write("// AUTO-GENERATED — do not edit\n")
        f.write("#pragma once\n\n")
        f.write(f"static constexpr WasmExport {sym}[] = {{\n")
        for mangled in names:
            # pretty = demangle(mangled).split('(')[0]
            # f.write(f'  {{ "{pretty}", "{mangled}" }},\n')
            demangled = demangle(mangled)
            # matches lines like add(int, float)
            match = re.match(r"([^\s(]+)\(([^)]*)\)", demangled)
            if match:
                func_name = match.group(1)
                params = match.group(2).strip()
                raw_params = [p.strip() for p in params.split(',')] if params else []
                # param_list = [p.replace(" ","") for p in params.split(',')] if params else []
                # pretty = f'{func_name}:{",".join(param_list)}'
                param_list = [normalize_type(p) for p in raw_params]
                pretty = f'{func_name}:{",".join(param_list)}' if params else func_name
            else:
                pretty = demangled  # fallback/ 0 params 
            f.write(f'  {{ "{pretty}", "{mangled}" }},\n')

        f.write("    nullptr  // sentinel\n")
        f.write("};\n")

if __name__ == "__main__":
    main()
