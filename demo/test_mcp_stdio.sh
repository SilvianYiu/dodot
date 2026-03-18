#!/bin/bash
# MCP stdio integration test
# Exercises the MCP server in stdio mode, verifying JSON-RPC responses.
#
# Usage: ./test_mcp_stdio.sh [/path/to/godot]
#
# Each test launches a separate godot process with a single request,
# then kills it after reading the response.

set -uo pipefail

GODOT="${1:-./bin/godot.linuxbsd.editor.x86_64}"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0
FAIL=0
TOTAL=0

pass() {
    PASS=$((PASS + 1))
    TOTAL=$((TOTAL + 1))
    echo "  PASS: $1"
}

fail() {
    FAIL=$((FAIL + 1))
    TOTAL=$((TOTAL + 1))
    echo "  FAIL: $1 — $2"
}

# Send a single MCP request and check the response.
# Launches godot, writes request, reads response, kills godot.
test_request() {
    local label="$1"
    local json="$2"
    local expect_error="${3:-no}"
    local content_length=${#json}
    local outfile="/tmp/mcp_test_$$.out"

    # Start godot in background, capture stdout
    "$GODOT" --headless --path "$PROJECT_DIR" --mcp-stdio \
        < <(
            # Send the request with Content-Length header
            printf "Content-Length: %d\r\n\r\n%s" "$content_length" "$json"
            # Keep stdin open for 5 seconds to allow processing
            sleep 5
        ) > "$outfile" 2>/dev/null &
    local pid=$!

    # Wait for response (up to 15s)
    local waited=0
    while [ $waited -lt 15 ]; do
        sleep 1
        waited=$((waited + 1))
        # Check if output file has content with a JSON response
        if [ -s "$outfile" ] 2>/dev/null; then
            # Check if we got a complete JSON response (contains "jsonrpc")
            if read -r line < "$outfile" 2>/dev/null; then
                if [[ "$line" == *"jsonrpc"* ]] || [[ "$(wc -c < "$outfile" 2>/dev/null)" -gt 20 ]]; then
                    break
                fi
            fi
        fi
    done

    # Kill godot
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null

    if [ ! -s "$outfile" ] 2>/dev/null; then
        fail "$label" "no output"
        rm -f "$outfile" 2>/dev/null
        return
    fi

    # Read the output — should be Content-Length header + JSON body
    local body=""
    while IFS= read -r line; do
        # Strip carriage return
        line="${line%$'\r'}"
        # Look for JSON (starts with {)
        if [[ "$line" == "{"* ]]; then
            body="$line"
            break
        fi
    done < "$outfile"

    rm -f "$outfile" 2>/dev/null

    if [ -z "$body" ]; then
        fail "$label" "no JSON in response"
        return
    fi

    if [[ "$body" != *'"jsonrpc"'* ]]; then
        fail "$label" "missing jsonrpc field"
        return
    fi

    if [ "$expect_error" = "yes" ]; then
        if [[ "$body" == *'"error"'* ]]; then
            pass "$label (expected error)"
        else
            fail "$label" "expected error but got success"
        fi
    else
        if [[ "$body" == *'"error"'* ]]; then
            fail "$label" "unexpected error in response"
        else
            pass "$label"
        fi
    fi
}

echo "=== Dodot MCP stdio Integration Test ==="
echo "Binary: $GODOT"
echo "Project: $PROJECT_DIR"
echo ""

# Protocol tests
test_request "initialize" \
    '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'

test_request "ping" \
    '{"jsonrpc":"2.0","id":1,"method":"ping","params":{}}'

test_request "tools/list" \
    '{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}'

test_request "resources/list" \
    '{"jsonrpc":"2.0","id":1,"method":"resources/list","params":{}}'

test_request "prompts/list" \
    '{"jsonrpc":"2.0","id":1,"method":"prompts/list","params":{}}'

# Tool calls
test_request "tools/call: get_engine_info" \
    '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_engine_info","arguments":{}}}'

test_request "tools/call: validate_script" \
    '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"validate_script","arguments":{"path":"res://scripts/player.gd"}}}'

test_request "tools/call: discover_tests" \
    '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"discover_tests","arguments":{}}}'

test_request "tools/call: run_tests" \
    '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"run_tests","arguments":{}}}'

# Error cases
test_request "unknown method (expect error)" \
    '{"jsonrpc":"2.0","id":1,"method":"nonexistent/method","params":{}}' \
    "yes"

test_request "unknown tool (expect error)" \
    '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"nonexistent_tool","arguments":{}}}' \
    "yes"

echo ""
echo "=== Results ==="
echo "  Total:  $TOTAL"
echo "  Passed: $PASS"
echo "  Failed: $FAIL"
echo ""
if [ $FAIL -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
