//
// Created by gomkyung2 on 2023/09/14.
//

#pragma once

#include <imgui.h>

namespace ImGui{
    struct ScopedDisabled{
        ScopedDisabled(bool disabled = true){
            ImGui::BeginDisabled(disabled);
        }

        ~ScopedDisabled() noexcept{
            ImGui::EndDisabled();
        }
    };
}