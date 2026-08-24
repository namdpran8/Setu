# Rendering Roadmap

Durable record of the rendering/drawable phase plan for Windroid, kept in-repo so
it survives independent of any chat session. Update this file when the plan
changes; do not let it drift.

**Last updated:** 2026-08-25

---

## Status at a glance

- **Phase 4 (ColorStateList + colour-only tinting): COMPLETE and committed** —
  landed in `40df498` on 2026-08-24, alongside the drawable-inflation work.
  Compilation against the current tree is still unverified here: the tree is
  mid-refactor with a large outstanding error count being worked through by hand,
  and per standing rule this project is never built by the assistant.
- **Phase 2 V1.5 (OVAL / LINE / RING): COMPLETE, uncompiled, as of 2026-08-25.**
  On disk and internally consistent; not yet built.
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
Phase 4  [DONE, committed]  →  Phase 2 V1.5  [DONE, uncompiled]  →  Phase 5  →  Phase 6  →  Item 9
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

## Phase 2 V1.5 — finish the `<shape>` / GradientDrawable surface  **[DONE, uncompiled]**

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

## Phase 5 — RippleDrawable  **[from code]**

`<ripple>` and `<animated-selector>` support. Confirmed by
`DrawableInflater.cpp` `phaseFor()`: these root elements are recognised and
logged as "Phase 5 (RippleDrawable)" rather than silently dropped.

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
