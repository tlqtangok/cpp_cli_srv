#!bash
set -x

bash build.sh
tar cvzf pkg.tgz web build/cpp_cli build/cpp_srv  data/test.json 
tfr t pkg.tgz

if [ 1 == 1 ]; then
curl -k -X POST https://jesson.tech:10221/post/run   -H 'Content-Type: application/json' -d '{"cmd":"patch_global_json","args":{"cpp_cli_srv_config":{"restart_cpp_srv":true}}}'
curl -k -X POST https://jesson.tech:10251/c/post/run -H 'Content-Type: application/json' -d '{"cmd":"patch_global_json","args":{"cpp_cli_srv_config":{"restart_cpp_srv":true}}}'
fi

