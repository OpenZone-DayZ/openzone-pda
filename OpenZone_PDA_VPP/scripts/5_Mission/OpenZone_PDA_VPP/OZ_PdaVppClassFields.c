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
        // A device the PDA counts as hardware, and a car that carries one.
        pick.Bind("HwClass", OZ_ClassIndex.ITEM);
        pick.Bind("HwCarClass", OZ_ClassIndex.VEHICLE);
    }
}

#endif
#endif
#endif
