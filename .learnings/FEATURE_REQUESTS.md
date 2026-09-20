# Feature Requests

Capabilities requested by the user.

---

## [FEAT-20260717-001] sdk_polygon_corner_resize_and_browse_responsiveness

**Logged**: 2026-07-17T15:00:00+08:00
**Priority**: critical
**Status**: in_progress
**Area**: frontend

### Requested Capability
Keep large local projects responsive during import, sustained keyboard browsing, and application reactivation; persist manual disease mode; and let users resize a selected artificial-mode area disease by dragging four visibly animated corner handles with database write-back.

### User Context
The supplied project contains more than 70,000 road-image files on a mechanical disk. Import, rapid W-key browsing, switching away and back, and long editing sessions currently stall the UI, show black frames, saturate the disk, and progressively slow disease drawing.

### Complexity Estimate
complex

### Suggested Implementation
Profile the current SDK loader and bridge path, bound and prioritize decoded-image work, cancel stale requests, prevent foreground reactivation from restarting broad I/O, persist the selected mode in the existing project settings source, and add a transactional SDK overlay edit state for four-corner geometry updates.

### Metadata
- Frequency: recurring
- Related Features: SDK tiled browsing, disease overlay editing, project settings, database persistence

---

## [FEAT-20260718-001] unified_disease_edit_move_merge_shortcuts

**Logged**: 2026-07-18T00:00:00+08:00
**Priority**: high
**Status**: in_progress
**Area**: frontend

### Requested Capability
Unify disease interaction in edit mode so every disease can open the disease-type editor on left click; allow whole-polygon movement for manual/design area diseases without conflicting with corner resize; repair merge for area diseases in manual, automatic, and design modes; reject unsupported design-line merging with a clear message; bind merge to F4; and correct/extend the shortcut help page.

### User Context
Users need the active interaction to be discoverable and consistent across disease modes. When a left click in a non-add mode hits no disease, the application must tell the user which mode is active so the click is not mistaken for a malfunction.

### Complexity Estimate
complex

### Suggested Implementation
Route SDK hit testing through one edit interaction state machine. Use click-to-edit, corner-handle drag-to-resize, and body drag past a movement threshold to translate a selected area disease, with database commit on release. Centralize merge eligibility and UI feedback, add F4 through the existing action/shortcut registry, and drive the shortcut settings text from the same canonical descriptions.

### Metadata
- Frequency: recurring
- Related Features: SDK disease overlay, edit mode, merge mode, shortcut settings

---
