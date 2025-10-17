#!/usr/bin/env python3
"""
Generate doctest sources from legacy TUT tests.

This generator progressively migrates the llcommon test suite by converting
TUT constructs into doctest-friendly code whenever a safe mechanical mapping
exists. Anything that falls outside the supported patterns remains annotated
with an explicit TODO marker so that manual follow-up stays straightforward.

Running the script multiple times with unchanged inputs produces identical
outputs (idempotent generation).
"""

from __future__ import annotations

import argparse
import ast
from dataclasses import dataclass
from datetime import datetime, timezone
import pathlib
import re
import textwrap
from typing import Iterable, List, Optional, Sequence, Tuple


TEST_RE = re.compile(
    r"template\s*<>\s*template\s*<>\s*void\s+"
    r"(?P<object>[A-Za-z_]\w*)::test<\s*(?P<index>\d+)\s*>\s*\(\s*\)\s*{",
    re.MULTILINE,
)

INCLUDE_RE = re.compile(r"^\s*#\s*include\s+[<\"].+[>\"]\s*$", re.MULTILINE)
CALL_PATTERN = re.compile(r"\b(?P<name>set_test_name|ensure(?:_[A-Za-z0-9]+)*|skip)\s*\(")
GROUP_DIRECT_RE = re.compile(
    r"\btest_group<[^>]+>\s+[A-Za-z_]\w*\s*\(\s*\"(?P<name>[^\"]+)\"",
    re.MULTILINE,
)
GROUP_TYPEDEF_RE = re.compile(
    r"\b[A-Za-z_]\w*group\s+[A-Za-z_]\w*\s*\(\s*\"(?P<name>[^\"]+)\"",
    re.MULTILINE,
)
NAMESPACE_TUT_RE = re.compile(r"\bnamespace\s+tut\b")

WINDOWS_EXCLUDES = (
    "sys/wait.h",
    "unistd.h",
    "llallocator.h",
    "llallocator_heap_profile.h",
    "llmemtype.h",
    "lllazy.h",
    "netinet/in.h",
    "StringVec.h",
)

COMPLEX_SOURCES = {
    "threadsafeschedule_test.cpp",
    "workqueue_test.cpp",
}

MANUAL_SKIP_TAG = "DOCTEST_SKIP_AUTOGEN"


@dataclass
class TestCase:
    object_name: str
    index: str
    body: str
    snippet: str


@dataclass
class ConvertedCase:
    case_name: str
    body_lines: List[str]
    issues: List[str]
    snippet: str


@dataclass
class FileSummary:
    source: pathlib.Path
    destination: pathlib.Path
    suite_name: str
    cases: Sequence[ConvertedCase]
    wrote_file: bool


def extract_block(source: str, brace_index: int) -> Tuple[str, int]:
    """Extract the block (without surrounding braces) that starts at brace_index."""
    depth = 0
    end_index = brace_index
    for idx in range(brace_index, len(source)):
        char = source[idx]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                end_index = idx
                break
    body = source[brace_index + 1 : end_index]
    return body, end_index + 1


def collect_includes(source: str) -> Tuple[List[str], List[str]]:
    includes: List[str] = []
    blocked: List[str] = []
    seen: set[str] = set()
    for match in INCLUDE_RE.finditer(source):
        line = match.group(0).strip()
        if "lltut.h" in line or "tut/" in line:
            continue
        for keyword in WINDOWS_EXCLUDES:
            if keyword in line:
                blocked.append(keyword)
                line = "// " + line + "  // not available on Windows"
                break
        if line not in seen:
            includes.append(line)
            seen.add(line)
    return includes, blocked


def find_matching_paren(text: str, open_index: int) -> int:
    depth = 1
    i = open_index + 1
    in_string = False
    escape = False
    string_delim = ""
    while i < len(text):
        char = text[i]
        if in_string:
            if escape:
                escape = False
            elif char == "\\":
                escape = True
            elif char == string_delim:
                in_string = False
            i += 1
            continue
        if char == '"' or char == "'":
            in_string = True
            string_delim = char
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("Unbalanced parentheses while parsing call")


def split_arguments(arg_text: str) -> List[str]:
    args: List[str] = []
    current: List[str] = []
    depth = 0
    in_string = False
    escape = False
    string_delim = ""
    for char in arg_text:
        if in_string:
            current.append(char)
            if escape:
                escape = False
            elif char == "\\":
                escape = True
            elif char == string_delim:
                in_string = False
            continue
        if char == '"' or char == "'":
            in_string = True
            string_delim = char
            current.append(char)
        elif char in "([{":
            depth += 1
            current.append(char)
        elif char in ")]}":
            depth -= 1
            current.append(char)
        elif char == "," and depth == 0:
            args.append("".join(current).strip())
            current = []
        else:
            current.append(char)
    if current:
        args.append("".join(current).strip())
    return args


def find_comment_spans(text: str) -> List[Tuple[int, int]]:
    spans: List[Tuple[int, int]] = []
    length = len(text)
    i = 0
    in_string = False
    escape = False
    string_delim = ""
    while i < length:
        char = text[i]
        if in_string:
            if escape:
                escape = False
            elif char == "\\":
                escape = True
            elif char == string_delim:
                in_string = False
            i += 1
            continue
        if text.startswith("//", i):
            start = i
            end = text.find("\n", i)
            if end == -1:
                end = length
            spans.append((start, end))
            i = end
            continue
        if text.startswith("/*", i):
            start = i
            end = text.find("*/", i + 2)
            if end == -1:
                end = length
            else:
                end += 2
            spans.append((start, end))
            i = end
            continue
        if char == '"' or char == "'":
            in_string = True
            string_delim = char
            escape = False
        i += 1
    return spans


def is_within_spans(index: int, spans: Sequence[Tuple[int, int]]) -> bool:
    for start, end in spans:
        if start <= index < end:
            return True
        if index < start:
            return False
    return False


def extract_string_literal(text: str) -> Optional[str]:
    stripped = text.strip()
    match = re.match(r'(?:[LuU8R]|u8|u|U|L)?(".*")', stripped, re.DOTALL)
    if not match:
        return None
    literal = match.group(1)
    try:
        return ast.literal_eval(literal)
    except (SyntaxError, ValueError):
        return None


def is_wide_string_literal(text: str) -> bool:
    stripped = text.strip()
    return stripped.startswith("L\"") or stripped.startswith("LR\"") or stripped.startswith("L'")


def looks_like_wide_string(expr: str) -> bool:
    stripped = expr.strip()
    if is_wide_string_literal(stripped):
        return True
    markers = ("std::wstring", "LLWString", "LLWSTRING", "llwchar", "wchar_t")
    return any(marker in stripped for marker in markers)


def sanitize_case_label(label: str) -> str:
    return label.replace("\\", "\\\\").replace('"', '\\"')


def escape_string_literal(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def handle_set_test_name(args_text: str, current_name: Optional[str], original: str) -> Tuple[str, Optional[str]]:
    args = split_arguments(args_text)
    if not args:
        return original, current_name
    new_name = extract_string_literal(args[0])
    if new_name and current_name is None:
        current_name = new_name
    joined_args = ", ".join(args)
    return f"TUT_SET_TEST_NAME({joined_args})", current_name


def handle_ensure_call(name: str, args_text: str, original: str) -> Tuple[str, Optional[str]]:
    args = split_arguments(args_text)
    if name == "ensure":
        if len(args) == 1:
            return f"TUT_ENSURE({args[0]})", None
        if len(args) == 2:
            return f"TUT_CHECK_MSG({args[1]}, {args[0]})", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_equals":
        if len(args) == 2:
            left, right = args
            if looks_like_wide_string(left) or looks_like_wide_string(right):
                block_lines = [
                    "do {",
                    "#ifdef _WIN32",
                    f"    TUT_ENSURE_WSTR_EQ({left}, {right});",
                    "#else",
                    f"    TUT_ENSURE_EQ({left}, {right});",
                    "#endif",
                    "} while (false)",
                ]
                return "\n".join(block_lines), None
            return f"TUT_ENSURE_EQ({left}, {right})", None
        if len(args) == 3:
            return f"TUT_ENSURE_EQ({', '.join(args)})", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_not":
        if 1 <= len(args) <= 2:
            return f"TUT_ENSURE_NOT({', '.join(args)})", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_throws":
        if len(args) == 1:
            return f"TUT_ENSURE_THROWS({args[0]})", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_not_equals":
        if len(args) == 3:
            message, left, right = args
            return f"TUT_CHECK_MSG(({left}) != ({right}), {message})", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_approximately_equals":
        if len(args) == 3:
            actual, expected, epsilon = args
            return f"TUT_ENSURE_APPROX({actual}, {expected}, {epsilon})", None
        if len(args) == 4:
            message, actual, expected, epsilon = args
            approx_expr = f"({actual}) == doctest::Approx({expected}).epsilon({epsilon})"
            return f"TUT_CHECK_MSG({approx_expr}, {message})", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_approximately_equals_range":
        if len(args) == 3:
            actual, expected, tolerance = args
            return f"TUT_ENSURE_APPROX_NEAR({actual}, {expected}, {tolerance})", None
        if len(args) == 4:
            if extract_string_literal(args[0]) is not None:
                message, actual, expected, tolerance = args
                condition = f"std::fabs(double({actual}) - double({expected})) <= ({tolerance})"
                return f"TUT_CHECK_MSG({condition}, {message})", None
            ptr_a, ptr_b, count, tolerance = args
            return f"TUT_ENSURE_APPROX_RANGE({ptr_a}, {ptr_b}, {count}, {tolerance})", None
        if len(args) == 5:
            message, ptr_a, ptr_b, count, tolerance = args
            joined = ", ".join([ptr_a, ptr_b, count, tolerance])
            return f"do {{ INFO({message}); TUT_ENSURE_APPROX_RANGE({joined}); }} while (false)", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_in_range":
        if len(args) == 3:
            value, low, high = args
            return f"TUT_ENSURE_IN_RANGE({value}, {low}, {high})", None
        if len(args) == 4:
            message, value, low, high = args
            joined = ", ".join([value, low, high])
            return f"do {{ INFO({message}); TUT_ENSURE_IN_RANGE({joined}); }} while (false)", None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_memory_matches":
        if len(args) == 3:
            return f"TUT_ENSURE_MEMORY_MATCHES({args[0]}, {args[1]}, {args[2]})", None
        if len(args) == 4:
            actual, actual_len, expected, expected_len = args
            block = [
                "do {",
                f"    TUT_ENSURE_EQ({actual_len}, {expected_len});",
                f"    TUT_ENSURE_MEMORY_MATCHES({actual}, {expected}, {actual_len});",
                "} while (false)",
            ]
            return "\n".join(block), None
        if len(args) == 5:
            message, actual, actual_len, expected, expected_len = args
            block = [
                "do {",
                f"    INFO({message});",
                f"    TUT_ENSURE_EQ({actual_len}, {expected_len});",
                f"    TUT_ENSURE_MEMORY_MATCHES({actual}, {expected}, {actual_len});",
                "} while (false)",
            ]
            return "\n".join(block), None
        return original, f"{name}:{len(args)}-args"
    if name == "ensure_contains":
        if len(args) == 2:
            return f"TUT_ENSURE_CONTAINS({args[0]}, {args[1]})", None
        if len(args) == 3:
            return f"TUT_ENSURE_CONTAINS({args[0]}, {args[1]}, {args[2]})", None
        return original, f"{name}:{len(args)}-args"
    return original, f"unsupported:{name}"


def transform_calls(body: str) -> Tuple[str, Optional[str], List[str]]:
    pos = 0
    fragments: List[str] = []
    friendly_name: Optional[str] = None
    issues: List[str] = []
    comment_spans = find_comment_spans(body)
    span_index = 0
    while True:
        match = CALL_PATTERN.search(body, pos)
        if not match:
            fragments.append(body[pos:])
            break
        start = match.start()
        while span_index < len(comment_spans) and start >= comment_spans[span_index][1]:
            span_index += 1
        if span_index < len(comment_spans) and comment_spans[span_index][0] <= start:
            fragments.append(body[pos:match.end()])
            pos = match.end()
            continue
        fragments.append(body[pos:start])
        name = match.group("name")
        open_paren = match.end() - 1
        close_paren = find_matching_paren(body, open_paren)
        args_text = body[open_paren + 1 : close_paren]
        original = body[match.start() : close_paren + 1]
        if name == "set_test_name":
            replacement, friendly_name = handle_set_test_name(args_text, friendly_name, original)
        elif name == "skip":
            replacement = f"TUT_SKIP({args_text.strip()})"
        else:
            replacement, issue = handle_ensure_call(name, args_text, original)
            if issue:
                issues.append(issue)
        fragments.append(replacement)
        pos = close_paren + 1
    rewritten = "".join(fragments)
    leftover_pattern = re.compile(r"\bensure(?:_[A-Za-z0-9]+)*\s*\(")
    leftover_spans = find_comment_spans(rewritten)
    search_pos = 0
    while True:
        leftover = leftover_pattern.search(rewritten, search_pos)
        if not leftover:
            break
        if is_within_spans(leftover.start(), leftover_spans):
            search_pos = leftover.end()
            continue
        issues.append("unconverted ensure call")
        break
    return rewritten, friendly_name, issues


def transform_body(body: str) -> Tuple[List[str], Optional[str], List[str]]:
    dedented = textwrap.dedent(body).strip("\n")
    rewritten, friendly, issues = transform_calls(dedented)
    lines = rewritten.splitlines()
    return [line.rstrip() for line in lines], friendly, issues


def strip_includes(section: str) -> str:
    lines = []
    for line in section.splitlines():
        if line.strip().startswith("#include"):
            continue
        lines.append(line.rstrip())
    return "\n".join(lines)


def clean_namespace_body(body: str) -> str:
    result: List[str] = []
    pos = 0
    while True:
        match = TEST_RE.search(body, pos)
        if not match:
            result.append(body[pos:])
            break
        result.append(body[pos:match.start()])
        brace_index = match.end() - 1
        _, end_index = extract_block(body, brace_index)
        pos = end_index
    cleaned = "".join(result)
    filtered_lines: List[str] = []
    for line in cleaned.splitlines():
        stripped = line.strip()
        if not stripped:
            filtered_lines.append("")
            continue
        if stripped.startswith("typedef") and "test_group" in stripped:
            continue
        if stripped.startswith("typedef") and "::object" in stripped:
            continue
        if re.match(r"[A-Za-z_]\w*group\s+[A-Za-z_]\w*\s*\(", stripped):
            continue
        if stripped.startswith("tut::"):
            continue
        filtered_lines.append(line.rstrip())
    while filtered_lines and filtered_lines[0] == "":
        filtered_lines.pop(0)
    while filtered_lines and filtered_lines[-1] == "":
        filtered_lines.pop()
    return "\n".join(filtered_lines)


def discover_tests(source: str) -> List[TestCase]:
    tests: List[TestCase] = []
    for match in TEST_RE.finditer(source):
        object_name = match.group("object")
        index = match.group("index")
        body, end_index = extract_block(source, match.end() - 1)
        snippet = source[match.start() : end_index]
        tests.append(TestCase(object_name=object_name, index=index, body=body, snippet=snippet))
    return tests


def determine_suite_name(source: str, default: str) -> str:
    for pattern in (GROUP_DIRECT_RE, GROUP_TYPEDEF_RE):
        match = pattern.search(source)
        if match:
            return match.group("name")
    return default


def convert_test_case(
    src_path: pathlib.Path,
    test: TestCase,
    has_tut_namespace: bool,
    blocked_includes: Sequence[str],
) -> ConvertedCase:
    lines, friendly_name, issues = transform_body(test.body)
    if not lines:
        issues.append("empty body")
    case_label = friendly_name or f"{test.object_name}_test_{test.index}"
    case_name = f"{src_path.stem}::{sanitize_case_label(case_label)}"
    missing_headers = sorted(set(blocked_includes))
    if missing_headers:
        header_summary = ", ".join(missing_headers)
        fallback_lines = [f'DOCTEST_FAIL("TODO: missing includes: {header_summary}");', "// Original snippet:"]
        original_lines = textwrap.dedent(test.snippet).strip("\n").splitlines()
        if not original_lines:
            original_lines = ["<empty snippet>"]
        for original_line in original_lines:
            fallback_lines.append(f"// {original_line}")
        issues = list(issues) + [f"missing include: {name}" for name in missing_headers]
        return ConvertedCase(case_name=case_name, body_lines=fallback_lines, issues=issues, snippet=test.snippet)
    if issues:
        summary = "; ".join(sorted(set(issues)))
        body_lines = [f'DOCTEST_FAIL("TODO: {summary}");', "// Original snippet:"]
        original_lines = textwrap.dedent(test.snippet).strip("\n").splitlines()
        if not original_lines:
            original_lines = ["<empty snippet>"]
        for original_line in original_lines:
            body_lines.append(f"// {original_line}")
        return ConvertedCase(case_name=case_name, body_lines=body_lines, issues=issues, snippet=test.snippet)
    body_lines = list(lines)
    if has_tut_namespace:
        body_lines.insert(0, "using namespace tut;")
    return ConvertedCase(case_name=case_name, body_lines=body_lines, issues=[], snippet=test.snippet)


def build_namespace_block(clean_body: str) -> List[str]:
    if not clean_body.strip():
        return []
    lines = [
        "namespace tut",
        "{",
        "    using tut_compat::ensure;",
        "    using tut_compat::ensure_equals;",
        "    using tut_compat::ensure_not;",
        "    using tut_compat::ensure_throws;",
        "",
    ]
    for line in clean_body.splitlines():
        if line:
            lines.append(f"    {line}")
        else:
            lines.append("")
    lines.append("} // namespace tut")
    return lines


def render_case(case: ConvertedCase) -> List[str]:
    lines = [f'    TUT_CASE("{case.case_name}")', "    {"]
    for body_line in case.body_lines:
        if body_line:
            lines.append(f"        {body_line}")
        else:
            lines.append("")
    lines.append("    }")
    return lines


def render_suite(suite_name: str, cases: Sequence[ConvertedCase]) -> List[str]:
    safe_suite = escape_string_literal(suite_name)
    lines: List[str] = [f'TUT_SUITE("{safe_suite}")', "{"]
    if not cases:
        lines.append(f'    TUT_CASE("{safe_suite}::no_tests_detected")')
        lines.append("    {")
        lines.append('        DOCTEST_FAIL("TODO: no TUT tests discovered in source file");')
        lines.append("    }")
    else:
        for index, case in enumerate(cases):
            lines.extend(render_case(case))
            if index != len(cases) - 1:
                lines.append("")
    lines.append("}")
    return lines


def replace_namespace_blocks(section: str) -> Tuple[str, bool]:
    processed = section
    replaced_any = False
    search_start = 0
    while True:
        match = NAMESPACE_TUT_RE.search(processed, search_start)
        if not match:
            break
        brace_index = processed.find("{", match.end())
        if brace_index == -1:
            break
        body, end_index = extract_block(processed, brace_index)
        cleaned_body = clean_namespace_body(body)
        namespace_lines = build_namespace_block(cleaned_body)
        block_text = "\n".join(namespace_lines)
        processed = processed[:match.start()] + block_text + processed[end_index:]
        search_start = match.start() + len(block_text)
        replaced_any = True
    return processed, replaced_any


def load_existing_timestamp(dst_path: pathlib.Path) -> Optional[str]:
    if not dst_path.exists():
        return None
    try:
        for line in dst_path.read_text(encoding="utf-8-sig").splitlines():
            if line.startswith("// Auto-generated from ") and " at " in line:
                return line.split(" at ", 1)[1].strip()
    except OSError:
        return None
    return None


def should_skip_autogen(dst_path: pathlib.Path) -> bool:
    if not dst_path.exists():
        return False
    try:
        content = dst_path.read_text(encoding="utf-8-sig")
    except OSError:
        return False
    return MANUAL_SKIP_TAG in content


def write_if_changed(path: pathlib.Path, content: str) -> bool:
    encoded = content.encode("utf-8")
    if path.exists():
        try:
            existing = path.read_bytes()
            if existing == encoded:
                return False
        except OSError:
            pass
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(encoded)
    return True


def convert_file(src_path: pathlib.Path, dst_path: pathlib.Path) -> FileSummary:
    source = src_path.read_text(encoding="utf-8")
    includes, blocked_includes = collect_includes(source)
    if src_path.name in COMPLEX_SOURCES and "complex-fixture" not in blocked_includes:
        blocked_includes = list(blocked_includes) + ["complex-fixture"]
    tests = discover_tests(source)
    suite_name = determine_suite_name(source, src_path.stem)
    body_section = strip_includes(source)
    body_section, has_namespace = replace_namespace_blocks(body_section)
    cases = [convert_test_case(src_path, test, has_namespace, blocked_includes) for test in tests]
    timestamp = load_existing_timestamp(dst_path)
    if not timestamp:
        timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds")
    skip_autogen = should_skip_autogen(dst_path)
    header_lines = [
        "// ---------------------------------------------------------------------------",
        f"// Auto-generated from {src_path.name} at {timestamp}",
        "// Generated by gen_tut_to_doctest.py",
        "// ---------------------------------------------------------------------------",
    ]
    include_lines = [
        '#include "doctest.h"',
        '#include "indra/test/ll_doctest_helpers.h"',
        '#include "indra/test/tut_compat_doctest.h"',
    ]
    include_lines.extend(includes)
    body_clean = body_section.strip("\n")
    body_lines = body_clean.splitlines() if body_clean else []
    if blocked_includes and body_lines:
        summary = ", ".join(sorted(set(blocked_includes)))
        commented: List[str] = [f"// Original helper code skipped (missing includes: {summary})"]
        for line in body_lines:
            if line:
                commented.append(f"// {line}")
            else:
                commented.append("//")
        body_lines = commented
    suite_lines = render_suite(suite_name, cases)
    output_lines: List[str] = []
    output_lines.extend(header_lines)
    output_lines.extend(include_lines)
    output_lines.append("")
    if body_lines:
        output_lines.extend(body_lines)
    output_lines.append("")
    output_lines.extend(suite_lines)
    output_lines.append("")
    content = "\n".join(output_lines)
    wrote_file = False
    if not skip_autogen:
        wrote_file = write_if_changed(dst_path, content)
    return FileSummary(
        source=src_path,
        destination=dst_path,
        suite_name=suite_name,
        cases=cases,
        wrote_file=wrote_file,
    )


def generate_index(dst_dir: pathlib.Path, mappings: Iterable[Tuple[pathlib.Path, pathlib.Path]]) -> None:
    lines = [f"{src.name} -> {dst.name}" for src, dst in mappings]
    index_path = dst_dir / "generated.index"
    write_if_changed(index_path, "\n".join(lines) + "\n")


def summarize_file(summary: FileSummary) -> str:
    total = len(summary.cases)
    todo = sum(1 for case in summary.cases if case.issues)
    converted = total - todo
    status = "updated" if summary.wrote_file else "unchanged"
    return f"[{summary.source.name}] suite={summary.suite_name} cases={total} converted={converted} todo={todo} ({status})"


def convert_directory(src_dir: pathlib.Path, dst_dir: pathlib.Path) -> List[FileSummary]:
    summaries: List[FileSummary] = []
    mappings: List[Tuple[pathlib.Path, pathlib.Path]] = []
    for cpp_path in sorted(src_dir.glob("*.cpp")):
        dest_filename = cpp_path.stem + "_doctest.cpp"
        dest_path = dst_dir / dest_filename
        summary = convert_file(cpp_path, dest_path)
        summaries.append(summary)
        mappings.append((cpp_path.relative_to(src_dir), dest_path.relative_to(dst_dir)))
    generate_index(dst_dir, mappings)
    return summaries


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate doctest sources from TUT inputs.")
    parser.add_argument("--src", required=True, help="Directory containing original TUT test sources.")
    parser.add_argument("--dst", required=True, help="Directory where doctest files will be written.")
    return parser.parse_args()


def main() -> None:
    args = parse_arguments()
    src_dir = pathlib.Path(args.src).resolve()
    dst_dir = pathlib.Path(args.dst).resolve()
    if not src_dir.exists():
        raise SystemExit(f"Source directory {src_dir} does not exist")
    dst_dir.mkdir(parents=True, exist_ok=True)
    summaries = convert_directory(src_dir, dst_dir)
    for summary in summaries:
        print(summarize_file(summary))


if __name__ == "__main__":
    main()
