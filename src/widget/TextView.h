#pragma once
#include "../view/View.h"
#include "../graphics/Paint.h"
#include "../graphics/TextLayout.h"
#include "../graphics/ColorStateList.h"
#include <memory>
#include <string>

namespace setu {
namespace widget {

class TextView : public view::View {
public:
    TextView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    TextView();
    virtual ~TextView() = default;

    void setText(const std::wstring& text);
    const std::wstring& getText() const { return mText; }

    void setTextColor(uint32_t color);
    uint32_t getTextColor() const { return mTextPaint.getColor(); }

    // android:textColor="@color/some_selector". The colour follows the view's
    // state, which is how a real disabled TextView greys out - the stock
    // framework text colour is a selector with a disabled entry, not a constant.
    // Passing null goes back to a flat colour.
    void setTextColor(const graphics::ColorStateListPtr& csl);
    const graphics::ColorStateListPtr& getTextColors() const { return mTextColorCsl; }

    void setTextSize(float size);
    float getTextSize() const { return mTextPaint.getTextSize(); }

    // The line-broken layout this view last measured itself against. Null until
    // the first measure pass.
    const graphics::TextLayout* getLayout() const { return mLayout.get(); }

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onDraw(graphics::Canvas& canvas) override;

    std::string getClassName() const override { return "TextView"; }

    // AOSP's default TextView text size is 14sp. Resolved against the current
    // scaled density, because everything downstream of Paint is in pixels.
    static float getDefaultTextSizePx();

protected:
    // Returns a layout for the current text/paint wrapped at `maxWidth`, reusing
    // the cached one when nothing that affects line breaking has changed.
    std::shared_ptr<graphics::TextLayout> obtainLayout(float maxWidth);

    // Re-resolves the text colour when the view's state changes.
    void drawableStateChanged() override;

    // Pushes the colour selector's answer for the current state into mTextPaint.
    // A no-op when the colour is a flat one.
    void updateTextColors();

    std::wstring mText;
    graphics::Paint mTextPaint;

    // The authored colour selector, when android:textColor named one. mTextPaint
    // stays the single source of truth for what actually paints - onDraw hands it
    // straight to the canvas - so this only ever feeds into it through
    // updateTextColors(). Keeping a separate "current colour" member would let the
    // two drift, and the one the canvas reads would win silently.
    graphics::ColorStateListPtr mTextColorCsl;

    int mEms = -1;

    // True when android:gravity was present in the layout. Subclasses with a
    // different default gravity (Button centres its label) must not clobber an
    // explicit value from XML, and the base constructor has already run by the
    // time they get a chance to set theirs.
    bool mGravitySetFromXml = false;

    // The same problem for android:textColor. EditText applies a darker default,
    // and it does so after this constructor has already resolved the authored
    // colour. Without this flag that default wins - which now means discarding a
    // whole colour selector, not just one colour.
    bool mTextColorSetFromXml = false;

    // Produced in onMeasure, consumed in onDraw. Sharing one layout between the
    // two passes is the whole point: it is why a view that measured itself two
    // lines tall now actually draws two lines.
    std::shared_ptr<graphics::TextLayout> mLayout;
};

} // namespace widget
} // namespace setu
