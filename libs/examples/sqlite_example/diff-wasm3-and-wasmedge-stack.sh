function cleanup_images
    rm -f wasm3-image/*
    rm -f wasmedge-image/*
end

function create_image
    wacret create --v2 --before-execution sqlite.wasm
end

function run_wasm3_checkpoint
    set -x NOP_CKPT 1
    mwasm3 --stack-size 1000000 sqlite.wasm --auto-test
    mv *.img wasm3-image/
end

function run_wasmedge_checkpoint
    set -x NOP_CKPT 1
    mwasmedge sqlite.wasm --auto-test
    mv *.img wasmedge-image/
end

function convert_to_json
    wacret view wasm3-image/call_stack.img > wasm3-image/call_stack.json
    wacret view wasmedge-image/call_stack.img > wasmedge-image/call_stack.json
end

function run_all
    cleanup_images
    create_image
    run_wasm3_checkpoint
    run_wasmedge_checkpoint
    convert_to_json
end

function diff_wasmedge_and_wasm3
    set do_cleanup 0
    set do_create 0
    set do_run_wasm3 0
    set do_run_wasmedge 0
    set do_convert 0
    set do_all 0

    # オプション解析
    argparse --name=run_wacret 'a/all' 'c/cleanup' 'b/build' 'w/wasm3' 'e/wasmedge' 'j/json' -- $argv
    or return 1

    if set -q _flag_all
        set do_all 1
    end
    if set -q _flag_cleanup
        set do_cleanup 1
    end
    if set -q _flag_build
        set do_create 1
    end
    if set -q _flag_wasm3
        set do_run_wasm3 1
    end
    if set -q _flag_wasmedge
        set do_run_wasmedge 1
    end
    if set -q _flag_json
        set do_convert 1
    end

    # 実行内容決定
    if test $do_all -eq 1
        set do_cleanup 1
        set do_create 1
        set do_run_wasm3 1
        set do_run_wasmedge 1
        set do_convert 1
    end

    # 各関数定義をインラインで
    if test $do_cleanup -eq 1
        echo "[*] Cleaning image directories..."
        rm -f wasm3-image/*
        rm -f wasmedge-image/*
    end

    if test $do_create -eq 1
        echo "[*] Creating initial image..."
        wacret create --v2 --before-execution sqlite.wasm
    end

    if test $do_run_wasm3 -eq 1
        echo "[*] Running checkpoint with wasm3..."
        set -x NOP_CKPT 1
        mwasm3 --stack-size 1000000 sqlite.wasm --auto-test
        mv *.img wasm3-image/
        wacret view wasm3-image/call_stack.img > wasm3-image/call_stack.json
    end

    if test $do_run_wasmedge -eq 1
        echo "[*] Running checkpoint with wasmedge..."
        set -x NOP_CKPT 1
        mwasmedge sqlite.wasm --auto-test
        mv *.img wasmedge-image/
        wacret view wasmedge-image/call_stack.img > wasmedge-image/call_stack.json
    end

    if test $do_convert -eq 1
        echo "[*] Converting call_stack.img to JSON..."
        wacret view wasm3-image/call_stack.img > wasm3-image/call_stack.json
        wacret view wasmedge-image/call_stack.img > wasmedge-image/call_stack.json
    end
end
