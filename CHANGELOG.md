Antares Changelog
=================

Notable changes per version, in the shape of Ares' own "What's New"
documentation. As there, the lists are non-exhaustive: internal changes,
refactoring and optimizations appear only if they are notable. Entries credit
their author, and where a fix came from another project, the upstream pull
request.

Ares grouped its bug fixes into three categories: fixes to genuine bugs in
unmodded Yuri's Revenge 1.001, fixes to vanilla logic that goes unused or does
not behave the way mods expect, and efficiency or crash fixes. Antares adds a
fourth: fixes to bugs in Ares itself, which Ares never listed because it did
not know about them.

Unreleased
----------

### Fixes to Ares bugs

+ **Abduction** no longer leaves occupation bits behind on the terrain when the
  victim was moving, which used to make the cell permanently impassable.
  (Trsdy, [Phobos #1417](https://github.com/Phobos-developers/Phobos/pull/1417))

+ **Amphibious** technos with `MovementZone=AmphibiousCrusher` or
  `AmphibiousDestroyer` can enter water structures again. Only
  `MovementZone=Amphibious` was exempted from the naval mismatch check.
  (CrimRecya, [Phobos #1595](https://github.com/Phobos-developers/Phobos/pull/1595))

+ **UnitDelivery** places buildings facing north, rather than at a facing
  derived from the target cell index. That facing is still used for units.
  (NetsuNegi, [Phobos #2155](https://github.com/Phobos-developers/Phobos/pull/2155))

+ **AltCameo** is shown for buildings with `UndeploysInto` once the matching
  vehicle factory has been infiltrated, the way it already was for the vehicles
  themselves.
  (NetsuNegi, [Phobos #1995](https://github.com/Phobos-developers/Phobos/pull/1995))

+ **Building animations** are no longer put back on a building that has been
  sold, erased or destroyed. Ares lets them expire rather than deleting them,
  so they outlive the building and their death lands in `Detach`, which used to
  restore the Idle and Active anims onto a corpse.
  (Trsdy, [Phobos #1588](https://github.com/Phobos-developers/Phobos/pull/1588))

+ **Cloning facilities** that are also a war factory no longer kick out clones
  past the free-link check, which used to pile them onto the exit cell and jam
  them.
  (NetsuNegi, [Phobos #1967](https://github.com/Phobos-developers/Phobos/pull/1967))

Ares 3.0p1 (21.352.1218)
------------------------

Reconstructed from the shipped Ares 3.0p1 binary onto the Ares 0.A codebase
(internal 16.1.1010), the last version released with source. The goal of this
release is to behave the way shipped 3.0p1 behaves, not to improve on it, so
its bugs were reproduced along with its features.

+ **Everything** documented for Ares 3.0 and 3.0p1 at
  [ares-developers.github.io](https://ares-developers.github.io/Ares-docs/),
  which is the reference for what this tree is supposed to do.
  (ZivDero, from research by Otamaa)
