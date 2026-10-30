// OpenZone PDA -- a S.T.A.L.K.E.R.-style handheld device.
//
// Depends HARD on OpenZone_Core. That dependency is fatal by design: a mod
// declaring it and loading without the core gets a blocking "Addon X requires
// addon Y" dialog before the game starts, not a silently skipped pbo.
//
// Three kinds of socket, not one list:
//
//   POWER    a battery. Vanilla slot; the engine plugs the device into it.
//   STORAGE  a data carrier. Its own slot: that is content, not capability.
//   MODULES  GPS (with the transponder), the spy module, the decryptor, and
//            whatever other mods bring. They share a LIMITED number of bays.
//
// The bay count is the tier lever. A rookie PDA has one and its owner chooses
// which single board to carry; a Duty PDA has three and carries all of it. A
// device with a slot per device type would offer no choice at all. (The
// radiometer and the dosimeter were removed by the owner on 2026-09-09.)
//
// What fits a bay, and what it does, is a table of classnames in JSON -- so a
// module can come from any mod, and an admin can point the PDA at an item we
// have never heard of.

class CfgSlots
{
    // The class is Slot_<name>; the bare `name` is what attachments[] and
    // inventorySlot[] match on.
    //
    // No ghostIcon on any of these, deliberately. The names this file used to
    // carry -- `memorycard` and `cable` -- do not exist: a scan of every
    // ghostIcon string in the shipped game yields only `cookingequipment`,
    // `cat_vehicle_*`, `cat_fp_*`, `cat_bb_*`, `cat_common_cargo` and their
    // siblings, and nothing among them means "chip" or "module". A name the
    // imageset does not carry draws nothing, so the old lines bought no icon
    // and cost a reader the belief that one was configured. Our own icons
    // arrive with the rest of the art from dayz-3d; until then the bays are
    // labelled by displayName alone.
    class Slot_OZ_DataCarrier
    {
        name = "OZ_DataCarrier";
        displayName = "$STR_OZ_SLOT_CARRIER";
    };

    // The wear slot on the CHARACTER: the device is active in hands or
    // here, and a PDA buried in a backpack stays silent.
    class Slot_OZ_PdaWear
    {
        name = "OZ_PdaWear";
        displayName = "$STR_OZ_SLOT_PDA";
        // Без привида слот у ванільній сітці інвентаря стоїть сліпим
        // квадратом, який ніхто не впізнає. Рація -- найближчий силует.
        ghostIcon = "set:dayz_inventory image:walkietalkie";
    };

    class Slot_OZ_Module1
    {
        name = "OZ_Module1";
        displayName = "$STR_OZ_SLOT_MODULE";
    };

    class Slot_OZ_Module2
    {
        name = "OZ_Module2";
        displayName = "$STR_OZ_SLOT_MODULE";
    };

    class Slot_OZ_Module3
    {
        name = "OZ_Module3";
        displayName = "$STR_OZ_SLOT_MODULE";
    };
};

class CfgPatches
{
    class OpenZone_PDA
    {
        // Every scope=2 class belongs here. Two that used to be missing --
        // OZ_PDA_Sealed and OZ_Module_Decryptor -- were between them the whole
        // sealed-device feature: the quest PDA and the tool that opens it.
        //
        // OZ_Module_Antenna is gone (owner decision 2026-09-09): the antenna
        // and the GPS were one module split in two, and the GPS is the one
        // that stayed. Devices that hold a persisted antenna plate lose it
        // with the class.
        //
        // OZ_Module_Radiometer and OZ_Module_Dosimeter are gone the same way
        // (owner decision 2026-09-09). Radiation is not the PDA's subject:
        // the two boards were screens for numbers only a radiation mod can
        // supply, and with no such mod on the server they read "no data" from
        // the day they shipped. A mod that wants them declares its own board
        // and its own page, the way the radio does.
        units[] =
        {
            "OZ_PDA_Novice",
            "OZ_PDA_Advanced",
            "OZ_PDA_Sealed",
            "OZ_DataCarrier_Chip",
            "OZ_DataCarrier_Floppy",
            "OZ_DataCarrier_Drive",
            "OZ_Module_SpyAntenna",
            "OZ_Module_Decryptor",
            "OZ_Module_GPS"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "JM_CF_Scripts",
            // The core's CfgPatches class name -- that string IS the addon
            // identity, and nothing else resolves to it.
            "OpenZone_Core",
            // The survivor classes gain our wear slot below, so the
            // characters must be defined before this addon loads.
            "DZ_Characters",
            // Vanilla models borrowed until dayz-3d delivers our own.
            // Consumables is here for \dz\gear\consumables\9v_battery.p3d --
            // the module placeholder. It is easy to miss because the battery
            // SLOT is vanilla while the battery MODEL lives in a different
            // addon from the one the name suggests.
            "DZ_Gear_Navigation",
            "DZ_Gear_Tools",
            "DZ_Gear_Consumables"
        };
    };
};

class CfgMods
{
    class OpenZone_PDA
    {
        dir = "OpenZone_PDA";
        name = "OpenZone PDA";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";
        // 2: markers appended to the item's CF record. Bumping this is what
        // lets CF_OnStoreLoad tell an older save (no markers) from a new one.
        // 3: the "already seeded" flag for sealed devices, appended after the
        // markers. Preset content must be written exactly once, and the flag
        // is what remembers that it was.
        storageVersion = 7;
        dependencies[] = {"Game", "World", "Mission"};
        defines[] = {"OPENZONE_PDA"};

        // NO `inputs=` LINE, and there will not be one.
        //
        // The mod used to declare UAOZPdaOpen (data/inputs.xml, default O).
        // Owner decision 2026-09-08: the open key is gone. A device is opened
        // by the action on the item in hands and by nothing else; a screen
        // that a hotkey summons out of a backpack behaves like a browser tab,
        // not like a thing in the Zone. Escape still closes it.

        class defs
        {
            // imageSets arrive with the UI they serve: oz_pda_icons is the
            // first one, the atlas tools/icons/make_icons.py draws and
            // OpenZone_PDA/gui/imagesets/oz_pda_icons.imageset describes.

            class imageSets { files[] = {"OpenZone_PDA/gui/imagesets/oz_pda_icons.imageset"}; };
            class gameScriptModule    { value = ""; files[] = {"OpenZone_PDA/scripts/3_Game"}; };
            class worldScriptModule   { value = ""; files[] = {"OpenZone_PDA/scripts/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"OpenZone_PDA/scripts/5_Mission"}; };
        };
    };
};

class CfgVehicles
{
    // Every survivor carries ONE wear slot for the device. += keeps the
    // vanilla attachment list intact -- we add, never redeclare.
    class Man;
    class SurvivorBase : Man
    {
        attachments[] += {"OZ_PdaWear"};

        // Сітку екіпіровки гравця ваніль будує НЕ з attachments[], а з
        // цього списку (inventory.c: "CfgVehicles SurvivorBase
        // InventoryEquipment playerSlots"). Без рядка тут слот працює,
        // але в інвентарі його ніколи не видно.
        class InventoryEquipment
        {
            playerSlots[] += {"Slot_OZ_PdaWear"};
        };
    };

    class Inventory_Base;

    class OZ_PDA_Base : Inventory_Base
    {
        scope = 0;

        // Placeholder model. The real one comes from dayz-3d as a .p3d plus a
        // front-panel render used as the UI bezel -- both out of one .blend so
        // they cannot drift apart.
        model = "\dz\gear\navigation\GPSReceiver.p3d";

        itemSize[] = {2, 3};
        weight = 300;
        rotationFlags = 1;
        absorbency = 0.3;

        // Every bay is declared here; the profile decides how many of them a
        // given model actually exposes, and the script hides the rest. Slots
        // cannot be added at runtime, so the maximum lives in config and the
        // limit lives in JSON.
        attachments[] =
        {
            "BatteryD",
            "OZ_DataCarrier",
            "OZ_Module1",
            "OZ_Module2",
            "OZ_Module3"
        };

        // The device itself seats in the character's wear slot.
        inventorySlot[] = {"OZ_PdaWear"};

        class EnergyManager
        {
            hasIcon = 1;
            // OFF, and deliberately (owner's decision 2026-09-01). The engine
            // reads this in componentenergymanager.c:191 and the chain
            // EECargoIn -> OnMovedInsideCargo -> HandleMoveInsideCargo ->
            // SwitchOff() is straight. A PLAYER IS A CONTAINER TOO, so with
            // the flag on the device switched itself off the moment it went
            // into a backpack -- and, because the wear slot is an attachment
            // rather than cargo, the two cases were impossible to tell apart
            // from the player's side. The device now stays on wherever it is
            // carried; the battery keeps draining, which is the intended
            // friction.
            autoSwitchOffWhenInCargo = 0;
            // THE STARTING VALUE ONLY: 0.5 per minute, the rate of a device
            // with empty bays. The real rate is set from script on every
            // attach, detach and power-on -- OZ_PDA_Base.OZ_ApplyDrain
            // multiplies the profile's PowerDrainPerMin by the PowerFactor
            // of every inserted board and calls SetEnergyUsage (TZ-5
            // R-B2.2). Editing the number here changes nothing but the
            // first tick of a device whose class is in no profile.
            energyUsagePerSecond = 0.0083;
            // Stores nothing itself: it lives off the attached battery.
            energyStorageMax = 0;
            plugType = 1;             // PLUG_BATTERY_SLOT
            attachmentAction = 1;     // PLUG_THIS_INTO_ATTACHMENT
            updateInterval = 30;
        };
    };

    class OZ_PDA_Novice : OZ_PDA_Base
    {
        scope = 2;
        displayName = "$STR_OZ_PDA_NOVICE";
        descriptionShort = "$STR_OZ_PDA_NOVICE_DESC";
    };

    class OZ_PDA_Advanced : OZ_PDA_Base
    {
        scope = 2;
        displayName = "$STR_OZ_PDA_ADVANCED";
        descriptionShort = "$STR_OZ_PDA_ADVANCED_DESC";
    };

    // A sealed device. Its profile marks it Sealed, so the server gives it a
    // random code nobody is ever told and writes whatever the profile lists
    // onto it. A separate classname on purpose: profiles are keyed by class,
    // and a quest PDA has to be a different item, not a flag on a normal one.
    class OZ_PDA_Sealed : OZ_PDA_Base
    {
        scope = 2;
        displayName = "$STR_OZ_PDA_SEALED";
        descriptionShort = "$STR_OZ_PDA_SEALED_DESC";
    };

    // ------------------------------------------------------------- storage

    class OZ_DataCarrier_Base : Inventory_Base
    {
        scope = 0;
        // Placeholder. gear_navigation.pbo carries GPSReceiver, compass,
        // compass_modern and Map_chernarus_animated -- there is no plain
        // Map.p3d, so the old path resolved to nothing. A small electronic
        // prop stands in until dayz-3d delivers the chip.
        model = "\dz\gear\tools\RemoteDetonator_Receiver.p3d";
        itemSize[] = {1, 1};
        weight = 20;
        rotationFlags = 1;
        inventorySlot[] = {"OZ_DataCarrier"};
    };

    class OZ_DataCarrier_Chip : OZ_DataCarrier_Base
    {
        scope = 2;
        displayName = "$STR_OZ_CARRIER_CHIP";
        descriptionShort = "$STR_OZ_CARRIER_CHIP_DESC";
    };

    // Same prop, different capacities: the class name is the tier lever,
    // Hardware.json says how much fits on each.
    class OZ_DataCarrier_Floppy : OZ_DataCarrier_Base
    {
        scope = 2;
        displayName = "$STR_OZ_CARRIER_FLOPPY";
        descriptionShort = "$STR_OZ_CARRIER_FLOPPY_DESC";
    };

    class OZ_DataCarrier_Drive : OZ_DataCarrier_Base
    {
        scope = 2;
        displayName = "$STR_OZ_CARRIER_DRIVE";
        descriptionShort = "$STR_OZ_CARRIER_DRIVE_DESC";
    };

    // ------------------------------------------------------------- modules
    //
    // A module fits ANY bay: one inventorySlot[] listing all three, so the
    // player is not made to remember which hole a given chip belongs in.

    class OZ_Module_Base : Inventory_Base
    {
        scope = 0;
        // Placeholder, and the path is not the obvious one: gear_tools.pbo
        // holds no battery model at all -- the 9V lives under consumables.
        // The old \dz\gear\tools\Battery9V.p3d resolved to nothing.
        model = "\dz\gear\consumables\9v_battery.p3d";
        itemSize[] = {1, 1};
        weight = 60;
        rotationFlags = 1;
        inventorySlot[] = {"OZ_Module1", "OZ_Module2", "OZ_Module3"};
    };

    // The Geiger counter and the injected dose sensor stood here, and they
    // are gone (owner decision 2026-09-09): see the note in CfgPatches above.
    // Nothing replaces them in this mod -- radiation left with them, contract
    // and all.

    // GPS receiver, and the transponder with it (owner decision 2026-09-09).
    // Without it the map still shows marks and routes, but no "you", no
    // distances and no beacons (TZ-4 R-B2.2): a device that does not know
    // where it is cannot tell anyone. Its Hardware.json entry carries the
    // transponder range; longer-range towers come from OpenZone Radio and
    // simply declare a larger RangeM.
    class OZ_Module_GPS : OZ_Module_Base
    {
        scope = 2;
        displayName = "$STR_OZ_MOD_GPS";
        descriptionShort = "$STR_OZ_MOD_GPS_DESC";
    };

    // The spy variant: sees every transponder that is switched on at all,
    // privacy be damned -- for a fistful of minutes, then the plate burns.
    class OZ_Module_SpyAntenna : OZ_Module_Base
    {
        scope = 2;
        displayName = "$STR_OZ_MOD_SPY";
        descriptionShort = "$STR_OZ_MOD_SPY_DESC";
    };

    // Opens sealed devices, and only those. On an ordinary PDA it just takes
    // up a bay. The value of a sealed PDA is never the hardware.
    class OZ_Module_Decryptor : OZ_Module_Base
    {
        scope = 2;
        displayName = "$STR_OZ_MOD_DECRYPTOR";
        descriptionShort = "$STR_OZ_MOD_DECRYPTOR_DESC";
    };
};
