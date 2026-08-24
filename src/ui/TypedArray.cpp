#include "TypedArray.h"
#include "../ui/LayoutInflater.h"
#include "../utils/Logger.h"

#include "DrawableInflater.h"
#include "ColorStateListInflater.h"
#include "../graphics/drawable/ColorDrawable.h"

#include "androidfw/AttributeResolution.h"
#include "androidfw/ResourceUtils.h"

namespace setu {

TypedArray::TypedArray(ResourceManager* resManager, const std::vector<uint32_t>& styleables)
    : m_resManager(resManager), m_styleables(styleables) {
    m_values.resize(styleables.size());
    m_stringValues.resize(styleables.size());
    m_resIds.resize(styleables.size(), 0);
    m_hasValue.resize(styleables.size(), false);
}

TypedArray::~TypedArray() {
}

void TypedArray::obtainStyledAttributes(const Theme* theme, ::android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes) {
    m_theme = const_cast<Theme*>(theme);

    std::vector<uint32_t> outValues(m_styleables.size() * ::android::STYLE_NUM_ENTRIES, 0);
    // out_indices must be sized attrs_length + 1 because the first element stores the count of valid indices
    std::vector<uint32_t> outIndices(m_styleables.size() + 1, 0);

    ::android::Theme* nativeTheme = theme ? const_cast<::android::Theme*>(theme->getTheme()) : nullptr;
    
    if (nativeTheme) {
        auto result = ::android::ApplyStyle(nativeTheme, parser, defStyleAttr, defStyleRes,
                                      m_styleables.data(), m_styleables.size(),
                                      outValues.data(), outIndices.data());
        if (!result.has_value()) {
            Logger::w("TypedArray", "Failed to apply style in obtainStyledAttributes.");
            return;
        }
    } else {
        ::android::AssetManager2* am = m_resManager ? m_resManager->getAssetManager() : nullptr;
        if (!am) {
            Logger::w("TypedArray", "No theme and no AssetManager, cannot obtain styled attributes.");
            return;
        }
        auto result = ::android::RetrieveAttributes(am, parser,
                                      m_styleables.data(), m_styleables.size(),
                                      outValues.data(), outIndices.data());
        if (!result.has_value()) {
            Logger::w("TypedArray", "Failed to retrieve attributes in obtainStyledAttributes.");
            return;
        }
    }

    for (size_t i = 0; i < m_styleables.size(); ++i) {
        uint32_t type = outValues[i * ::android::STYLE_NUM_ENTRIES + ::android::STYLE_TYPE];
        if (type != ::android::Res_value::TYPE_NULL) {
            ::android::AssetManager2::SelectedValue val;
            val.type = type;
            val.data = outValues[i * ::android::STYLE_NUM_ENTRIES + ::android::STYLE_DATA];
            
            int32_t javaCookie = outValues[i * ::android::STYLE_NUM_ENTRIES + ::android::STYLE_ASSET_COOKIE];
            val.cookie = (javaCookie != -1) ? (javaCookie - 1) : ::android::kInvalidCookie;
            
            val.flags = 0;
            val.resid = outValues[i * ::android::STYLE_NUM_ENTRIES + ::android::STYLE_RESOURCE_ID];

            if (m_resManager) {
                m_resManager->resolveValue(val, const_cast<Theme*>(theme));
            }

            // Held for getDrawable(): resolveValue() rewrites resid as it follows
            // an alias chain, so this is the ID the value finally came from.
            m_resIds[i] = val.resid;

            ::android::Res_value resVal;
            resVal.dataType = val.type;
            resVal.data = val.data;

            m_values[i] = resVal;
            m_hasValue[i] = true;
            
            if (val.type == ::android::Res_value::TYPE_STRING) {
                if (val.cookie != ::android::kInvalidCookie) {
                    if (m_resManager && m_resManager->getAssetManager()) {
                        auto pool = m_resManager->getAssetManager()->GetStringPoolForCookie(val.cookie);
                        if (pool) {
                            auto str_exp = pool->stringAt(val.data);
                            if (str_exp.has_value()) {
                                m_stringValues[i] = ::android::util::Utf16ToUtf8(str_exp.value());
                            } else {
                                auto str8_exp = pool->string8At(val.data);
                                if (str8_exp.has_value()) {
                                    m_stringValues[i] = std::string(str8_exp.value());
                                }
                            }
                        }
                    }
                } else if (parser != nullptr) {
                    const ::android::ResStringPool& pool = parser->getStrings();
                    auto str_exp = pool.stringAt(val.data);
                    if (str_exp.has_value()) {
                        m_stringValues[i] = ::android::util::Utf16ToUtf8(str_exp.value());
                    } else {
                        auto str8_exp = pool.string8At(val.data);
                        if (str8_exp.has_value()) {
                            m_stringValues[i] = std::string(str8_exp.value());
                        }
                    }
                }
            }
        }
    }
}

bool TypedArray::hasValue(int index) const {
    if (index < 0 || index >= m_hasValue.size()) return false;
    return m_hasValue[index];
}

bool TypedArray::getBoolean(int index, bool defValue) const {
    if (!hasValue(index)) return defValue;
    return m_values[index].data != 0;
}

int TypedArray::getInt(int index, int defValue) const {
    if (!hasValue(index)) return defValue;
    uint8_t type = m_values[index].dataType;
    if (type == ::android::Res_value::TYPE_REFERENCE) return defValue; // Unresolved reference
    return (int)m_values[index].data;
}

float TypedArray::getFloat(int index, float defValue) const {
    if (!hasValue(index)) return defValue;
    uint8_t type = m_values[index].dataType;
    if (type == ::android::Res_value::TYPE_REFERENCE) return defValue;
    
    if (type == ::android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = m_values[index].data;
        return u.f;
    }
    return (float)m_values[index].data;
}

uint32_t TypedArray::getColor(int index, uint32_t defValue) const {
    if (!hasValue(index)) return defValue;
    
    uint8_t type = m_values[index].dataType;
    if (type == ::android::Res_value::TYPE_REFERENCE) return defValue;
    if (type < ::android::Res_value::TYPE_FIRST_COLOR_INT || type > ::android::Res_value::TYPE_LAST_COLOR_INT) return defValue;
    
    return m_values[index].data;
}

int TypedArray::getDimensionPixelSize(int index, int defValue) const {
    if (!hasValue(index)) return defValue;
    
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    
    if (type == ::android::Res_value::TYPE_REFERENCE) return defValue;
    
    if (type == ::android::Res_value::TYPE_DIMENSION) {
        return LayoutInflater::parseComplexDimension(data);
    }
    
    if (type >= ::android::Res_value::TYPE_FIRST_INT && type <= ::android::Res_value::TYPE_LAST_INT) {
        return (int)data; // fallback for plain integers
    }
    
    return defValue;
}

int TypedArray::getLayoutDimension(int index, int defValue) const {
    if (!hasValue(index)) return defValue;
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    if (type == ::android::Res_value::TYPE_REFERENCE) return defValue;
    if (type == ::android::Res_value::TYPE_INT_DEC) {
        return (int)data; // MATCH_PARENT or WRAP_CONTENT
    }
    if (type == ::android::Res_value::TYPE_DIMENSION) {
        if (m_resManager) {
            // Re-use complexToDimension logic if we want to be accurate, but resolveDimension takes resId.
            // Oh wait, we already resolved it, so we can't use resolveDimension.
            // But we can just use getDimensionPixelSize.
        }
        return getDimensionPixelSize(index, defValue);
    }
    return defValue;
}

graphics::DrawablePtr TypedArray::getDrawable(int index) const {
    if (!hasValue(index)) return nullptr;

    const uint8_t type = m_values[index].dataType;

    // An inline colour needs no resource lookup, and this is also how
    // ?attr/colorPrimary-style backgrounds arrive once the theme has resolved.
    if (type >= ::android::Res_value::TYPE_FIRST_COLOR_INT &&
        type <= ::android::Res_value::TYPE_LAST_COLOR_INT) {
        return std::make_shared<graphics::ColorDrawable>(m_values[index].data);
    }

    if (index >= (int)m_resIds.size() || m_resIds[index] == 0) return nullptr;
    return DrawableInflater::inflate(m_resManager, m_theme, m_resIds[index]);
}

graphics::ColorStateListPtr TypedArray::getColorStateList(int index) const {
    if (!hasValue(index)) return nullptr;

    const uint8_t type = m_values[index].dataType;

    // Same shape as getDrawable: an inline colour needs no resource lookup, and
    // this is also how a ?attr/textColorPrimary arrives once the theme resolved it.
    if (type >= ::android::Res_value::TYPE_FIRST_COLOR_INT &&
        type <= ::android::Res_value::TYPE_LAST_COLOR_INT) {
        return graphics::ColorStateList::valueOf(m_values[index].data);
    }

    if (index >= (int)m_resIds.size() || m_resIds[index] == 0) return nullptr;
    return ColorStateListInflater::inflate(m_resManager, m_theme, m_resIds[index]);
}

std::string TypedArray::getString(int index) const {
    if (!hasValue(index)) return "";

    if (!m_stringValues[index].empty()) {
        return m_stringValues[index];
    }
    
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    
    if (type == ::android::Res_value::TYPE_REFERENCE) {
        if (m_resManager) return m_resManager->getString(data);
    }
    
    return std::to_string(data);
}

} // namespace setu

