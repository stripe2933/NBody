//
// Created by gomkyung2 on 2023/09/04.
//

#pragma once

#include <string>
#include <optional>

#include <imgui.h>

template <typename ResultType, typename... Args>
struct Dialog{
protected:
    using result_t = ResultType;

    bool is_open = false;

    virtual std::optional<result_t> inner(Args ...args) = 0;

    auto closeAndPresentResult(auto &&result) {
        is_open = false;
        ImGui::CloseCurrentPopup();
        return std::forward<decltype(result)>(result);
    }

public:
    std::string name;

    /**
     * Construct dialog with name.
     * @param name ImGui identifier of dialog.
     */
    explicit Dialog(std::string name) : name { std::move(name) } {

    }

    /**
     * Open dialog.
     */
    void open(){
        is_open = true;
        ImGui::OpenPopup(name.c_str());
    }

    /**
     * Show dialog and get dialog result if presented.
     * @param name Name of dialog, which is also used for ImGui identifier.
     * @return \p std::nullopt if dialog is waiting for user input or closed without result, otherwise \p std::optional of result.
     * @note The dialog will only shows then dialog is opened, otherwise it just return \p std::nullopt .
     */
    std::optional<result_t> show(Args ...args){
        // Center dialog window.
        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });

        if (ImGui::BeginPopupModal(name.c_str(), &is_open, ImGuiWindowFlags_AlwaysAutoResize)){
            const auto result = inner(std::forward<Args>(args)...);
            if (result){
                is_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            return result;
        }

        return std::nullopt;
    }
};