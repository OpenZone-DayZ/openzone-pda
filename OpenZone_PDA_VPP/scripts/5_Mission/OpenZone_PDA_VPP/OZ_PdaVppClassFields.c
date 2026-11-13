// The PDA hardware page's class fields in the window's live class search (the core's
// OZ_ClassPick, design docs/specs/2026-11-13-openzone-vpp-class-search-design.md in the
// series' docs).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    override void ClassFields(OZ_ClassPick pick)
    {
        super.ClassFields(pick);
        // A module that goes into a PDA's bay, and a carrier of records (a floppy, a chip):
        // both are things a player carries.
        pick.Bind("HwClass", OZ_ClassIndex.ITEM);
        pick.Bind("HwCarClass", OZ_ClassIndex.ITEM);
    }
}

#endif
#endif
#endif
