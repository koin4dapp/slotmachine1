/*
created by koin4dapp.appspot.com
(bettors community token)
warning, this is only for learning purpose, use it as your own risk
*/

#include <slotmachine1.hpp>
#include <eosiolib/transaction.hpp>
#include <string>

ACTION slotmachine1::initgame(uint64_t enabled) {
  require_auth(_self);
  rseed savedseed(_self, ("secretscope"_n).value);
  savedseed.set(seed{enabled,now()},_self);
}

//private
uint64_t slotmachine1::isenabled() {
  rseed savedseed(_self,("secretscope"_n).value);
  auto result = savedseed.get();
  return result.enabled;
}
    
//private
void slotmachine1::ontransfer( name from, 
                  name to,
                  asset quantity,
                  std::string memo )
{
  if (from == _self) {
        // we're sending money, do nothing additional
        return;
  }
  eosio_assert(to == _self, "not to our contract");
  eosio_assert(quantity.symbol.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "only positive quantity allowed");
  
  eosio_assert(quantity.symbol == eosio_symbol(), TOKEN_MESSAGE);
  eosio::print("spin");
  eosio::print("\n");
  spin(from, quantity);
}

//private
void slotmachine1::spin( name user, 
                asset quantity)
{
    eosio_assert(isenabled()==1, "slot machine not enabled yet!");
    auto sym = quantity.symbol;
    //invalid symbol name, token symbol not match, invalid quantity, quantity not reach minimum allowed
    check_asset(quantity, MIN_PLAY,1);
    
    random random;
    random.init(get_lastseed() ^ user.value); //get seed from previous session
    uint64_t slot1=random.rand(7); //slot 1 number (1-7)
    uint64_t slot2=random.rand(7); //slot 2 number (1-7)
    uint64_t slot3=random.rand(7); //slot 3 number (1-7)
    
    set_nextseed(random.getseed()); //save for seed for next session
    
    double sign =0;
    if (slot1==slot2 && slot2==slot3 && slot1!=7) //and not all jocker
      sign = 57; //payback (7*7*7)/6=57x
    else if ((slot1==slot2 && slot3==7)
        || (slot1==slot3 && slot2==7)
        || (slot2==slot3 && slot1==7)
        || (slot1==slot2 && slot1==7)
        || (slot1==slot3 && slot1==7)
        || (slot2==slot3 && slot2==7))
      double sign = 8; //payback (7*7*7)/(7*6)=8x

    eosio::asset payback = eosio::asset(sign*quantity.amount,quantity.symbol);
    
    std::string feedback = " SPIN result:" + std::to_string(slot1*100+slot2*10+slot3) 
      + " you " + (payback.amount>0?" won":"loss");
    
    eosio::print(feedback+"\n");

    send_notify(user, feedback);
    if (payback.amount > 0)
      send_settle(user, payback, feedback);
}

//private
void slotmachine1::send_notify(name to, std::string memo)
{
  //need permission eosio.code, check cleos set permission
  action(
    permission_level{ _self,"active"_n},
    _self,
    "notify"_n,
    std::make_tuple(to, name{to}.to_string() + memo)
  ).send();
    
}

//public action
ACTION slotmachine1::notify( name to, std::string memo ) 
{
    require_auth( _self ); //security
    require_recipient(to);
}

//private
void slotmachine1::send_settle(name to, asset payback, std::string memo)
{
  
  //record transaction to prevent rollback attack
  vsettles vsettlestable( _self, to.value);
  uint64_t vskey=vsettlestable.available_primary_key();
  vsettlestable.emplace( _self, [&]( auto& v ){
      v.key = vskey;
      v.payback = payback;
  });
  //need permission eosio.code, check cleos set permission
  eosio::transaction out{};
    out.actions.emplace_back(
      permission_level{_self, "active"_n},
      _self,
      "settle"_n,
      std::make_tuple(to, payback, memo)
    );
    out.delay_sec = DELAYSETTLE;
  out.send(to.value, _self); //using user value to prevent double transaction from same player
}

//public action
ACTION slotmachine1::settle(uint64_t vskey, name to, asset payback, std::string memo )
{
  require_auth( _self ); //security
  
  //validate settle record, to prevent rollback attack
  vsettles vsettlestable( _self, to.value);
  auto settle = vsettlestable.find( vskey );
  if (settle != vsettlestable.end()) {
    
    if (settle->payback==payback) {
  
      //need permission eosio.code, check cleos set permission
      action(
        permission_level{ _self,"active"_n},
        eosio_contract(),
        "transfer"_n,
        std::make_tuple( _self, to, payback, memo) //sent back TLOS
      ).send();
    }
    
    vsettlestable.erase(settle); //erase settled record
  }
}