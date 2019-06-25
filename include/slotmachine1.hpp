/*
created by koin4dapp.appspot.com
(bettors community token)
warning, this is only for learning purpose, use it as your own risk
*/

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/print.hpp>
#include "setting.hpp"
#include "random.hpp"

using namespace eosio;

CONTRACT slotmachine1 : public contract {
  public:
    using contract::contract;
    slotmachine1(eosio::name receiver, eosio::name code, datastream<const char*> ds):contract(receiver, code, ds) {}
    
    static symbol eosio_symbol() { return symbol(MAIN_TOKEN,4); }
    static name eosio_contract() { return name(MAIN_CONTRACT); }
    
    ACTION initgame(uint64_t enabled);

    void ontransfer(eosio::name from, 
                    eosio::name to,
                    eosio::asset quantity,
                    std::string memo
    );
    
    //public
    ACTION notify( name to, std::string memo );
    ACTION settle(uint64_t vskey, name to, asset payback, std::string memo );
    
  private:
  
    //table for record transaction to prevent rollback attack
    struct [[eosio::table]] valid_settle {
      uint64_t  key;
      asset     payback;
        
      uint64_t primary_key()const { return key; }
    };
    
    typedef eosio::multi_index< "vsettles"_n, valid_settle > vsettles;
         
    //singletone for save dapp random seed
    struct seed {
      uint64_t  enabled;
      uint64_t  lastseed;
    };
    
    //for security reason, change table name "rsecret"_n to your secret valid eosio name [a-z,1-5,.]
    //even though they are not visible in ABI file, but still can be access via other smart contract
    typedef eosio::singleton <"rsecret"_n,seed> rseed;
    
    uint64_t isenabled();
    
    //singletone setter/getter for random seed, better use external oracle service
    uint64_t get_lastseed() {
      //for security reason, change scope "secretscope" to your secret valid eosio name [a-z,1-5,.]
      rseed savedseed(_self,("secretscope"_n).value);
      auto result = savedseed.get_or_create(_self, seed{1,now()});
      return result.lastseed;
    }
         
    void set_nextseed( uint64_t lastseed ) {
      rseed savedseed(_self, ("secretscope"_n).value);
      savedseed.set(seed{1,lastseed}, _self);
    }
    
    void check_asset( asset quantity, uint64_t minvalue, uint8_t nonfloat) {
      eosio_assert( quantity.symbol.is_valid(), "invalid symbol name" );
      eosio_assert( quantity.symbol == eosio_symbol(), "token symbol not match");
      eosio_assert( quantity.is_valid(), "invalid quantity");
      eosio_assert( quantity.amount > 0, "quantity must > 0");
      eosio_assert( quantity.amount >= minvalue, "quantity not reach minimum allowed");
      if (nonfloat > 0)
        eosio_assert( (quantity.amount%10000)==0, "quantity must integer");
    }
    
    void spin( name user, asset quantity);
    void send_settle(name to, asset payback, std::string memo);
    void send_notify(name to, std::string memo);
};

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
#if DEBUG
    eosio::print("receiver:");
    eosio::print(receiver);
    eosio::print("code:");
    eosio::print(code);
    eosio::print("action:");
    eosio::print(action);
    eosio::print("\n");
#endif

    if (code == slotmachine1::eosio_contract().value && action == "transfer"_n.value)
    {
        //proses transfer from user to our contract account
        //eosio::print("ontransfer");
        //eosio::print("\n");
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &slotmachine1::ontransfer
        );
    }
    else if (code == receiver)
    {
        switch (action)
        {
        EOSIO_DISPATCH_HELPER( slotmachine1, (initgame)(notify)(settle))
        }
    }
}