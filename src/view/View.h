/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "../graphics/Canvas.h"
#include "../graphics/RenderNode.h"
#include "../graphics/drawable/Drawable.h"
#include "MotionEvent.h"
#include "KeyEvent.h"

// Forward declare Context/Theme so we don't need a heavy include

namespace android { class ResXMLParser; }

namespace setu {
class ResourceManager;
class Theme;
class TypedArray;
namespace view {

class ViewGroup;

class View : public std::enable_shared_from_this<View>,
             public graphics::Drawable::Callback {
public:
    static const int MATCH_PARENT = -1;
    static const int WRAP_CONTENT = -2;
    static const int VISIBLE = 0x00000000;
    static const int INVISIBLE = 0x00000004;
    static const int GONE = 0x00000008;

    class LayoutParams {
    public:
        int width;
        int height;

        // Margins
        int leftMargin = 0;
        int topMargin = 0;
        int rightMargin = 0;
        int bottomMargin = 0;

        LayoutParams(int w, int h) : width(w), height(h) {}
        virtual ~LayoutParams() = default;
    };

    View(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    View();
    virtual ~View();

    int getId() const { return mId; }
    void setId(int id) { mId = id; }

    virtual std::shared_ptr<View> findViewById(int targetId);

    std::shared_ptr<LayoutParams> getLayoutParams() const { return mLayoutParams; }
    void setLayoutParams(std::shared_ptr<LayoutParams> params);
    void requestLayout();

    // Layout dimensions and positioning (relative to parent)
    int getLeft() const { return mLeft; }
    int getTop() const { return mTop; }
    int getRight() const { return mRight; }
    int getBottom() const { return mBottom; }
    int getWidth() const { return mRight - mLeft; }
    int getHeight() const { return mBottom - mTop; }

    void setLeft(int left) { mLeft = left; }
    void setTop(int top) { mTop = top; }
    void setRight(int right) { mRight = right; }
    void setBottom(int bottom) { mBottom = bottom; }

    // Measured dimensions (what the view wants to be)
    int getMeasuredWidth() const { return mMeasuredWidth; }
    int getMeasuredHeight() const { return mMeasuredHeight; }

    // Parent
    ViewGroup* getParent() const { return mParent; }
    void setParent(ViewGroup* parent) { mParent = parent; }

    int getVisibility() const { return mVisibility; }
    void setVisibility(int visibility) { mVisibility = visibility; }

    // Android Measure/Layout/Draw passes
    virtual void measure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void layout(int l, int t, int r, int b);
    virtual void draw(graphics::Canvas& canvas);

    // To be overridden by subclasses
    virtual void onAttachedToWindow() {}
    virtual void onDetachedFromWindow() {}
    virtual void onMeasure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void onLayout(bool changed, int l, int t, int r, int b);
    virtual void onDraw(graphics::Canvas& canvas);
    virtual void onFinishInflate() {}

    virtual void dump(int depth = 0);
    virtual std::string getClassName() const { return "View"; }
    
    std::string getOriginalClassName() const { return mOriginalClassName; }
    void setOriginalClassName(const std::string& name) { mOriginalClassName = name; }

    // Event handling
    virtual bool dispatchTouchEvent(class MotionEvent& event);
    virtual bool onTouchEvent(class MotionEvent& event);

    virtual bool dispatchKeyEvent(const class KeyEvent& event);
    virtual bool onKeyEvent(const class KeyEvent& event);

    bool isFocused() const { return mIsFocused; }
    virtual void setFocus(bool focus);

    // Drawable state. A <selector> background asks its owner "which of these are
    // you?" through getDrawableState(), so every flag below is something a real
    // APK can make visible: a pressed button darkens, a disabled one greys out, a
    // selected list row highlights.
    bool isPressed() const { return mIsPressed; }
    virtual void setPressed(bool pressed);

    bool isEnabled() const { return mEnabled; }
    virtual void setEnabled(bool enabled);

    bool isSelected() const { return mIsSelected; }
    virtual void setSelected(bool selected);

    bool isActivated() const { return mIsActivated; }
    virtual void setActivated(bool activated);

    bool isHovered() const { return mIsHovered; }
    virtual void setHovered(bool hovered);

    // The state set handed to stateful drawables, in StateSet token form. Cached,
    // because a selector asks for it on every state change and building it
    // allocates. Not const: the first call after a flag moves rebuilds it.
    const std::vector<int>& getDrawableState();

    // Rebuilds the state set and pushes it into anything that cares. Call this
    // after changing anything getDrawableState() reports.
    void refreshDrawableState();

    // Tells the background where the finger is, in this view's own coordinates.
    //
    // Order matters and AOSP is explicit about it: a widget calls this *before*
    // setPressed(true), because a ripple starts from wherever the hotspot was when
    // the pressed state arrived. Call it after, and every first touch ripples from
    // the centre of the view instead of from under the finger.
    void drawableHotspotChanged(float x, float y);

    bool isClickable() const { return mClickable; }
    virtual void setClickable(bool clickable) { mClickable = clickable; }

    bool isFocusable() const { return mFocusable; }
    virtual void setFocusable(bool focusable) { mFocusable = focusable; }

    int getGravity() const { return mGravity; }
    virtual void setGravity(int gravity) { mGravity = gravity; requestLayout(); invalidate(); }

    // Background. A View's background is a Drawable, so a <shape>, <selector> or
    // nine-patch from a layout renders instead of being discarded for not being
    // a colour. setBackgroundColor() is a convenience that wraps the value in a
    // ColorDrawable, so both paths draw through the same code.
    virtual void setBackground(std::shared_ptr<graphics::Drawable> background);
    graphics::Drawable* getBackground() const { return mBackground.get(); }
    const std::shared_ptr<graphics::Drawable>& getBackgroundDrawable() const { return mBackground; }
    virtual void setBackgroundColor(uint32_t color);
    // The colour of the background when it is a plain ColorDrawable; 0 otherwise.
    uint32_t getBackgroundColor() const;

    // graphics::Drawable::Callback
    void invalidateDrawable(graphics::Drawable* who) override;
    void scheduleDrawable(graphics::Drawable* who, std::function<void()> what,
                          long long whenMs) override;
    void unscheduleDrawable(graphics::Drawable* who) override;

    void setOnClickListener(std::function<void()> listener) { mOnClickListener = listener; }
    void setOnLongClickListener(std::function<bool()> listener) { mOnLongClickListener = listener; }
    void performLongClick();
    virtual void performClick() { if (mOnClickListener) mOnClickListener(); }

    // MeasureSpec constants
    static const int MEASURE_SPEC_UNSPECIFIED = 0 << 30;
    static const int MEASURE_SPEC_EXACTLY = 1 << 30;
    static const int MEASURE_SPEC_AT_MOST = 2 << 30;
    static const int MEASURE_SPEC_MODE_MASK = 3 << 30;

    static int makeMeasureSpec(int size, int mode) {
        return (size & ~MEASURE_SPEC_MODE_MASK) | (mode & MEASURE_SPEC_MODE_MASK);
    }
    static int getMode(int measureSpec) {
        return (measureSpec & MEASURE_SPEC_MODE_MASK);
    }
    static int getSize(int measureSpec) {
        return (measureSpec & ~MEASURE_SPEC_MODE_MASK);
    }

    static int resolveSize(int size, int measureSpec) {
        int specMode = getMode(measureSpec);
        int specSize = getSize(measureSpec);
        int result = size;
        switch (specMode) {
            case MEASURE_SPEC_AT_MOST:
                result = std::min(size, specSize);
                break;
            case MEASURE_SPEC_EXACTLY:
                result = specSize;
                break;
            case MEASURE_SPEC_UNSPECIFIED:
            default:
                result = size;
        }
        return result;
    }

    void setMeasuredDimension(int measuredWidth, int measuredHeight);

    // RenderNode integration
    graphics::RenderNode* getRenderNode() const { return mRenderNode.get(); }
    void updateRenderNode();

    int getPaddingLeft() const { return mPaddingLeft; }
    int getPaddingTop() const { return mPaddingTop; }
    int getPaddingRight() const { return mPaddingRight; }
    int getPaddingBottom() const { return mPaddingBottom; }

    void setPadding(int left, int top, int right, int bottom);

    int getMinimumWidth() const { return mMinWidth; }
    int getMinimumHeight() const { return mMinHeight; }
    void setMinimumWidth(int minWidth) { mMinWidth = minWidth; requestLayout(); }
    void setMinimumHeight(int minHeight) { mMinHeight = minHeight; requestLayout(); }

    void invalidate();
    void dispatchAttachedToWindow();
    void dispatchDetachedFromWindow();

    float getAlpha() const { return mAlpha; }
    void setAlpha(float alpha) { mAlpha = alpha; invalidate(); }
    float getTranslationX() const { return mTranslationX; }
    void setTranslationX(float x) { mTranslationX = x; invalidate(); }
    float getTranslationY() const { return mTranslationY; }
    void setTranslationY(float y) { mTranslationY = y; invalidate(); }
    float getScaleX() const { return mScaleX; }
    void setScaleX(float x) { mScaleX = x; invalidate(); }
    float getScaleY() const { return mScaleY; }
    void setScaleY(float y) { mScaleY = y; invalidate(); }


    // Installed once by the host so that invalidate() actually reaches the
    // screen. Without it, marking a render node dirty updated the display list
    // and stopped there, which is why widgets used to call InvalidateRect() by
    // hand. The view layer is also built standalone (constraint_layout_test), so
    // this is a hook rather than a direct call into WindowManager.
    static void setInvalidateHandler(std::function<void()> handler);
    static void requestHostRedraw();

    // The animation clock, and the second half of the same bargain.
    //
    // An animating drawable asks its owner to run work at a deadline
    // (Drawable::scheduleSelf). A View has no message queue of its own, so every
    // such request lands in one process-wide queue and the host drains it.
    //
    // runScheduledWork() runs everything now due and returns true while anything
    // is still queued - the host's cue to keep frames coming. The handler installed
    // here fires only on the idle-to-animating edge, so the host arms a frame clock
    // when an animation starts and can stop it again the moment the queue empties.
    // Nothing is charged for a tick while the UI is at rest.
    static void setAnimationHandler(std::function<void()> handler);
    static void postTask(std::function<void()> what);

    // Display metrics live here rather than in WindowManager for the same reason
    // the invalidate handler does: the view layer is built standalone, so
    // ViewGroup cannot reach a symbol that only exists in the full runtime.
    // WindowManager's getDensity()/setDensity() forward to these, so there is
    // still exactly one value. 2.0 (xhdpi) until something actually queries the
    // display - the same constant WindowManager defaulted to before.
    static float getDisplayDensity();
    static float getScaledDensity();
    static void setDisplayMetrics(float density, float scaledDensity);

protected:
    float mAlpha = 1.0f;
    float mTranslationX = 0.0f;
    float mTranslationY = 0.0f;
    float mScaleX = 1.0f;
    float mScaleY = 1.0f;

    // The states this view is in. AOSP passes an extraSpace count so a subclass
    // can size the array up front; a vector makes that pointless, so a subclass
    // just appends to what the base returns - a CheckBox would add
    // STATE_CHECKABLE and STATE_CHECKED here.
    virtual std::vector<int> onCreateDrawableState() const;

    // Called after the state set changes. The default pushes it into the
    // background and repaints only if the background's appearance actually moved.
    virtual void drawableStateChanged();

    // Applies padding without marking it as user-specified, so a later
    // background swap can still contribute its own insets.
    void internalSetPadding(int left, int top, int right, int bottom);

    // Pushes the current size into the background drawable.
    void updateBackgroundBounds();

    std::string mOriginalClassName;

    int mId = 0;
    int mLeft = 0;
    int mTop = 0;
    int mRight = 0;
    int mBottom = 0;

    int mMeasuredWidth = 0;
    int mMeasuredHeight = 0;

    int mPaddingLeft = 0;
    int mPaddingTop = 0;
    int mPaddingRight = 0;
    int mPaddingBottom = 0;

    int mMinWidth = 0;
    int mMinHeight = 0;

    ViewGroup* mParent = nullptr;

    std::unique_ptr<graphics::RenderNode> mRenderNode;
    std::function<void()> mOnClickListener;
    std::function<bool()> mOnLongClickListener;
    uint64_t mPendingCheckForLongPress = 0;
    std::shared_ptr<LayoutParams> mLayoutParams;
    bool mIsFocused = false;
    bool mIsPressed = false;
    // Enabled by default, like every real View. Note that this is the one state
    // flag whose default is true, which is also why a background <selector> has to
    // be told the state the moment it is installed: an empty state set reads as
    // "not enabled" to AOSP's matcher.
    bool mEnabled = true;
    bool mIsSelected = false;
    bool mIsActivated = false;
    bool mIsHovered = false;
    bool mAttachedToWindow = false;
    // Rebuilt lazily by getDrawableState().
    std::vector<int> mDrawableState;
    bool mDrawableStateDirty = true;
    bool mClickable = false;
    bool mFocusable = false;
    int mGravity = 0x33; // Default TOP | LEFT
    std::shared_ptr<graphics::Drawable> mBackground;
    // True once padding has been set explicitly (from XML or code). AOSP gives
    // explicit padding precedence over a background drawable's own insets.
    bool mUserPaddingDefined = false;
    int mVisibility = VISIBLE;
    bool mIsLayoutRequested = false;
    bool mIsRenderNodeDirty = true;

private:
    // One queued animation callback. `who` is kept so the entry can be dropped
    // when its drawable is replaced or its owner destroyed - the lambda captures
    // the drawable raw, and running it afterwards would touch freed memory.
    static std::function<void()> s_animationHandler;
    static std::function<void()> s_invalidateHandler;
    static float s_density;
    static float s_scaledDensity;
};

} // namespace view
} // namespace setu

