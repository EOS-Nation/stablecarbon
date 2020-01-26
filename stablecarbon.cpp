#include "stablecarbon.hpp"

// @action
void token::swap( const name from, const asset quantity )
{
   check( has_auth( from ) || has_auth( get_self() ), "missing authority of " + from.to_string() );
   check( quantity.symbol == symbol{"CUSD", 2}, "symbol precision mismatch");

   // assets
   const symbol USDT = symbol{"USDT", 4};
   const string memo = "1:1 CUSD/USDT swap";
   const asset usdt = asset{ quantity.amount * 100, USDT };

   // check if existing USDT balance is available
   check( token::get_balance( "tethertether"_n, get_self(), USDT.code()) > usdt, "please wait until USDT balance is refilled, for further questions please contact https://www.carbon.money or https://t.me/carbon_money");

   // static actioncs
   token::burn_action burn( get_self(), { get_self(), "active"_n });
   token::transfer_action transfer( "tethertether"_n, { get_self(), "active"_n });

   // transfer USDT at 1:1
   burn.send( from, quantity, memo );
   transfer.send( get_self(), from, usdt, memo );
}

// @action
void token::burn( const name from, const asset quantity, const string memo )
{
   check( has_auth( from ) || has_auth( get_self() ), "missing authority of " + from.to_string() );

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

   // prevent transfer to exchanges
   if ( to == "dexeoswallet"_n || to == "newdexpublic"_n || to == "yorescusd112"_n ) {
      check( false, "CUSD transfer action has been disabled for DEX, please wait for official news from https://www.carbon.money or https://t.me/carbon_money");
   }
   // prevent blacklist accounts
   if ( from == "stringbeanzz"_n ) {
      check( false, "CUSD transfer action has been disabled, please wait for official news from https://www.carbon.money or https://t.me/carbon_money");
   }

   check( from != to, "cannot transfer to self" );
   check( is_account( to ), "to account does not exist");

   auto sym = quantity.symbol.code();
   stats statstable( get_self(), sym.raw() );
   const auto& st = statstable.get( sym.raw() );

   require_recipient( from );
   require_recipient( to );

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must transfer positive quantity" );
   check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   auto payer = get_self();

   sub_balance( from, quantity );
   add_balance( to, quantity, payer );
}

// @action
void token::close( const name& owner, const symbol& symbol )
{
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, get_self(), [&]( auto& a ) {
      a.balance -= value;
      if ( a.balance.amount <= 0 ) from_acnts.erase( from );
   });
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

