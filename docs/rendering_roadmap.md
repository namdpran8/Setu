# Rendering Roadmap

Durable record of the rendering/drawable phase plan for Windroid, kept in-repo so
it survives independent of any chat session. Update this file when the plan
changes; do not let it drift.

**Last updated:** 2026-08-25 (Phase 5 complete)

---

## Status at a glance

- **Phase 4 (ColorStateList + colour-only tinting): COMPLETE and committed** —
  landed in `40df498` on 2026-08-24, alongside the drawable-inflation work.
  Compilation against the current tree is still unverified here: the tree is
  mid-refactor with a large outstanding error count being worked through by hand,
  and per standing rule this project is never built by the assistant.
- **Phase 2 V1.5 (OVAL / LINE / RING): COMPLETE and committed** — landed in
  `9d6d40f` on 2026-08-25.
- **Phase 5 (RippleDrawable): COMPLETE, uncompiled, as of 2026-08-25.** Brought
  the runtime's first *animating* drawable, and with it the frame clock. On disk
  and internally consistent; not yet built.
- **`<animated-selector>` was scoped OUT of Phase 5.** It is
  `AnimatedStateListDrawable`, a different class that cross-fades or plays an
  `AnimationDrawable` *between* selector items — not a variant of `<ripple>`.
  `phaseFor()` was retargeted accordingly and it now sits under "Not on this
  roadmap".
- **Gradients and dashed strokes are now the only `<shape>` features left, and
  they are UNSCHEDULED.** They were candidates for V1.5 and were deliberately
  cut; they need placing in the order. See "Deferred, unscheduled" below.
- **`android:backgroundTint` is explicitly OUT of Phase 4** and is not scheduled
  into any later phase here. It was a deliberate scope decision, not an
  oversight. (Note: `android:tint` on a drawable and `android:backgroundTint` on
  a View are different attributes; only the View-level tint is the one ruled
  out.)

---

## Confirmed phase order

```
Phase 4  [DONE, committed]  →  Phase 2 V1.5  [DONE, committed]  →  Phase 5  [DONE, uncompiled]  →  Phase 6  →  Item 9
```

This ordering is confirmed. What follows is the detail for each entry, marked by
how well-grounded it is: **[confirmed]** decisions, **[from code]** scope derived
from TODO/comment anchors in the tree, and **[reconfirm]** items whose detailed
scope lives only in chat history and should be re-agreed before work starts.

---

## Phase 4 — ColorStateList + colour-only tinting  **[DONE, committed]**

The `android.content.res.ColorStateList` value type and the plumbing that lets a
colour follow a view's drawable state.

Delivered:

- `graphics::ColorStateList` value type — `src/graphics/ColorStateList.{h,cpp}`.
  Deliberately **not** a `Drawable` and **not** under `graphics/drawable/`.
- Resource inflation — `src/ui/ColorStateListInflater.{h,cpp}` (a
  `res/color/*.xml` `<selector>`) and the shared state-set parser
  `src/ui/StateSetInflater.{h,cpp}` (moved out of `DrawableInflater`).
- Attribute access — `XmlAttrs::getColorStateList` and `TypedArray::getColorStateList`.
- Consumers wired: `<solid>` and `<stroke>` colour selectors in
  `GradientDrawable`, and `android:textColor` in `TextView`.
- `mTextColorSetFromXml` guard in `EditText` so its default text colour cannot
  wipe an XML-authored colour selector (follows the existing
  `mGravitySetFromXml` precedent).
- All three new `.cpp`s registered in `setu_runtime` only (the
  `constraint_layout_test` target compiles none of the units that include
  `ColorStateList.h`).

Remaining for this phase: **compile.** That is the maintainer's to do; per
standing rule this project is never built by the assistant.

---

## Phase 2 V1.5 — finish the `<shape>` / GradientDrawable surface  **[DONE, committed]**

Phase 2 built the background-drawable pipeline: `GradientDrawable` paints a
`<shape>` and `DrawableInflater` reads one out of an APK (see the header note in
`src/ui/DrawableInflater.h`, which calls itself "the other half of Phase 2").
V1.5 completed the shape geometry.

**Scope was decided as: the shapes that need nothing new below
`GradientDrawable`.** `Path::addOval`, `Path::FillType::EVEN_ODD`,
`Canvas::drawPath` and `Canvas::drawLine` already existed and were unused for
this purpose, and `Direct2DCanvas::drawPath` already mapped `EVEN_ODD` to
`D2D1_FILL_MODE_ALTERNATE`. So OVAL/LINE/RING cost no Canvas, Paint, Path or
backend changes. Gradients and dashed strokes were cut because both require
extending `Paint` *and* the Direct2D backend *and* `RecordingCanvas` — a
subsystem, not a point-five.

Delivered:

- **OVAL** — built via `Path::addOval` into the existing cached `mPath`.
- **LINE** — stroke only, across the vertical centre, per AOSP. A line shape
  carrying only a `<solid>` and no `<stroke>` draws nothing, on a real device
  too.
- **RING** — `buildRingPath()`, following AOSP's `buildRing` for the full-sweep
  case.
- **Ring geometry** — `android:innerRadius` / `innerRadiusRatio` / `thickness` /
  `thicknessRatio`, with AOSP's `-1` sentinel and its
  `DEFAULT_INNER_RADIUS_RATIO` (3.0) / `DEFAULT_THICKNESS_RATIO` (9.0), now
  named constants on `GradientDrawable`.

Three AOSP details verified against `scratch/GradientDrawable.java` rather than
recalled, because all three are easy to get wrong:

1. Full-sweep rings use `addOval(CW)` + `addOval(CCW)` under the **default
   WINDING** rule; the `EVEN_ODD` branch is only for partial sweeps. Our
   `Path::addOval` takes no direction and always winds clockwise, so we use
   `EVEN_ODD` with two same-wound ovals — identical result for two nested
   non-intersecting ovals, and exactly what `Path.h`'s own comment prescribes.
2. Ring `thickness` and inner `radius` **both divide `bounds.width()`**, even
   though one drives a vertical inset. That is AOSP's arithmetic; it is what
   makes a ring in a non-square view match.
3. The `*Ratio` attributes are read **only when** the matching absolute
   dimension is absent.

Known fidelity quirk, matching AOSP exactly: `draw()` bails on an empty rect,
and `ensureValidRect` insets by half the stroke width. So a LINE in a view
exactly as tall as its own stroke collapses to zero height and draws nothing.
Real Android does the same — which is why real dividers use a rectangle+solid or
give the view more height. Not a bug to fix here.

Still unread on `<shape>`: `android:tint` and `android:useLevel` (the latter
would make a ring a partial arc driven by `setLevel()`).

---

## Deferred, unscheduled — gradients and dashed strokes  **[needs placing]**

Cut from V1.5. Both need `Paint` extended and the Direct2D backend taught a new
trick, so each is a real slice of work rather than a shape case:

- **Real gradients.** `<gradient>` is currently approximated by averaging its
  stops into one flat colour (`DrawableInflater::inflateShape`, `meanColor` /
  `meanColor3`). Needs a shader concept on `Paint`, D2D linear/radial/sweep
  gradient brushes, and `RecordingCanvas` support. Deleting the averaging
  branch is the marker for this landing.
- **Dashed strokes.** `dashWidth` / `dashGap` are parsed and stored but draw
  solid (`GradientDrawable.h` members). Needs dash support on `Paint` and a D2D
  stroke style.

These have **no agreed position in the phase order.** Decide where they go
before starting either.

---

## Phase 5 — RippleDrawable  **[DONE, uncompiled]**

`<ripple>` support: the touch feedback every Material widget is built on. Before
this, a `<ripple>` background inflated to nothing at all, which is why a stock
AppCompat button rendered as a hole.

This is the first drawable in the runtime that **animates**, so most of the phase
was infrastructure that did not exist: no delayed post, no frame source, no
monotonic clock, no interpolators, and `setHotspot` was declared but never called.

### Delivered — infrastructure

- **`src/utils/SystemClock.h`** — `uptimeMillis()`. Header-only, because
  `View.cpp` reaches for it and compiles into both `setu_runtime` *and*
  `constraint_layout_test`.
- **`src/utils/Interpolator.h`** — `linear`, `lerp`, and `fastOutSlowIn` (AOSP's
  `PathInterpolator(0.4, 0, 0.2, 1)`). Header-only for the same reason.
- **The frame clock**, in three parts:
  - `Drawable::scheduleSelf` / `unscheduleSelf` — a drawable asks its owner to run
    work at a deadline. `whenMs` is an **absolute `uptimeMillis()` deadline, not a
    delay**, matching AOSP's `Handler.postAtTime`.
  - `View::scheduleDrawable` / `unscheduleDrawable` / `runScheduledWork` /
    `setAnimationHandler` — one process-wide queue, drained by the host. The
    handler fires only on the **idle-to-animating edge**, so nothing is charged
    for a tick while the UI is at rest.
  - `WindowManager` — `SetTimer` ID 2 (`TIMER_ANIMATION`, 16ms), armed on that
    edge and killed the moment `runScheduledWork()` reports the queue empty. (ID 1
    was already taken by the idle "ghost touch" easter egg.)
- **Hotspot plumbing** — `View::drawableHotspotChanged`, called from
  `Button.cpp`'s DOWN and MOVE handling. Order is load-bearing and AOSP is
  explicit about it: the hotspot must be reported **before** `setPressed(true)`,
  or every first touch ripples from the centre of the view.

Two invariants worth not breaking:

1. **Queue non-empty ⟺ timer running.** `runScheduledWork` moves everything due
   *out* of the queue before running any of it, so a self-rescheduling callback
   repopulates the queue during the run and the return value is still correct. On
   the last frame nothing reschedules, it returns `false`, and `WM_TIMER` calls
   `KillTimer`.
2. **`WM_TIMER` deliberately does not `InvalidateRect`.** Callbacks invalidate for
   themselves, via `invalidateSelf()` → `View::invalidateDrawable` →
   `invalidate()` → `requestHostRedraw()`. A callback that changes nothing costs
   no repaint.

### Delivered — the drawable

- **`src/graphics/drawable/RippleDrawable.{h,cpp}`** — registered in
  `setu_runtime` only (it includes `ColorStateList.h`, which
  `constraint_layout_test` does not compile).
- **`<ripple>` inflation** — `DrawableInflater::inflateRipple` +
  `inflateRippleItem`, with `@android:id/mask` hard-coded as `0x0102002e`. It is a
  *public* framework ID and therefore frozen for good, and hard-coding keeps it
  working with no framework-res loaded — which is exactly when a name lookup would
  fail.

**The animation model is timestamp-driven, not animator-driven.** Every value is
a pure function of `uptimeMillis()`, so a ripple is fully described by
`(startX, startY, enterMs, exitMs)` — four numbers replacing AOSP's four
`ObjectAnimator`s. A late or dropped frame changes nothing, and there is no
per-frame mutable state to fall out of step with the clock. `scheduleSelf` is a
frame *pump*, not a carrier of animation state.

Two exact algebraic simplifications of AOSP, both re-derived against
`scratch/RippleForeground.java` rather than recalled:

- `fadeStart = exitMs + max(0, 225 - (exitMs - enterMs))` reduces **exactly** to
  `max(exitMs, enterMs + OPACITY_HOLD_DURATION)`.
- AOSP's `canvas.translate(cx, cy)` plus `lerp(startX - centreX, 0, tween)`
  collapses **exactly** to `lerp(startX, centreX, tween)` — no canvas translate,
  saving two display-list commands per frame. (`mTargetX`/`mTargetY` were verified
  to be 0.)

### Four departures from AOSP, each forced from below

Each is documented in the class header as well as here.

1. **No animators** — see above.
2. **No `LayerDrawable` underneath.** AOSP's `RippleDrawable` extends it to hold
   an arbitrary layer stack. Drawable containers are not on the roadmap yet, so
   this holds one content layer and one mask layer, which is what essentially
   every real `<ripple>` resource actually contains. A `<ripple>` with extra
   content layers logs and keeps the first.
3. **No mask shader.** AOSP builds an `ALPHA_8` bitmap from the mask or content
   and draws the ripple through a `BitmapShader` + `PorterDuffColorFilter`.
   `Paint` has no alpha, no shader and no colour filter, so this takes AOSP's
   *other* branch — **`MASK_NONE`**, where `clipRect` to the bounds does the
   containing. The mask child is still parsed and held, because it decides
   `isBounded()`.
4. **Solid style only.** `STYLE_PATTERNED` (Android 12's "sparkle") needs a
   `RuntimeShader`. This is the `STYLE_SOLID` path every version before it used
   and every version since still falls back to. `android:effectColor` is parsed
   and logged as ignored for the same reason.

### Fidelity notes

- **Corners square off.** A consequence of `MASK_NONE`: a ripple on a rounded
  button fills the corners the button itself leaves empty. Wrong in a pixel diff,
  but *bounded*, which the alternative is not.
- **A circle costs nothing new.** `drawRoundRect` with a corner radius exactly
  half the side length *is* a circle — the four arcs meet tangentially with
  nothing straight left between them — so no `Canvas::drawCircle` had to be added
  across 6 files, and D2D still renders it with native arcs rather than a Bézier
  approximation. Same trick as Phase 2 V1.5's "nothing new below".
- **Hover is dead code today.** `setHovered` and `WM_MOUSEMOVE` are unwired, so
  the hover half of the background glow cannot fire. The focus half is live via
  `View::setFocus`.

### Two deliberate behavioural divergences

Both found by reading AOSP, judged, and documented at the site rather than
silently reproduced or silently changed:

- **The fade hold at `timeSinceEnter == 0`.** AOSP guards its subtraction with
  `timeSinceEnter > 0`, so a press and release inside the same millisecond gets no
  hold at all and flashes nothing. Holding it is what `OPACITY_HOLD_DURATION` is
  *for*, so this does not reproduce that.
- **`setHotspot` records unconditionally.** AOSP skips recording when both a
  ripple and a background exist. That guard is nearly always true, and where it is
  not, a stale pending point makes the *next* press ripple from the centre instead
  of from the finger.

### One divergence in the *failure* path

A `<ripple>` with no `android:color` makes AOSP throw
`XmlPullParserException`, which aborts inflation and leaves the widget with no
background at all. This logs and returns **the content layer alone**: the widget
looks right at rest and simply does not respond to touch.

Remaining for this phase: **compile.**

---

## Phase 6 — bitmap pipeline  **[from code]**

Decoding and drawing raster drawables: `<bitmap>`, `<nine-patch>`, and the raw
image formats (`.png`, `.9.png`, `.webp`, `.jpg`). Needs a decoder and
`Canvas::drawBitmap`. Confirmed by `DrawableInflater.cpp`: `phaseFor()` maps
`bitmap`/`nine-patch` to "Phase 6 (bitmap pipeline)", and `inflate()` logs
"Skipping bitmap drawable (Phase 6)" for non-`.xml` drawable paths.

Retires the hand-built flat-colour selector standing in for `btn_default`'s
nine-patch assets in `Button.cpp` (`fade()` / `darken()` / `makeDefaultBackground`).

---

## Item 9  **[reconfirm]**

The terminal roadmap entry in the confirmed order. Its detailed scope is **not**
recorded outside chat history and should be re-established before it comes up.
Do not infer its contents from the Phase 4 build-order (which also happened to
have nine items — a separate, now-completed list).

---

## Not on this roadmap

Recognised by the inflater but deliberately unscheduled here (logged, not
dropped, so a missing background in a real app names its status):

- Vector drawables (`<vector>`, `<animated-vector>`).
- Drawable containers (`<layer-list>`, `<level-list>`, `<transition>`).
- `AnimatedStateListDrawable` (`<animated-selector>`) — scoped out of Phase 5;
  see the status note above.
- `android:backgroundTint` (View-level tint) — see the status note above.

---

## Where the ground truth lives

When this doc and the code disagree, the code wins — then fix this doc. The
load-bearing anchors:

- `src/ui/DrawableInflater.cpp` — `phaseFor()` maps unsupported root elements to
  the phase that will fix them. The single best index of what belongs where.
- `src/ui/DrawableInflater.h` — header note framing the inflater as "the other
  half of Phase 2".
- `src/graphics/drawable/GradientDrawable.{h,cpp}` — inline notes on every
  deferred `<shape>` feature.
- `src/graphics/drawable/RippleDrawable.h` — the four Phase 5 departures from
  AOSP, stated in the class header with the reason each was forced.
- `src/view/View.h` — the frame-clock contract
  (`setAnimationHandler` / `runScheduledWork` / `hasScheduledWork`), including why
  it is a static hook rather than a call into `WindowManager`.
