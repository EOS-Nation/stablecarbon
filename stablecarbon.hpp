#pragma once

#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/eosio.hpp>

#include <string>

using namespace eosio;
using namespace std;

class [[eosio::contract("stablecarbon")]] token : public contract {
   public:
      using eosio::contract::contract;

      [[eosio::action]]
      void burn( const name from, const asset quantity, const string memo );

      [[eosio::action]]
      void transfer( const name&    from,
                     const name&    to,
                     const asset&   quantity,
                     const string&  memo );

      [[eosio::action]]
      void close( const name owner, const symbol symbol );

      [[eosio::action]]
      void closeall( const name owner );

      [[eosio::action]]
      void swap( const name account );

      [[eosio::action]]
      void unauthorize( const name account, const bool active );

      static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
      {
         stats statstable( token_contract_account, sym_code.raw() );
         const auto& st = statstable.get( sym_code.raw() );
         return st.supply;
      }

      static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
         accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw(), "owner has no remaining balance" );
         return ac.balance;
      }

      using burn_action = eosio::action_wrapper<"burn"_n, &token::burn>;
      using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
      using close_action = eosio::action_wrapper<"close"_n, &token::close>;
   private:
      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] unauthorize_row {
         name    account;

         uint64_t primary_key() const { return account.value; }
      };

      struct [[eosio::table]] currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;

         uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };

      typedef eosio::multi_index< "unauthorize"_n, unauthorize_row > unauthorize_table;
      typedef eosio::multi_index< "accounts"_n, account > accounts;
      typedef eosio::multi_index< "stat"_n, currency_stats > stats;

      void sub_balance( const name& owner, const asset& value );
      void add_balance( const name& owner, const asset& value, const name& ram_payer );
      void swap_transfer( const eosio::name from, const eosio::name to, const eosio::asset quantity);
      void check_unauthorize( const name account );
};
