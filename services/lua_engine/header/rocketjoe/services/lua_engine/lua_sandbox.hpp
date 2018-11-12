#pragma  once

#include <array>

#include <rocketjoe/services/lua_engine/device.hpp>
#include <rocketjoe/api/transport_base.hpp>
#include <sol.hpp>
#include <goblin-engineer/abstract_service.hpp>

namespace rocketjoe { namespace services { namespace lua_engine { namespace lua_vm {

                using id = std::size_t;

                class lua_context final {
                public:
                    lua_context(const std::string&,actor_zeta::behavior::context_t&);

                    auto push_job(api::transport &&job) -> void;

                    auto run() -> void;

                private:
                    device<api::transport> device_;
                    sol::environment environment;
                    sol::state lua;
                    sol::load_result r;
                    actor_zeta::behavior::context_t& context_;
                    std::unique_ptr<std::thread> exuctor;  ///TODO: HACK
                };
            }


        }
    }
}