# 1. convert wat to wasm
wasm-tools parse sqlite.wat -o sqlite2.wasm

# 2. create a stack stable
wacret create --v2 sqlite2.wasm

# 3. execute and checkpoint on wasm3
NOP_CKPT=1 mwasm3 --stack-size 1000000 sqlite2.wasm
mv *.img wasm3-image
wacret view wasm3-image/call_stack.img > wasm3-image/call_stack.json

# 4. execute and checkpoint on wasmedge
NOP_CKPT=1 mwasmedge sqlite2.wasm
mv *.img wasmedge-image
wacret view wasmedge-image/call_stack.img > wasmedge-image/call_stack.json
