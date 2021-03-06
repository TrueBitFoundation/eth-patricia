
#include <iostream>
#include <boost/multiprecision/cpp_int.hpp>
#include <stdio.h>
#include <map>

#include "info.h"

using u256 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using s256 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void>>;  

FILE *openFile(char const *fname, char const *perm) {
    std::cout << "Open file " << fname << " perm " << perm << std::endl;
    FILE *f = fopen(fname, perm);
    if (!f) {
        std::cout << "Cannot open file " << fname << std::endl;
        exit(-1);
    }
    return f;
}

u256 bytes32ToUint(uint8_t *res) {
    u256 x;
    for (int i = 0; i < 32; i++) {
        x = x*256;
        x += res[i];
    }
    return x;
}

std::vector<uint8_t> toBigEndian(u256 const &a) {
    u256 b = a;
    std::vector<uint8_t> res(32, 0);
    for (int i = res.size(); i != 0; i--) {
		res[i-1] = (uint8_t)b & 0xff;
        // b >>= 8;
        b = b / 256;
	}
    return res;
}

void put_bytes32(FILE *f, u256 a) {
    std::vector<uint8_t> v = toBigEndian(a);
    fwrite(v.data(), 1, 32, f);
}

u256 bytes20ToUint(uint8_t *res) {
    u256 x;
    for (int i = 0; i < 32; i++) {
        x = x*256;
        if (i >= 12) x += res[i-12];
    }
    return x;
}

u256 get_bytes32(FILE *f, bool &eof) {
    uint8_t *res = (uint8_t*)malloc(32);
    int ret = fread(res, 1, 32, f);
    // std::cout << "Got " << ret << std::endl;
    if (ret != 32) {
        std::cout << "Error " << ferror(f) << ": " << strerror(ferror(f)) << std::endl;
        free(res);
        eof = true;
        return 0;
    }
    u256 x;
    for (int i = 0; i < 32; i++) {
        x = x*256;
        x += res[i];
    }
    free(res);
    // std::cout << "Reading " << x << std::endl;
    return x;
}

u256 get_bytes32(FILE *f) {
    bool foo;
    return get_bytes32(f, foo);
}

std::string hex(u256 a) {
    std::vector<uint8_t> _a = toBigEndian(a);
	static char const* hexdigits = "0123456789abcdef";
	std::string hex(64, '0');
    int off = 0;
	for (int i = 0; i < 32; i++) {
		hex[off++] = hexdigits[(_a[i] >> 4) & 0x0f];
		hex[off++] = hexdigits[_a[i] & 0x0f];
	}
	return hex;
}

int main(int argc, char**argv) {
        
    FILE *f = openFile("input.data", "rb");
    u256 token_address = get_bytes32(f);
    u256 vote_address = get_bytes32(f);
    u256 start_block = get_bytes32(f);
    u256 end_block = get_bytes32(f);
    
    std::cout << "Vote address " << hex(vote_address) << std::endl;
    
    fclose(f);
    
    u256 yes_data = 0x4c6c9b78;
    u256 no_data = 0xb6c52ae8;
    
    int start = int(start_block);
    int end = int(end_block);
    
    u256 yes_votes = 0;
    u256 no_votes = 0;
    
    std::map<u256, bool> voted;
    
    for (int i = end; i >= start; i--) {
        int num = blockTransactions(i);
        std::cout << "Number of transactions: " << num << std::endl;
        for (int j = 0; j < num; j++) {
            u256 receiver = bytes20ToUint(transactionReceiver(i, j));
            std::cout << "Receiver is " << hex(receiver) << std::endl;
            
            if (receiver == vote_address) {
                u256 sender = bytes20ToUint(transactionSender(i, j));
                u256 data = bytes32ToUint(transactionData(i, j));
                if (voted.find(sender) == voted.end()) {
                    u256 balance = bytes32ToUint(accountStorage(end, toBigEndian(token_address), toBigEndian(sender)));
                    if (data == yes_data) {
                        yes_votes += balance;
                    }
                    else if (data == no_data) {
                        no_votes += balance;
                    }
                }
            }
        }
    }
    
    f = openFile("output.data", "wb");
    put_bytes32(f, yes_votes);
    put_bytes32(f, no_votes);
    fclose(f);
    return 0;
}

/*

/home/sami/emscripten-module-wrapper/../ocaml-offchain/interpreter/wasm -m -input -file record.bin -file input.data \
  -table-size 20 -stack-size 20 -memory-size 25 -wasm globals.wasm -file output.data -file block_inst  -debug-error \
  -custom 200,_readBlockTransactions,/home/sami/patricia/numtr.js \
  -custom 201,_readTransactionReceiver,/home/sami/patricia/trreceiver.js \
  -custom 202,_readTransactionSender,/home/sami/patricia/trsender.js \
  -custom 203,_readTransactionData,/home/sami/patricia/trdata.js \
  -custom 204,_readAccountStorage,/home/sami/patricia/getstorage.js

*/



