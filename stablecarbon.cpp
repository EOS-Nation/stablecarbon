#include "stablecarbon.hpp"

void token::swap( const eosio::name account )
{
   require_auth( get_self() );

   const asset balance = token::get_balance( "stablecarbon"_n, account, symbol_code{"CUSD"} );
   check( balance.amount > 0, "account has no balance");

   sub_balance( account, balance );
   add_balance( "pipepipepipe"_n, balance, get_self() );

   swap_transfer( account, "pipepipepipe"_n, balance );
}

void token::swap_transfer( const eosio::name from, const eosio::name to, const eosio::asset quantity )
{
   if ( from == get_self() ) return;

   // Only monitor incoming transfers to get_self() account
   check( to != get_self(), "please send CUSD tokens to \"pipepipepipe\" to receive 1:1 CUSD/USDT swap, for further questions please contact https://www.carbon.money or https://t.me/carbon_money");

   // only monitor authorized account for 1:1 CUSD/USDT swap
   if ( to != "pipepipepipe"_n) return;

   check( quantity.symbol == symbol{"CUSD", 2}, "symbol precision mismatch");

   // assets
   const name usdt_holder = "pipepipepipe"_n;
   const symbol USDT = symbol{"USDT", 4};
   const asset usdt = asset{ quantity.amount * 100, USDT };

   // check if existing USDT balance is available
   check( token::get_balance( "tethertether"_n, usdt_holder, USDT.code()) >= usdt, "please wait until USDT balance is refilled, for further questions please contact https://www.carbon.money or https://t.me/carbon_money");

   // static actioncs
   token::transfer_action transfer( "tethertether"_n, { usdt_holder, "active"_n });

   // transfer USDT at 1:1
   transfer.send( usdt_holder, from, usdt, "1:1 CUSD/USDT swap" );
}

// @action
void token::burn( const name from, const asset quantity, const string memo )
{
   check( has_auth( get_self() ) || has_auth( from ), "missing authority of " + from.to_string());

   check_unauthorize( from );

   auto sym = quantity.symbol;
   check( sym.is_valid(), "invalid symbol name" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   stats statstable( get_self(), sym.code().raw() );
   auto existing = statstable.find( sym.code().raw() );
   check( existing != statstable.end(), "token with symbol does not exist" );
   const auto& st = *existing;

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must retire positive quantity" );

   check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.supply -= quantity;
   });

   sub_balance( from, quantity );
}

// @action
void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
   require_auth( from );

   check_unauthorize( from );
   check_unauthorize( to );

   // prevent transfer to exchanges
   if ( to == "bancorc11144"_n || to == "thisisbancor"_n || to == "dexeoswallet"_n || to == "newdexpublic"_n || to == "yorescusd112"_n ||
        from == "bancorc11144"_n || from == "thisisbancor"_n || from == "yorescusd112"_n ) {
      check( false, "CUSD transfer action has been disabled for DEX, please wait for official news from https://www.carbon.money or https://t.me/carbon_money");
   }

   check( from != to, "cannot transfer to self" );
   check( is_account( to ), "to account does not exist");

   auto sym = quantity.symbol.code();
   stats statstable( get_self(), sym.raw() );
   const auto& st = statstable.get( sym.raw() );

   require_recipient( from );
   require_recipient( get_self() );
   require_recipient( to );

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must transfer positive quantity" );
   check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   auto payer = get_self();

   sub_balance( from, quantity );
   add_balance( to, quantity, payer );

   swap_transfer( from, to, quantity );
}

// @action
void token::close( const name owner, const symbol symbol )
{
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

// @action
void token::closeall( const name owner )
{
   require_auth( get_self() );

   accounts _accounts( get_self(), owner.value );

   auto account = _accounts.begin();
   while ( account != _accounts.end() ) {
      check( account->balance.amount == 0, "Cannot close because the balance is not zero." );
      account = _accounts.erase( account );
   }
}

// @action
void token::unauthorize( const name account, const bool active )
{
   require_auth( get_self() );

   unauthorize_table _unauthorize( get_self(), get_self().value );
   auto itr = _unauthorize.find( account.value );

   if ( active ) {
      check( itr != _unauthorize.end(), "[account] is not unauthorized");
      _unauthorize.erase( itr );

   } else {
      check( itr == _unauthorize.end(), "[account] is already unauthorized");

      _unauthorize.emplace( get_self(), [&]( auto& row ) {
         row.account = account;
      });
   }
}

void token::check_unauthorize( const name account )
{
   if ( !has_auth( get_self() ) ) {
      unauthorize_table _unauthorize( get_self(), get_self().value );
      auto itr = _unauthorize.find( account.value );
      check( itr == _unauthorize.end(), "this account is unauthorized, please contact https://www.carbon.money or https://t.me/carbon_money");
   }
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   bool is_empty = false;
   from_acnts.modify( from, get_self(), [&]( auto& a ) {
      a.balance -= value;
      if ( a.balance.amount <= 0 ) is_empty = true;
   });
   if ( is_empty ) from_acnts.erase( from );
}

void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( get_self(), [&]( auto& a ){
         a.balance = value;
      });
   } else {
      to_acnts.modify( to, get_self(), [&]( auto& a ) {
         a.balance += value;
      });
   }
}

