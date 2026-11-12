// The script colours of the PDA's pages in the OpenZone admin window: the window's metal
// (VPP rework 2026-11-12, design 2.3), the same values as the vpp* names of the core's
// tokens (openzone-core\ui\tokens.json, group `color`), which the pages' layout
// descriptions take. OZ_Palette is the PDA's own palette -- the PDA as a device keeps its
// look -- so nothing of the admin pages paints from it. The engine does not read the
// tokens file: change a vpp* token there, change it here too.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

class OZ_PdaVppMetal
{
    // --- grounds
    static const int PANEL  = ARGB(255,  12,  12,  12);   // vppPanel   0.047
    static const int RAISED = ARGB(255,  30,  30,  29);   // vppRaised  0.118 0.118 0.114
    static const int EDGE   = ARGB(255,  60,  60,  58);   // vppEdge    0.235 0.235 0.227

    // --- text
    static const int TEXT   = ARGB(255, 224, 223, 218);   // vppText    0.88  0.875 0.855
    static const int MUTED  = ARGB(255, 160, 159, 153);   // vppMuted   0.627 0.624 0.6

    // --- states
    static const int ACCENT = ARGB(255, 232, 120,  43);   // vppAccent  0.91  0.47  0.17
    static const int ALERT  = ARGB(255, 245, 199,  64);   // vppAlert   0.96  0.78  0.25
}

#endif
#endif
#endif
