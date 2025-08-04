./build/replace -lef ./test/sha3/NanGate_45nm.lef -def ./test/sha3/sha3_45nm.def -verilog ./test/sha3/sha3_SYN.v -lib ./test/sha3/NanGate_45nm_slow.lib -sdc ./test/sha3/sha3.sdc -timing -resPerMicron 2.5357 -capPerMicron 27.2261e-17 -output ./TTest | tee test.log

./build/replace -lef ./test/sha3/NanGate_15nm.lef -def ./test/sha3/sha3_15nm.def -verilog ./test/sha3/sha3_SYN.v -lib ./test/sha3/NanGate_15nm_slow.lib -sdc ./test/sha3/sha3.sdc -timing -resPerMicron 33.2811 -capPerMicron 7.5151e-17 -output ./TTest | tee test.log

net weighting option:
    -maxnw 1024 -nwDecay 0.5

for cadb:
./build/replace -lef ./test/sha3/NanGate_45nm.lef -def ./test/sha3/sha3_45nm.def -verilog ./test/sha3/sha3_SYN.v -lib ./test/sha3/NanGate_45nm_slow.lib -sdc ./test/sha3/sha3.sdc -resPerMicron 2.5357 -capPerMicron 27.2261e-17 -maxnw 1024 -nwDecay 0.5 -cadb23in ./test/sha3/sha3_cadb.in -cadb23out ./test/sha3/sha3_cadb.out -skipIP -output ./TTest | tee test.log