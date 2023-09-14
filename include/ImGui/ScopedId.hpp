//
// Created by gomkyung2 on 2023/09/14.
//

#pragma once

#include <imgui.h>

namespace ImGui{
    struct ScopedId{
        explicit ScopedId(const char *id){
            ImGui::PushID(id);
        }

        explicit ScopedId(int id){
            ImGui::PushID(id);
        }

        explicit ScopedId(void *id){
            ImGui::PushID(id);
        }

        ~ScopedId() noexcept{
            ImGui::PopID();
        }
    };
}