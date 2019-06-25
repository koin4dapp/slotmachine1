/*
created by koin4dapp.appspot.com
(bettors community token)
warning, this is only for learning purpose, use it as your own risk
*/

#include <eosiolib/transaction.h>
#include <string>

class random {
private:
    uint64_t seed;

public:
    void init(uint64_t initseed) {
        auto s = read_transaction(nullptr,0);
        char *tx = (char *)malloc(s);
        read_transaction(tx, s);  //packed_trx
        capi_checksum256 result;
        sha256(tx,s, &result);    //txid

        seed = result.hash[7];
        seed <<= 8;
        seed ^= result.hash[6];
        seed <<= 8;
        seed ^= result.hash[5];
        seed <<= 8;
        seed ^= result.hash[4];
        seed <<= 8;
        seed ^= result.hash[3];
        seed <<= 8;
        seed ^= result.hash[2];
        seed <<= 8;
        seed ^= result.hash[1];
        seed <<= 8;
        seed ^= result.hash[0];
        seed ^= initseed;
        seed ^= (tapos_block_prefix()*tapos_block_num()); //current block
    }
    
    void randraw() { 
        capi_checksum256 result; //32bytes of 8 chunks of uint_32
        sha256((char *)&seed, sizeof(seed), &result);
        seed = result.hash[7];
        seed <<= 8;
        seed ^= result.hash[6];
        seed <<= 8;
        seed ^= result.hash[5];
        seed <<= 8;
        seed ^= result.hash[4];
        seed <<= 8;
        seed ^= result.hash[3];
        seed <<= 8;
        seed ^= result.hash[2];
        seed <<= 8;
        seed ^= result.hash[1];
        seed <<= 8;
        seed ^= result.hash[0];
    }
    
    uint64_t rand(uint64_t to) { //generate random 1 - to
        randraw();
        return (seed % to) + 1;
    }
    
    uint64_t getseed() {
      return (seed>>8);
    }
};