#include "View.h"
#include "ViewGroup.h"
#include <algorithm>
#include "../graphics/RecordingCanvas.h"
#include "../graphics/drawable/ColorDrawable.h"
#include "../graphics/drawable/StateSet.h"
#include "MotionEvent.h"
#include "../utils/Logger.h"

namespace setu {
namespace view {

std::function<void()> View::s_invalidateHandler;

// xhdpi. Nothing queries the real display yet, so this is the value every dp in
// every layout is scaled by; WindowManager::getDensity() reads it back out.
float View::s_density = 2.0f;
float View::s_scaledDensity = 2.0f;

void View::setInvalidateHandler(std::function<void()> handler) {
    s_invalidateHandler = std::move(handler);
}

float View::getDisplayDensity() { return s_density; }

float View::getScaledDensity() { return s_scaledDensity; }

void View::setDisplayMetrics(float density, float scaledDensity) {
    if (density > 0.0f) s_density = density;
    if (scaledDensity > 0.0f) s_scaledDensity = scaledDensity;
}

void View::requestHostRedraw() {
    if (s_invalidateHandler) {
        s_invalidateHandler();
    }
}

View::View(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes) {
    // In a real implementation, we would extract View's styleables here (e.g. layout_width, layout_height, visibility, id)
    // using TypedArray ta(resManager, { R::styleable::View_id, R::styleable::View_visibility, ... });
    // ta.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);
    // setId(ta.getInt(0, 0));
}

View::View() {}

std::shared_ptr<setu::view::View> View::findViewById(int targetId) {
    if (mId == targetId) return shared_from_this();
    return nullptr;
}

void View::measure(int widthMeasureSpec, int heightMeasureSpec) {
    onMeasure(widthMeasureSpec, heightMeasureSpec);
}

void View::layout(int l, int t, int r, int b) {
    bool changed = (mLeft != l) || (mTop != t) || (mRight != r) || (mBottom != b);
    if (changed) {
        mLeft = l;
        mTop = t;
        mRight = r;
        mBottom = b;
        // A drawable never asks its owner how big it is, so the new size has to
        // be pushed in. Miss this and the background keeps painting at whatever
        // size the view had on the previous layout pass.
        updateBackgroundBounds();
    }
    onLayout(changed, l, t, r, b);
}

void View::draw(graphics::Canvas& canvas) {
    if (mVisibility != VISIBLE) return;

    canvas.save();
    canvas.translate((float)mLeft, (float)mTop);

    canvas.clipRect(0.0f, 0.0f, (float)getWidth(), (float)getHeight());

    if (mBackground) {
        mBackground->draw(canvas);
    }

    onDraw(canvas);

    canvas.restore();
}

void View::setBackground(std::shared_ptr<graphics::Drawable> background) {
    if (mBackground == background) return;

    if (mBackground) {
        mBackground->setCallback(nullptr);
        mBackground->setVisible(false, false);
    }

    mBackground = std::move(background);

    if (mBackground) {
        mBackground->setCallback(this);
        mBackground->setVisible(mVisibility == VISIBLE, false);
        updateBackgroundBounds();

        // A stateful background has to be told the state immediately, not on the
        // first touch. AOSP's matcher treats an empty state set as "none of these
        // are set", so a stock selector - which almost always leads with a
        // state_enabled="false" item - would otherwise render every widget in its
        // disabled appearance until something happened to it.
        if (mBackground->isStateful()) {
            mBackground->setState(getDrawableState());
        }

        // A <shape> with <padding>, or a nine-patch's content box, defines the
        // view's insets. Explicit padding from the layout still wins - AOSP
        // re-applies the XML padding after the background for exactly this
        // reason, and mUserPaddingDefined gets us the same result without
        // depending on the order attributes happen to be visited in.
        //
        // A background with no padding of its own leaves the current padding
        // alone, as AOSP does. Zeroing it here instead would wipe out the
        // built-in chrome insets a widget like Button sets for itself.
        if (!mUserPaddingDefined) {
            graphics::Rect padding;
            if (mBackground->getPadding(padding)) {
                internalSetPadding(padding.left, padding.top, padding.right, padding.bottom);
            }
        }
    }

    requestLayout();
    invalidate();
}

void View::setBackgroundColor(uint32_t color) {
    // Reuse the existing ColorDrawable when there is one: swapping the colour in
    // place keeps the drawable's identity (and its callback) stable, and avoids
    // re-running the padding logic on every press-state colour change.
    if (mBackground) {
        auto* existing = dynamic_cast<graphics::ColorDrawable*>(mBackground.get());
        if (existing) {
            existing->setColor(color);
            return;
        }
    }
    setBackground(std::make_shared<graphics::ColorDrawable>(color));
}

uint32_t View::getBackgroundColor() const {
    if (!mBackground) return 0;
    auto* color = dynamic_cast<graphics::ColorDrawable*>(mBackground.get());
    return color ? color->getColor() : 0;
}

void View::invalidateDrawable(graphics::Drawable* who) {
    if (who == mBackground.get()) {
        invalidate();
    }
}

const std::vector<int>& View::getDrawableState() {
    if (mDrawableStateDirty) {
        mDrawableState = onCreateDrawableState();
        mDrawableStateDirty = false;
    }
    return mDrawableState;
}

std::vector<int> View::onCreateDrawableState() const {
    using namespace graphics::StateSet;

    std::vector<int> state;
    state.reserve(6);

    // Windroid draws one app window and it is the one the user is looking at, so
    // state_window_focused is always set. A real device clears it when a dialog or
    // the notification shade takes focus, and plenty of older selectors carry a
    // separate unfocused-window item; when WM_ACTIVATE is wired through, this is
    // the line that changes.
    state.push_back(STATE_WINDOW_FOCUSED);

    // The order below is the order AOSP reports these in. Matching does not care -
    // StateSet::matches searches - but a logged state set then reads the same way
    // it does in a real hierarchy dump.
    if (mIsSelected) state.push_back(STATE_SELECTED);
    if (mIsFocused) state.push_back(STATE_FOCUSED);
    if (mEnabled) state.push_back(STATE_ENABLED);
    if (mIsPressed) state.push_back(STATE_PRESSED);
    if (mIsActivated) state.push_back(STATE_ACTIVATED);
    // Direct2D means every view really is hardware-accelerated, and a real device
    // sets this too. Almost nothing branches on it, but reporting it costs nothing
    // and the alternative is silently disagreeing with the device.
    state.push_back(STATE_ACCELERATED);
    if (mIsHovered) state.push_back(STATE_HOVERED);
    // Not reported: the two drag states, which need a drag-and-drop pipeline
    // before they could ever be true.

    return state;
}

void View::refreshDrawableState() {
    mDrawableStateDirty = true;
    drawableStateChanged();
    // AOSP also notifies the parent, so a ViewGroup with addStatesFromChildren can
    // fold a child's state into its own. Nothing sets that flag here yet, so there
    // is nobody to tell.
}

void View::drawableStateChanged() {
    const std::vector<int>& state = getDrawableState();
    bool changed = false;

    if (mBackground && mBackground->isStateful()) {
        changed |= mBackground->setState(state);
    }
    // A foreground drawable and the default focus highlight are also pushed here
    // on a real device. Neither exists yet.

    if (changed) {
        invalidate();
    }
}

void View::setFocus(bool focus) {
    if (focus == mIsFocused) return;
    mIsFocused = focus;
    refreshDrawableState();
}

void View::setPressed(bool pressed) {
    if (pressed == mIsPressed) return;
    mIsPressed = pressed;
    refreshDrawableState();
    // AOSP also dispatches down to children so a duplicateParentState child
    // presses with its parent. Nothing reads that flag yet.
}

void View::setEnabled(bool enabled) {
    if (enabled == mEnabled) return;
    mEnabled = enabled;
    refreshDrawableState();
    // Repaint regardless of what the background said: a disabled view is expected
    // to look different even when its background has no disabled item, because the
    // text greys out. That fade is Phase 4's ColorStateList work.
    invalidate();
}

void View::setSelected(bool selected) {
    if (selected == mIsSelected) return;
    mIsSelected = selected;
    refreshDrawableState();
    invalidate();
}

void View::setActivated(bool activated) {
    if (activated == mIsActivated) return;
    mIsActivated = activated;
    refreshDrawableState();
    invalidate();
}

void View::setHovered(bool hovered) {
    if (hovered == mIsHovered) return;
    mIsHovered = hovered;
    refreshDrawableState();
}

void View::updateBackgroundBounds() {
    if (!mBackground) return;
    // draw() translates by mLeft/mTop before painting, so the background lives in
    // the view's own coordinate space.
    mBackground->setBounds(0, 0, getWidth(), getHeight());
}

void View::setPadding(int left, int top, int right, int bottom) {
    mUserPaddingDefined = true;
    internalSetPadding(left, top, right, bottom);
}

void View::internalSetPadding(int left, int top, int right, int bottom) {
    if (mPaddingLeft == left && mPaddingTop == top &&
        mPaddingRight == right && mPaddingBottom == bottom) {
        return;
    }
    mPaddingLeft = left;
    mPaddingTop = top;
    mPaddingRight = right;
    mPaddingBottom = bottom;
    requestLayout();
    invalidate();
}

void View::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // Default implementation: just take the provided size if exactly/at most, 
    // or a default minimum size if unspecified.
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);
    
    int measuredWidth = (widthMode == MEASURE_SPEC_UNSPECIFIED) ? 0 : widthSize;
    int measuredHeight = (heightMode == MEASURE_SPEC_UNSPECIFIED) ? 0 : heightSize;
    
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void View::onLayout(bool changed, int l, int t, int r, int b) {
    // Default implementation does nothing
}

void View::onDraw(graphics::Canvas& canvas) {
    // Default implementation does nothing
}

void View::setMeasuredDimension(int measuredWidth, int measuredHeight) {
    mMeasuredWidth = measuredWidth;
    mMeasuredHeight = measuredHeight;
}

void View::setLayoutParams(std::shared_ptr<LayoutParams> params) {
    mLayoutParams = params;
    requestLayout();
}

void View::requestLayout() {
    mIsLayoutRequested = true;
    mIsRenderNodeDirty = true;
    if (mParent) {
        mParent->requestLayout();
        return;
    }
    requestHostRedraw();
}

void View::invalidate() {
    mIsRenderNodeDirty = true;
    if (mParent) {
        mParent->invalidate();
        return;
    }
    // Reached the root. Marking render nodes dirty only rebuilds display lists on
    // the next frame - something still has to ask for that frame, or the change
    // never appears on screen.
    requestHostRedraw();
}

bool View::dispatchTouchEvent(MotionEvent& event) {
    return onTouchEvent(event);
}

bool View::onTouchEvent(MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        if (mOnClickListener) {
            performClick();
            return true;
        }
    }
    return false;
}

bool View::dispatchKeyEvent(const KeyEvent& event) {
    return onKeyEvent(event);
}

bool View::onKeyEvent(const KeyEvent& event) {
    return false;
}

void View::updateRenderNode() {
    if (!mIsRenderNodeDirty && mRenderNode) {
        return; // Cache hit
    }

    if (!mRenderNode) {
        mRenderNode = std::make_unique<graphics::RenderNode>();
    }
    mRenderNode->clear();

    graphics::RecordingCanvas canvas(mRenderNode.get());
    draw(canvas);
    
    mIsRenderNodeDirty = false;
}


void View::dump(int depth) {
    std::string indent(depth * 2, ' ');
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s[%s id=%d] bounds=(%d,%d)-(%d,%d) w=%d h=%d",
             indent.c_str(),
             getClassName().c_str(),
             mId,
             mLeft, mTop, mRight, mBottom,
             mRight - mLeft, mBottom - mTop);
    Logger::i("ViewDump", std::string(buffer));
}

} // namespace view
} // namespace setu



