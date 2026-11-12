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
    static const int PANEL  = 0xFF0C0C0C;   // vppPanel   0.047
    static const int RAISED = 0xFF1E1E1D;   // vppRaised  0.118 0.118 0.114
    static const int EDGE   = 0xFF3C3C3A;   // vppEdge    0.235 0.235 0.227

    // --- text
    static const int TEXT   = 0xFFE0DFDA;   // vppText    0.88  0.875 0.855
    static const int MUTED  = 0xFFA09F99;   // vppMuted   0.627 0.624 0.6

    // --- states
    static const int ACCENT = 0xFFE8782B;   // vppAccent  0.91  0.47  0.17
    static const int ALERT  = 0xFFF5C740;   // vppAlert   0.96  0.78  0.25
}

#endif
#endif
#endif
