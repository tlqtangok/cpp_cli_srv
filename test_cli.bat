@echo off
set CLI=c:\Users\tlqta\WorkBuddy\Claw\cpp_cli_srv\build\mytool-cli.exe

echo === Test 1: echo (human) ===
%CLI% --cmd echo --args "{\"text\":\"hello world\"}" --human

echo === Test 2: add (human) ===
%CLI% --cmd add --args "{\"a\":3,\"b\":4}" --human

echo === Test 3: upper (JSON output) ===
%CLI% --cmd upper --args "{\"text\":\"hello\"}"

echo === Test 4: unknown command (error) ===
%CLI% --cmd bad_cmd --args "{}"

echo === Test 5: schema ===
%CLI% --schema
