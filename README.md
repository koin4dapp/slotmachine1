# SlotMachine1

This is smart contract provide slot machine game, transfer EOS to play.

created by koin4dapp.appspot.com
(bettors community token)

## What you learn

1. Singleton for saving data
2. Table for saving data
3. Inline action for user notification
4. Deffered action for settle payment
5. Receive EOS token and execute action
6. Create random number
7. Secure your smart contract from rollback attack
8. Frontend Web Integration using Scatter-JS (please visit https://github.com/koin4dapp/slotmachine1)
9. Frontend Web Integration using Lynx-Wallet (please visit https://github.com/koin4dapp/slotmachine1)

## How to use:

1. Open file setting.hpp
2. Change to your target platform
3. Set permission eosio.code to your contract account

```
//change to your target platform eosio/sister/side chain (EOS/TLOS/BOS/MEETONE)
#define MAIN_TOKEN "EOS" //EOS/TLOS/BOS/MEETONE
#define MAIN_CONTRACT "eosio.token"
#define MIN_PLAY 1E4 //minimum token amount, 1E4=1.0000 EOS
#define TOKEN_MESSAGE "only EOS tokens allowed"
#define DELAYSETTLE 1 //1 second, adjust to prevent result attack
```

## What this smart contract do
1. Accept mainnet token transfer

```
extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    if (code == slotmachine1::eosio_contract().value && action == "transfer"_n.value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &slotmachine1::ontransfer
        );
    }
    else if (code == receiver)
    {
        switch (action)
        {
        EOSIO_DISPATCH_HELPER(slotmachine1, (initgame))
        }
    }
}
```
2. Execute ontransfer action function to validate
```
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
  spin(from, quantity);
}
```
3. Call spin action to start game session and determine payback
```
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
      sign = 8; //payback (7*7*7)/(7*6)=8x

    eosio::asset payback = eosio::asset(sign*quantity.amount,quantity.symbol);
    
    std::string feedback = " SPIN result:" + std::to_string(slot1*100+slot2*10+slot3) 
      + " you " + (payback.amount>0?"won":"loss");
    
    eosio::print(feedback+"\n");

    send_notify(user, feedback);
    if (payback.amount > 0)
      send_settle(user, payback, feedback);
}
```
4. Check if game enabled
```
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
```
5. Read and write to singleton table
```
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
```
6. Generate Random number
```
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
```
7. Send result notification to user using inline action
```
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

//public
void slotmachine1::notify( name to, std::string memo ) 
{
    require_auth( _self ); //security
    require_recipient(to);
}
```
8. If user won, sent payment to user via deffered transaction (prevent check balance attack)
```
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
```
9. Check if transaction exist before transfer payback (prevent rollback attack)
```
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
```
# How to deploy
1. Create Testnet account at Kylin Testnet
```
http://faucet.cryptokylin.io/create_account?slotmachine1

record your private key
```
2. Get free EOS token from Kylin Testnet
```
http://faucet.cryptokylin.io/get_token?slotmachine

your can run several time for enough EOS to buy RAM
```
3. Register Key using Key Manager
4. Change to Kylin Network
5. Bookmark your account name (eg. slotmachine1)
6. Compile your smart contract
7. Buy RAM min 11x wasm binary file size
```
Send Transfer
Tx:8a7c5c
Jun-25-2019, 16:25:48
slotmachine1 → eosio.ramfee 0.0621 EOS
,

MEMO: ram fee

Send Transfer
Tx:8a7c5c
Jun-25-2019, 16:25:48
slotmachine1 → eosio.ram 10.4032 EOS
,

MEMO: buy ram

Buy RAM Bytes
Tx:8a7c5c
Jun-25-2019, 16:25:48
slotmachine1 bought 519400 bytes of RAM for slotmachine1
```
8. Set your account permission to eosio.code
```
Update Auth
Tx:d93b69
Jun-25-2019, 16:25:07
Configuration:

account:    slotmachine1
permission: active
parent:     owner
auth: 
  threshold: 1
  keys: 
    - 
      key:    EOS6RhqBP44jfX8F6FLc7pFPZ3eo92QXARYjUqtSZm2vzgYYTAMRg
      weight: 1
  accounts: 
    - 
      permission: 
        actor:      slotmachine1
        permission: eosio.code
      weight:     1
  waits: 
    (empty array)
```
9. Deploy to your account (eg. slotmachine1)
```
Set ABI
Tx:c522e1
Jun-25-2019, 16:30:45
Contract ABI deployed to slotmachine1

account: slotmachine1
abi:     0e656f73696f3a3a6162... (hash)

,
Set Code
Tx:7b7c59
Jun-25-2019, 16:30:42
Contract code deployed to slotmachine1

account:   slotmachine1
vmtype:    0
vmversion: 0
code:      0061736d0100000001f8... (hash)
```
10. Bookmark your smart contract (eg. slotmachine1)
11. Run initial action and set enabled to 1
```
slotmachine1: initgame
slotmachine1 (contract) processed the following data

enabled: 1
```
12. Check your smart contract at Kylin Testnet
```
https://kylin.eosx.io/account/slotmachine1

you will see Contract Detected, you can view CONTRACT DETAILS, CONTRACT TABLE and SENT CONTRACT ACTION
```
12. Congratulation, you are success deploy your first smart contract

# How to Test
1. Create Testnet account at Kylin Testnet
```
http://faucet.cryptokylin.io/create_account?slotmachine1

record your private key
```
2. Get free EOS token from Kylin Testnet
```
http://faucet.cryptokylin.io/get_token?slotmachine

your can run several time for enough EOS to buy RAM
```
3. Register Key using Key Manager
4. Bookmark your account name (eg. slotmachine2)
5. Change account to slotmachine2
6. Change contract to eosio.token
7. Transfer 1.0000 EOS from slotmachine2 to slotmachine1
```
txid: xxx
```
8. Check your account at Kylin Testnet
```
https://kylin.eosx.io/account/slotmachine2

Send Transfer
slotmachine2 → slotmachine1 1.0000 EOS
MEMO: play

slotmachine1 (contract) processed the following data

to:   slotmachine2
memo: slotmachine2 SPIN result:613 you loss

```
6. Congratulation, you are success run your first smart contract
