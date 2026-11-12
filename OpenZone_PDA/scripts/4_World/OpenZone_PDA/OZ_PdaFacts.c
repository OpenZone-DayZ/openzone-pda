// The PDA's card on Ядро › Огляд (the core's OZ_AdminFacts, VPP rework 2026-11-12,
// design 4.3): profiles and the device classes they name, the modules and carriers of
// Hardware.json, the pages the registry holds; the problems the last load found in the
// three files. "Reload every module" reads the three files again as the start does, and
// writes none of them: a fix Validate makes stays in memory.
//
// Registered at the server mission start of OZ_PdaModule, after the files are read.
class OZ_PdaFacts : OZ_AdminFactsProvider
{
    override void Fill(OZ_AdminModuleCard card)
    {
        card.Id = "pda";
        card.Name = "#STR_OZ_PDA_VPP_MOD";

        // Pages in the window come with this repository's VPP glue; a server that loads
        // no VPP has the card without them.
        card.Pages = 0;
#ifdef OpenZone_PDA_VPP
        card.Pages = 1;
#endif

        int found = OZ_ConfigProblems.Of("Tuning");
        found += OZ_ConfigProblems.Of("Hardware");
        found += OZ_ConfigProblems.Of("Profiles");
        card.Problems = found;

        card.Add("#STR_OZ_PDA_FACT_PROFILES", OZ_PdaProfiles.Count().ToString());
        card.Add("#STR_OZ_PDA_FACT_DEVICES", DeviceClasses().ToString());
        card.Add("#STR_OZ_PDA_FACT_MODULES", OZ_PdaHardware.ModuleCount().ToString());
        card.Add("#STR_OZ_PDA_FACT_CARRIERS", OZ_PdaHardware.CarrierCount().ToString());
        card.Add("#STR_OZ_PDA_FACT_PAGES", OZ_PageRegistry.Count().ToString());
    }

    // The device classes the profiles name, each once.
    private int DeviceClasses()
    {
        OZ_PdaProfilesConfig cfg = OZ_PdaProfiles.Get();
        if (!cfg || !cfg.Profiles)
            return 0;

        array<string> seen = new array<string>();
        for (int i = 0; i < cfg.Profiles.Count(); i++)
        {
            OZ_PdaProfile p = cfg.Profiles[i];
            if (!p || !p.ClassNames)
                continue;
            for (int c = 0; c < p.ClassNames.Count(); c++)
            {
                string cls = p.ClassNames[c];
                if (seen.Find(cls) == -1)
                    seen.Insert(cls);
            }
        }
        return seen.Count();
    }

    // As at the start, and then what the window's apply of each file does: the beacon
    // period follows Tuning, and the values the core's sync packet carries (the toast,
    // the route step, the message cap, the slots of every model) reach the players
    // already in the Zone.
    override bool Reload(array<string> said)
    {
        OZ_PdaProfiles.ServerLoad(false);
        OZ_PdaHardware.ServerLoad(false);
        OZ_PdaTuning.ServerLoad(false);

        OZ_PdaModule.RearmBeacons();
        OZ_SyncSender.SendAll("pda configs reloaded");

        string line = "overview: pda configs reloaded, profiles=" + OZ_PdaProfiles.Count().ToString();
        line += " modules=" + OZ_PdaHardware.ModuleCount().ToString();
        OZ_Log.Info(line);
        return true;
    }
}
