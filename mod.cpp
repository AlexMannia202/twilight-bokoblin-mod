#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"

#include "d/actor/d_a_e_oc.h"
#include <cstdint>
#include <cstddef>
#include <unordered_map>

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

// DuskLight resolves these member-function symbols from the game symbol manifest.
DEFINE_HOOK(&daE_OC_c::executeFind, BokoblinFind);
DEFINE_HOOK(&daE_OC_c::executeAttack, BokoblinAttack);
DEFINE_HOOK(&daE_OC_c::execute, BokoblinExecute);
DEFINE_HOOK(&daE_OC_c::ctrlJoint, BokoblinCtrlJoint);

namespace {
struct GuardState {
    std::uint32_t rng;
    int cooldown;
    int remaining;
};

std::unordered_map<daE_OC_c*, GuardState> g_guardStates;

std::uint32_t next_random(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

GuardState& state_for(daE_OC_c* bokoblin) {
    auto [it, inserted] = g_guardStates.try_emplace(
        bokoblin,
        GuardState{static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(bokoblin)) ^ 0xB0C0B11u,
                   90, 0});
    return it->second;
}

template <typename T>
T& actor_field(daE_OC_c* actor, std::ptrdiff_t offset) {
    return *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(actor) + offset);
}

bool is_combat_state(const daE_OC_c* bokoblin) {
    // E_OC_ACTION_FIND == 3, E_OC_ACTION_ATTACK == 4.
    const int mode = const_cast<daE_OC_c*>(bokoblin)->getActionMode();
    return mode == 3 || mode == 4;
}

HookAction on_bokoblin_execute_pre(ModContext*, void* args, void*, void*) {
    auto* bokoblin = mods::arg<daE_OC_c*>(args, 0);
    if (bokoblin == nullptr) {
        return HOOK_CONTINUE;
    }

    auto& guard = state_for(bokoblin);
    if (!is_combat_state(bokoblin)) {
        guard.remaining = 0;
        if (guard.cooldown > 0) --guard.cooldown;
        return HOOK_CONTINUE;
    }

    if (guard.remaining > 0) {
        --guard.remaining;
    } else if (guard.cooldown > 0) {
        --guard.cooldown;
    } else {
        // Roughly one defensive feint every 2-4 seconds while fighting.
        if ((next_random(guard.rng) % 100u) < 22u) {
            guard.remaining = 18; // about 0.3 seconds at 60 FPS
            guard.cooldown = 120 + static_cast<int>(next_random(guard.rng) % 120u);
        } else {
            guard.cooldown = 18;
        }
    }

    return HOOK_CONTINUE;
}

HookAction on_bokoblin_ctrl_joint_pre(ModContext*, void* args, void*, void*) {
    auto* bokoblin = mods::arg<daE_OC_c*>(args, 0);
    auto* joint = mods::arg<J3DJoint*>(args, 1);
    if (bokoblin == nullptr || joint == nullptr) {
        return HOOK_CONTINUE;
    }

    auto it = g_guardStates.find(bokoblin);
    if (it == g_guardStates.end() || it->second.remaining <= 0) {
        return HOOK_CONTINUE;
    }

    // Joint 0x0C is the Bokoblin's weapon-bearing joint: the vanilla callback
    // applies field_0x6d0 to it. Temporarily bias that existing rotation while
    // the guard window is active. This is intentionally a visual-only first
    // pass; the sign/magnitude must be judged in-game and may need tuning.
    if (joint->getJntNo() == 0x0c) {
        actor_field<s16>(bokoblin, 0x6d0) = 0x1800;
    }

    return HOOK_CONTINUE;
}

HookAction on_bokoblin_find_pre(ModContext*, void* args, void*, void*) {
    auto* bokoblin = mods::arg<daE_OC_c*>(args, 0);
    if (bokoblin == nullptr) {
        return HOOK_CONTINUE;
    }

    // E_OC_ACTION_FIND == 3. In find-state 3, shorten the original wind-up.
    if (bokoblin->getActionMode() == 3 &&
        actor_field<int>(bokoblin, 0x6b0) == 3 &&
        actor_field<s16>(bokoblin, 0x6c0) > 1) {
        actor_field<s16>(bokoblin, 0x6c0) = 1;
    }

    return HOOK_CONTINUE;
}

HookAction on_bokoblin_attack_pre(ModContext*, void* args, void*, void*) {
    auto* bokoblin = mods::arg<daE_OC_c*>(args, 0);
    if (bokoblin == nullptr) {
        return HOOK_CONTINUE;
    }

    // Keep the vanilla follow-up window alive; damage and hit values are untouched.
    actor_field<s16>(bokoblin, 0x6ca) = 180;
    return HOOK_CONTINUE;
}
} // namespace

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    ModResult result = mods::hook::add_pre<BokoblinFind>(on_bokoblin_find_pre);
    if (result != MOD_OK) return result;

    result = mods::hook::add_pre<BokoblinAttack>(on_bokoblin_attack_pre);
    if (result != MOD_OK) {
        mods::hook::uninstall<BokoblinFind>();
        return result;
    }

    result = mods::hook::add_pre<BokoblinExecute>(on_bokoblin_execute_pre);
    if (result != MOD_OK) {
        mods::hook::uninstall<BokoblinAttack>();
        mods::hook::uninstall<BokoblinFind>();
        return result;
    }

    result = mods::hook::add_pre<BokoblinCtrlJoint>(on_bokoblin_ctrl_joint_pre);
    if (result != MOD_OK) {
        mods::hook::uninstall<BokoblinExecute>();
        mods::hook::uninstall<BokoblinAttack>();
        mods::hook::uninstall<BokoblinFind>();
        return result;
    }

    mods::log::info("Twilight Bokoblin Mod experimental guard prototype initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    mods::hook::uninstall<BokoblinCtrlJoint>();
    mods::hook::uninstall<BokoblinExecute>();
    mods::hook::uninstall<BokoblinAttack>();
    mods::hook::uninstall<BokoblinFind>();
    g_guardStates.clear();
    return MOD_OK;
}
}
