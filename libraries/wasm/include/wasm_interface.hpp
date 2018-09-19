#pragma once

#include <stdint.h>
#include <graphene/chain/protocol/types.hpp>
//#include <graphene/chain/exceptions.hpp>
#include "exceptions.hpp"
#include "Runtime/Linker.h"
#include "Runtime/Runtime.h"
#include <fc/exception/exception.hpp>
#include <boost/container/string.hpp>
#include "apply_context.hpp"
#include "controller.hpp"
#include "transaction_context.hpp"

using std::string;
using std::map;
using std::unique_ptr;
using namespace Runtime;
using graphene::chain::digest_type;
using IR::Module;
using shared_string = boost::container::basic_string<char, std::char_traits<char>, std::allocator<char>>;
using bytes         = std::vector<char>;

namespace eosio { namespace chain {

    struct wasm_interface_impl;

   struct wasm_exit {
      int32_t code = 0;
   };

   namespace webassembly { namespace common {
      class intrinsics_accessor;

      struct root_resolver : Runtime::Resolver {
         //when validating is true; only allow "env" imports. Otherwise allow any imports. This resolver is used
         //in two cases: once by the generic validating code where we only want "env" to pass; and then second in the
         //wavm runtime where we need to allow linkage to injected functions
         root_resolver(bool validating = false) : validating(validating) {}
         bool validating;

         bool resolve(const string& mod_name,
                      const string& export_name,
                      IR::ObjectType type,
                      Runtime::ObjectInstance*& out) override {
      try {
         //protect access to "private" injected functions; so for now just simply allow "env" since injected functions
         //  are in a different module
         if(validating && mod_name != "env")
            EOS_ASSERT( false, wasm_exception, "importing from module that is not 'env': ${module}.${export}", ("module",mod_name)("export",export_name) );

         // Try to resolve an intrinsic first.
         if(Runtime::IntrinsicResolver::singleton.resolve(mod_name,export_name,type, out)) {
            return true;
         }

         EOS_ASSERT( false, wasm_exception, "${module}.${export} unresolveable", ("module",mod_name)("export",export_name) );
         return false;
      } FC_CAPTURE_AND_RETHROW( (mod_name)(export_name) ) }
      };
   } }

   /**
    * @class wasm_interface
    *
    */
   class wasm_interface {
      public:
         enum class vm_type {
            wavm,
            binaryen,
         };

         wasm_interface(vm_type vm);
         ~wasm_interface();

         //validates code -- does a WASM validation pass and checks the wasm against EOSIO specific constraints
         static void validate(const controller& control, const bytes& code);

         //Calls apply or error on a given code
         void apply(const digest_type& code_id, const shared_string& code, apply_context& context);

      private:
         unique_ptr<wasm_interface_impl> my;
         friend class eosio::chain::webassembly::common::intrinsics_accessor;
   };

} } // eosio::chain

namespace eosio{ namespace chain {
   std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime);
}}

FC_REFLECT_ENUM( eosio::chain::wasm_interface::vm_type, (wavm)(binaryen) )
