#include "stdafx.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Utilities/Scanner.h>

#include <GWCA/Managers/UIMgr.h>

#include <GuiUtils.h>
#include <Utf8.h>
#include <fontawesome5.h>
#include <Modules/Resources.h>

namespace {
    ImFont* font_widget_large = nullptr;
    ImFont* font_widget_small = nullptr;
    ImFont* font_widget_label = nullptr;
    ImFont* font_header1 = nullptr;
    ImFont* font_header2 = nullptr;
    ImFont* font_text = nullptr;

    bool fonts_loading = false;
    bool fonts_loaded = false;
}
// Has GuiUtils::LoadFonts() finished?
bool GuiUtils::FontsLoaded() {
    return fonts_loaded;
}
// Loads fonts asynchronously. CJK font files can by over 20mb in size!
void GuiUtils::LoadFonts() {
    if (fonts_loaded || fonts_loading)
        return;
    fonts_loading = true;

    std::thread t([] {
        printf("Loading fonts\n");

        ImGuiIO& io = ImGui::GetIO();

        std::vector<std::pair<const wchar_t*, const ImWchar*>> fonts_on_disk;
        fonts_on_disk.emplace_back(L"Font.ttf", io.Fonts->GetGlyphRangesDefault());
        fonts_on_disk.emplace_back(L"Font_Japanese.ttf", io.Fonts->GetGlyphRangesJapanese());
        fonts_on_disk.emplace_back(L"Font_Cyrillic.ttf", io.Fonts->GetGlyphRangesCyrillic());
        fonts_on_disk.emplace_back(L"Font_ChineseTraditional.ttf", io.Fonts->GetGlyphRangesChineseFull());
        fonts_on_disk.emplace_back(L"Font_Korean.ttf", io.Fonts->GetGlyphRangesKorean());

        struct FontData
        {
            const ImWchar* glyph_ranges = nullptr;
            size_t data_size = 0;
            void* data = nullptr;
        };
        std::vector<FontData> fonts;
        for (size_t i = 0; i < fonts_on_disk.size(); ++i) {
            const auto& f = fonts_on_disk[i];
            utf8::string utf8path = Resources::GetPathUtf8(f.first);
            size_t size;
            void* data = ImFileLoadToMemory(utf8path.bytes, "rb", &size, 0);
            if (data) {
                fonts.push_back({f.second, size, data});
            } else if (i == 0) {
                // first one cannot fail
                printf("Failed to find Font.ttf file\n");
                fonts_loaded = true;
                fonts_loading = false;
                return; 
            }
        }

        // TODO: expose those in UI
        constexpr float size_text = 16.0f;
        constexpr float size_header1 = 18.0f;
        constexpr float size_header2 = 20.0f;
        constexpr float size_widget_label = 24.0f;
        constexpr float size_widget_small = 42.0f;
        constexpr float size_widget_large = 48.0f;
        static constexpr ImWchar fontawesome5_glyph_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};


        ImFontConfig cfg = ImFontConfig();
        cfg.MergeMode = false;
        cfg.PixelSnapH = true;
        cfg.FontDataOwnedByAtlas = true;
        for (const auto& font : fonts) {
            io.Fonts->AddFontFromMemoryTTF(font.data, font.data_size, size_text, &cfg, font.glyph_ranges);
            cfg.MergeMode = true; // for all but the first
        }
        io.Fonts->AddFontFromMemoryCompressedTTF(
            fontawesome5_compressed_data, fontawesome5_compressed_size, size_text, &cfg, fontawesome5_glyph_ranges);
        font_text = io.Fonts->Fonts.back();

        // All other fonts re-used the data
        cfg.FontDataOwnedByAtlas = false;

        const auto& base = fonts.front(); // base font


        cfg.MergeMode = false;
        io.Fonts->AddFontFromMemoryTTF(base.data, base.data_size, size_header1, &cfg, base.glyph_ranges);
        cfg.MergeMode = true;
        io.Fonts->AddFontFromMemoryCompressedTTF(
            fontawesome5_compressed_data, fontawesome5_compressed_size, size_header1, &cfg, fontawesome5_glyph_ranges);
        font_header2 = io.Fonts->Fonts.back();

        cfg.MergeMode = false;
        io.Fonts->AddFontFromMemoryTTF(base.data, base.data_size, size_header2, &cfg, base.glyph_ranges);

        cfg.MergeMode = true;
        io.Fonts->AddFontFromMemoryCompressedTTF(
            fontawesome5_compressed_data, fontawesome5_compressed_size, size_header2, &cfg, fontawesome5_glyph_ranges);
        font_header1 = io.Fonts->Fonts.back();

        cfg.MergeMode = false;

        io.Fonts->AddFontFromMemoryTTF(base.data, base.data_size, size_widget_label, &cfg, base.glyph_ranges);
        font_widget_label = io.Fonts->Fonts.back();

        io.Fonts->AddFontFromMemoryTTF(base.data, base.data_size, size_widget_small, &cfg, base.glyph_ranges);
        font_widget_small = io.Fonts->Fonts.back();

        io.Fonts->AddFontFromMemoryTTF(base.data, base.data_size, size_widget_large, &cfg, base.glyph_ranges);
        font_widget_large = io.Fonts->Fonts.back();

        if (!io.Fonts->IsBuilt())
            io.Fonts->Build();

        printf("Fonts loaded\n");
        fonts_loaded = true;
        fonts_loading = false;
    });
    t.detach();
}
ImFont* GuiUtils::GetFont(GuiUtils::FontSize size) {
    ImFont* font = [](FontSize size) -> ImFont* {
        switch (size) {
            case GuiUtils::FontSize::widget_large: return font_widget_large;
            case GuiUtils::FontSize::widget_small: return font_widget_small;
            case GuiUtils::FontSize::widget_label: return font_widget_label;
            case GuiUtils::FontSize::header1: return font_header1;
            case GuiUtils::FontSize::header2: return font_header2;
            case GuiUtils::FontSize::text: return font_text;
        default: return nullptr;
        }
    }(size);
    if (font && font->IsLoaded()) {
        return font;
    } else {
        return ImGui::GetIO().Fonts->Fonts[0];
    }
}

size_t GuiUtils::GetPartyHealthbarHeight() {
    GW::Constants::InterfaceSize interfacesize =
        static_cast<GW::Constants::InterfaceSize>(GW::UI::GetPreference(GW::UI::Preference_InterfaceSize));

    switch (interfacesize) {
    case GW::Constants::InterfaceSize::SMALL: return GW::Constants::HealthbarHeight::Small;
    case GW::Constants::InterfaceSize::NORMAL: return GW::Constants::HealthbarHeight::Normal;
    case GW::Constants::InterfaceSize::LARGE: return GW::Constants::HealthbarHeight::Large;
    case GW::Constants::InterfaceSize::LARGER: return GW::Constants::HealthbarHeight::Larger;
    default:
        return GW::Constants::HealthbarHeight::Normal;
    }
}

std::string GuiUtils::ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](char c) -> char {
        return static_cast<char>(::tolower(c));
    });
    return s;
}

std::wstring GuiUtils::ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) -> wchar_t {
        return static_cast<wchar_t>(::tolower(c));
    });
    return s;
}
// Convert a wide Unicode string to an UTF8 string
std::string GuiUtils::WStringToString(const std::wstring& wstr)
{
    // @Cleanup: No error handling whatsoever
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(static_cast<size_t>(size_needed), 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring GuiUtils::StringToWString(const std::string& str)
{
    // @Cleanup: No error handling whatsoever
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(static_cast<size_t>(size_needed), 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
std::wstring GuiUtils::SanitizePlayerName(std::wstring s) {
    // @Cleanup:
    // What exactly needs to be done to sanitize a player name?
    // Add this information as a comment, because nobody want to read the regex.
    if (s.empty()) return L"";
    static std::wregex remove(L" ?[\\(\\[]\\w+[\\)\\]]");
    s = std::regex_replace(s, remove, L""); 
    return s;
}
std::string GuiUtils::RemovePunctuation(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), &ispunct), s.end());
    return s;
}
std::wstring GuiUtils::RemovePunctuation(std::wstring s) {
    s.erase(std::remove_if(s.begin(), s.end(), &ispunct), s.end());
    return s;
}
bool GuiUtils::ParseInt(const char *str, int *val, int base) {
    char *end;
    *val = strtol(str, &end, base);
    if (*end != 0 || errno == ERANGE)
        return false;
    else
        return true;
}
bool GuiUtils::ParseInt(const wchar_t *str, int *val, int base) {
    wchar_t *end;
    *val = wcstol(str, &end, base);
    if (*end != 0 || errno == ERANGE)
        return false;
    else
        return true;
}
bool GuiUtils::ParseFloat(const char *str, float *val) {
    char *end;
    *val = strtof(str, &end);
    return str != end && errno != ERANGE;
}
bool GuiUtils::ParseFloat(const wchar_t *str, float *val) {
    wchar_t *end;
    *val = wcstof(str, &end);
    return str != end && errno != ERANGE;
}
bool GuiUtils::ParseUInt(const char *str, unsigned int *val, int base) {
    char *end;
    *val = strtoul(str, &end, base);
    if (str == end || errno == ERANGE)
        return false;
    else
        return true;
}
bool GuiUtils::ParseUInt(const wchar_t *str, unsigned int *val, int base) {
    wchar_t *end;
    *val = wcstoul(str, &end, base);
    if (str == end || errno == ERANGE)
        return false;
    else
        return true;
}
std::wstring GuiUtils::ToWstr(std::string &str) {
    // @Cleanup: No error handling whatsoever
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(static_cast<size_t>(size_needed), 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
size_t GuiUtils::wcstostr(char *dest, const wchar_t *src, size_t n) {
    size_t i;
    unsigned char *d = (unsigned char *)dest;
    for (i = 0; i < n; i++) {
        if (src[i] & ~0x7f)
            return 0;
        d[i] = src[i] & 0x7fu;
        if (src[i] == 0) break;
    }
    return i;
}
char *GuiUtils::StrCopy(char *dest, const char *src, size_t dest_size) {
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = 0;
    return dest;
}
