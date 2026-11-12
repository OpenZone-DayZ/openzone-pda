// Модуль «КПК» в адмінському вікні OpenZone (VPP rework 2026-11-12, design 5.3): три
// сторінки модуля "pda" на рейці вікна -- Налаштування (Tuning.json формою), Обладнання
// (Hardware.json: модулі й носії) і Профілі (сирий Profiles.json, доки його не візьме
// Ядро › Файли конфігів). Кожна сторінка -- своя розмітка з опису в ui/OpenZone_PDA_VPP/,
// зареєстрована RegisterPage; ядро про КПК не знає, КПК докладає свої сторінки сам цим
// modded-шаром.
//
// Усе, що йде на сервер або викидає роботу адміна, питає на смузі підтвердження вікна
// (design 3.4): зберегти налаштування чи запис, відкинути незбережені налаштування,
// видалити запис, застосувати сирі профілі. «Так» вертається в OnConfirmed дією "pda:".
//
// Гарди: NO_GUI -- сервер компілює Mission без UI; AVPPAdminTools і OpenZone_VPP --
// імена класів CfgMods (їх авто-дефайнить рушій).
//
// Члени цього шару починаються з Pda (поля -- m_Pda): той самий клас модифікує кожна
// склейка, і ім'я, яке мають двоє, не компілюється. Імена віджетів теж унікальні на все
// вікно: шукаються від M_SUB_WIDGET.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string PDA_PAGE_TUN  = "pda.tuning";
    static const string PDA_PAGE_HW   = "pda.hardware";
    static const string PDA_PAGE_PROF = "pda.profiles";

    // ------------------------------------------------- Налаштування
    // The file as the server last sent it: the boxes are compared with it.
    private ref OZ_PdaTuning m_PdaTun;
    // The shipped values, for the line under each box.
    private ref OZ_PdaTuning m_PdaTunDef;
    // The form as a table: a field of OZ_PdaTuning, its caption, its group's legend.
    private ref array<string> m_PdaTunKeys;
    private ref array<string> m_PdaTunLabels;
    private ref array<string> m_PdaTunGroups;

    // The script writes the boxes: an OnChange that comes of it is not an edit.
    private bool m_PdaFill = false;

    // ------------------------------------------------- Обладнання
    private ref OZ_PdaHardwareConfig m_PdaHwCfg;
    // A row of HwList -> what it is ("head", "module", "carrier") and its index.
    private ref array<string> m_PdaHwRowKind;
    private ref array<int>    m_PdaHwRowIdx;
    // The entry on the card: its kind, its index, its class as the file has it, and
    // whether it is new (not in the file yet).
    private string m_PdaHwKind = "";
    private int    m_PdaHwIdx = -1;
    private string m_PdaHwClass = "";
    private bool   m_PdaHwNew = false;
    private bool   m_PdaHwWritable = true;
    // Rebuilding HwList selects a row; the OnItemSelected that comes of it is not a pick.
    private bool   m_PdaRepaint = false;

    // ------------------------------------------------- Профілі
    // The editor holds less than the whole file: applying would cut it.
    private bool m_PdaProfCut = false;
    // The page asked for the file: the answer goes into the editor. Ядро › Файли
    // конфігів asks for the same file, and its answer leaves this editor alone.
    private bool m_PdaProfWaiting = false;
    private bool m_PdaProfAsked = false;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;

        m_PdaHwRowKind = new array<string>();
        m_PdaHwRowIdx  = new array<int>();
        m_PdaTunKeys   = new array<string>();
        m_PdaTunLabels = new array<string>();
        m_PdaTunGroups = new array<string>();
        PdaTunTable();

        m_PdaTunDef = new OZ_PdaTuning();
        m_PdaTunDef.LoadDefaults();

        PdaAddPage(PDA_PAGE_TUN,  "#STR_OZ_PDA_VPP_PAGE_TUN",  "oz_pda_vpp_tuning",   "PdaTunHint");
        PdaAddPage(PDA_PAGE_HW,   "#STR_OZ_PDA_VPP_PAGE_HW",   "oz_pda_vpp_hardware", "PdaHwHint");
        PdaAddPage(PDA_PAGE_PROF, "#STR_OZ_PDA_VPP_PAGE_PROF", "oz_pda_vpp_profiles", "PdaProfHint");

        // The defaults under the boxes and the empty card, before any file arrives.
        PdaTunPaint();
        PdaHwPaintCard();
    }

    private void PdaAddPage(string id, string label, string layout, string hint)
    {
        string path = "OpenZone_PDA_VPP/gui/layouts/" + layout + ".layout";
        Widget pane = GetGame().GetWorkspace().CreateWidgets(path, M_SUB_WIDGET);
        if (!pane)
        {
            OZ_Log.Error("pda vpp page: layout failed to load: " + path);
            return;
        }
        RegisterPage("pda", "#STR_OZ_PDA_VPP_MOD", id, label, pane, hint);
    }

    // A page of the module is shown: its file, unless the page holds the admin's work.
    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);

        if (id == PDA_PAGE_TUN)
        {
            array<int> changed = new array<int>();
            PdaTunChanged(changed);
            if (changed.Count() == 0)
                AskCfg("Tuning");
            return;
        }

        if (id == PDA_PAGE_HW)
        {
            // The answer keeps a card with unsaved edits (PdaOnHardwareText).
            AskCfg("Hardware");
            return;
        }

        if (id == PDA_PAGE_PROF && !m_PdaProfAsked)
            PdaAskProfiles();
    }

    // Every name goes on to super: Ядро › Файли конфігів shows the same files raw, and
    // a layer that kept a name to itself left that page's editor empty for it.
    override void OnCfgText(string name, string body)
    {
        if (name == "Tuning")
            PdaOnTuningText(body);
        else if (name == "Hardware")
            PdaOnHardwareText(body);
        else if (name == "Profiles")
            PdaOnProfilesText(body);

        super.OnCfgText(name, body);
    }

    // A file was applied: the page on screen reads its file back as the server keeps it.
    override void OnCfgApplied()
    {
        super.OnCfgApplied();

        string shown = CurrentPane();
        if (shown == PDA_PAGE_TUN)
            AskCfg("Tuning");
        else if (shown == PDA_PAGE_HW)
            AskCfg("Hardware");
        else if (shown == PDA_PAGE_PROF)
            PdaAskProfiles();
    }

    // The confirm strip's "yes" (design 3.4). The PDA's actions start with "pda:".
    override void OnConfirmed(string action)
    {
        if (action.IndexOf("pda:") != 0)
        {
            super.OnConfirmed(action);
            return;
        }

        if (action == "pda:tun")
        {
            PdaTunSaveNow();
            return;
        }
        if (action == "pda:tundrop")
        {
            PdaFillTuning();
            Hint("#STR_OZ_PDA_VPP_H_DROPPED");
            return;
        }
        if (action == "pda:hwsave")
        {
            PdaHwSaveNow();
            return;
        }
        if (action.IndexOf("pda:hwdel:") == 0)
        {
            PdaHwDeleteNow(action.Substring(10, action.Length() - 10));
            return;
        }
        if (action == "pda:prof")
            PdaProfApplyNow();
    }

    // ================================================= Налаштування

    // The form in the order of the page: three columns, six groups.
    private void PdaTunTable()
    {
        PdaTunRow("ChatMsgMaxBytes",       "CHAT_MSG",       "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("ChatTitleMaxBytes",     "CHAT_TITLE",     "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("ChatDescMaxBytes",      "CHAT_DESC",      "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("ChatHistoryOpen",       "CHAT_HIST_OPEN", "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("ChatHistoryPage",       "CHAT_HIST_PAGE", "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("ChatGroupMax",          "CHAT_GROUP",     "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("GroupInviteTtlSeconds", "CHAT_INVITE",    "#STR_OZ_PDA_VPP_G_CHAT");
        PdaTunRow("PinMaxFails",           "PIN_FAILS",      "#STR_OZ_PDA_VPP_G_PIN");
        PdaTunRow("PinLockoutSeconds",     "PIN_LOCK",       "#STR_OZ_PDA_VPP_G_PIN");
        PdaTunRow("NoteTitleMaxBytes",     "NOTE_TITLE",     "#STR_OZ_PDA_VPP_G_NOTES");
        PdaTunRow("NoteBodyMaxBytes",      "NOTE_BODY",      "#STR_OZ_PDA_VPP_G_NOTES");
        PdaTunRow("FriendReachMeters",     "REACH",          "#STR_OZ_PDA_VPP_G_CONTACTS");
        PdaTunRow("SwapOfferTtlSeconds",   "OFFER",          "#STR_OZ_PDA_VPP_G_CONTACTS");
        PdaTunRow("MarkerNameMaxBytes",    "MARK_NAME",      "#STR_OZ_PDA_VPP_G_MARKS");
        PdaTunRow("MarkerDescMaxBytes",    "MARK_DESC",      "#STR_OZ_PDA_VPP_G_MARKS");
        PdaTunRow("MapHome",               "MAP_HOME",       "#STR_OZ_PDA_VPP_G_MARKS");
        PdaTunRow("ToastSeconds",          "TOAST",          "#STR_OZ_PDA_VPP_G_HUD");
        PdaTunRow("RouteAdvanceMeters",    "ROUTE",          "#STR_OZ_PDA_VPP_G_HUD");
        PdaTunRow("BeaconPushSeconds",     "BEACON",         "#STR_OZ_PDA_VPP_G_HUD");
    }

    private void PdaTunRow(string key, string code, string group)
    {
        m_PdaTunKeys.Insert(key);
        m_PdaTunLabels.Insert("#STR_OZ_PDA_VPP_F_" + code);
        m_PdaTunGroups.Insert(group);
    }

    // A whole-number field of OZ_PdaTuning by its name. The form is a table and the class
    // has no reflection to lean on, so the eighteen numbers are named here once.
    private int PdaTunGet(OZ_PdaTuning t, string key)
    {
        if (key == "PinMaxFails")
            return t.PinMaxFails;
        if (key == "PinLockoutSeconds")
            return t.PinLockoutSeconds;
        if (key == "ChatMsgMaxBytes")
            return t.ChatMsgMaxBytes;
        if (key == "ChatTitleMaxBytes")
            return t.ChatTitleMaxBytes;
        if (key == "ChatDescMaxBytes")
            return t.ChatDescMaxBytes;
        if (key == "ChatHistoryOpen")
            return t.ChatHistoryOpen;
        if (key == "ChatHistoryPage")
            return t.ChatHistoryPage;
        if (key == "GroupInviteTtlSeconds")
            return t.GroupInviteTtlSeconds;
        if (key == "ChatGroupMax")
            return t.ChatGroupMax;
        if (key == "NoteTitleMaxBytes")
            return t.NoteTitleMaxBytes;
        if (key == "NoteBodyMaxBytes")
            return t.NoteBodyMaxBytes;
        if (key == "MarkerNameMaxBytes")
            return t.MarkerNameMaxBytes;
        if (key == "MarkerDescMaxBytes")
            return t.MarkerDescMaxBytes;
        if (key == "FriendReachMeters")
            return t.FriendReachMeters;
        if (key == "SwapOfferTtlSeconds")
            return t.SwapOfferTtlSeconds;
        if (key == "ToastSeconds")
            return t.ToastSeconds;
        if (key == "RouteAdvanceMeters")
            return t.RouteAdvanceMeters;
        if (key == "BeaconPushSeconds")
            return t.BeaconPushSeconds;
        return 0;
    }

    private void PdaTunSet(OZ_PdaTuning t, string key, int v)
    {
        if (key == "PinMaxFails")
            t.PinMaxFails = v;
        else if (key == "PinLockoutSeconds")
            t.PinLockoutSeconds = v;
        else if (key == "ChatMsgMaxBytes")
            t.ChatMsgMaxBytes = v;
        else if (key == "ChatTitleMaxBytes")
            t.ChatTitleMaxBytes = v;
        else if (key == "ChatDescMaxBytes")
            t.ChatDescMaxBytes = v;
        else if (key == "ChatHistoryOpen")
            t.ChatHistoryOpen = v;
        else if (key == "ChatHistoryPage")
            t.ChatHistoryPage = v;
        else if (key == "GroupInviteTtlSeconds")
            t.GroupInviteTtlSeconds = v;
        else if (key == "ChatGroupMax")
            t.ChatGroupMax = v;
        else if (key == "NoteTitleMaxBytes")
            t.NoteTitleMaxBytes = v;
        else if (key == "NoteBodyMaxBytes")
            t.NoteBodyMaxBytes = v;
        else if (key == "MarkerNameMaxBytes")
            t.MarkerNameMaxBytes = v;
        else if (key == "MarkerDescMaxBytes")
            t.MarkerDescMaxBytes = v;
        else if (key == "FriendReachMeters")
            t.FriendReachMeters = v;
        else if (key == "SwapOfferTtlSeconds")
            t.SwapOfferTtlSeconds = v;
        else if (key == "ToastSeconds")
            t.ToastSeconds = v;
        else if (key == "RouteAdvanceMeters")
            t.RouteAdvanceMeters = v;
        else if (key == "BeaconPushSeconds")
            t.BeaconPushSeconds = v;
    }

    // A field as the box shows it: MapHome is words, the rest whole numbers.
    private string PdaTunText(OZ_PdaTuning t, string key)
    {
        if (key == "MapHome")
            return t.MapHome;
        int v = PdaTunGet(t, key);
        return v.ToString();
    }

    // The file arrived. A form with the admin's unsaved changes is left as it is: the
    // same file arrives for Ядро › Файли конфігів and after the page is shown again.
    private void PdaOnTuningText(string body)
    {
        if (m_PdaTun)
        {
            array<int> changed = new array<int>();
            PdaTunChanged(changed);
            if (changed.Count() > 0)
            {
                if (CurrentPane() == PDA_PAGE_TUN)
                    Hint("#STR_OZ_PDA_VPP_H_KEPT");
                return;
            }
        }

        // The root made by the script, the way the core's header (OZ_ConfigBase) says:
        // a field the file lacks keeps its initialiser. OZ_PdaTuning holds no nested
        // object, so nothing the serializer allocated lives on in it.
        OZ_PdaTuning t = new OZ_PdaTuning();
        string err;
        if (!JsonFileLoader<OZ_PdaTuning>.LoadData(body, t, err) || !t)
        {
            if (CurrentPane() == PDA_PAGE_TUN)
            {
                string bad = Widget.TranslateString("#STR_OZ_PDA_VPP_H_BAD_FILE");
                Hint("Tuning.json " + bad + " " + err);
            }
            return;
        }

        m_PdaTun = t;
        PdaFillTuning();
    }

    private void PdaFillTuning()
    {
        if (m_PdaTun)
        {
            m_PdaFill = true;
            for (int i = 0; i < m_PdaTunKeys.Count(); i++)
            {
                string key = m_PdaTunKeys[i];
                SetEdit("Tun_" + key, PdaTunText(m_PdaTun, key));
            }
            m_PdaFill = false;
        }
        PdaTunPaint();
    }

    // A box that holds no whole number counts as changed: saving names it instead of
    // writing a 0 the admin never typed.
    private bool PdaTunFieldChanged(int i)
    {
        string key = m_PdaTunKeys[i];
        string typed = PdaTrimmed("Tun_" + key);
        if (key == "MapHome")
            return typed != m_PdaTun.MapHome;
        if (!PdaIsInt(typed))
            return true;
        int v = typed.ToInt();
        return v != PdaTunGet(m_PdaTun, key);
    }

    // The fields whose box differs from the file, in the order of the page.
    private void PdaTunChanged(array<int> outIdx)
    {
        outIdx.Clear();
        if (!m_PdaTun)
            return;
        for (int i = 0; i < m_PdaTunKeys.Count(); i++)
        {
            if (PdaTunFieldChanged(i))
                outIdx.Insert(i);
        }
    }

    // "ЧАТ: назва, байт", and with values "ЧАТ: назва, байт 32 -> 40".
    private string PdaTunItem(int i, bool withValues)
    {
        string grp = Widget.TranslateString(m_PdaTunGroups[i]);
        string lab = Widget.TranslateString(m_PdaTunLabels[i]);
        string item = grp + ": " + lab;
        if (!withValues || !m_PdaTun)
            return item;

        string key = m_PdaTunKeys[i];
        string was = PdaTunText(m_PdaTun, key);
        string typed = PdaTrimmed("Tun_" + key);
        item = item + " " + was;
        item = item + " -> " + typed;
        return item;
    }

    private string PdaTunList(array<int> idx, bool withValues)
    {
        string list = "";
        for (int i = 0; i < idx.Count(); i++)
        {
            string item = PdaTunItem(idx[i], withValues);
            if (list != "")
                list = list + "; ";
            list = list + item;
        }
        return list;
    }

    // Under each box its default, in the alert colour with "changed" while the box
    // differs from the file; the bar names what is not saved yet.
    private void PdaTunPaint()
    {
        if (!M_SUB_WIDGET || !m_PdaTunKeys)
            return;

        string defWord = Widget.TranslateString("#STR_OZ_PDA_VPP_DEFAULT");
        string changedWord = Widget.TranslateString("#STR_OZ_PDA_VPP_CHANGED");
        string homeDef = Widget.TranslateString("#STR_OZ_PDA_VPP_DEF_HOME");
        array<int> changed = new array<int>();

        for (int i = 0; i < m_PdaTunKeys.Count(); i++)
        {
            string key = m_PdaTunKeys[i];
            bool dirty = false;
            if (m_PdaTun)
                dirty = PdaTunFieldChanged(i);

            string line = defWord + " " + PdaTunText(m_PdaTunDef, key);
            if (key == "MapHome")
                line = homeDef;

            int ink = OZ_PdaVppMetal.MUTED;
            int edge = OZ_PdaVppMetal.EDGE;
            if (dirty)
            {
                line = line + ", " + changedWord;
                ink = OZ_PdaVppMetal.ALERT;
                edge = OZ_PdaVppMetal.ALERT;
                changed.Insert(i);
            }
            PdaText("TunD_" + key, line, ink);
            PdaColor("Tun_" + key + "Frame", edge);
        }

        if (changed.Count() == 0)
        {
            PdaText("PdaTunDirty", "#STR_OZ_PDA_VPP_NO_CHANGES", OZ_PdaVppMetal.MUTED);
            return;
        }
        string unsaved = Widget.TranslateString("#STR_OZ_PDA_VPP_UNSAVED");
        string list = PdaTunList(changed, false);
        PdaText("PdaTunDirty", unsaved + " " + list, OZ_PdaVppMetal.ALERT);
    }

    private void PdaTunAskSave()
    {
        if (!m_PdaTun)
        {
            Hint("#STR_OZ_PDA_VPP_H_NOT_LOADED");
            return;
        }

        array<int> changed = new array<int>();
        PdaTunChanged(changed);
        if (changed.Count() == 0)
        {
            Hint("#STR_OZ_PDA_VPP_NO_CHANGES");
            return;
        }

        // A box that holds no whole number is named, not saved as 0.
        for (int i = 0; i < changed.Count(); i++)
        {
            string key = m_PdaTunKeys[changed[i]];
            if (key == "MapHome")
                continue;
            string typed = PdaTrimmed("Tun_" + key);
            if (PdaIsInt(typed))
                continue;
            string item = PdaTunItem(changed[i], false);
            string must = Widget.TranslateString("#STR_OZ_PDA_VPP_H_NUMBER");
            Hint(item + " - " + must);
            return;
        }

        string ask = Widget.TranslateString("#STR_OZ_PDA_VPP_ASK_TUN");
        string list = PdaTunList(changed, true);
        Confirm(ask + " " + list, "#STR_OZ_PDA_VPP_YES_SAVE", "pda:tun");
    }

    private void PdaTunAskDrop()
    {
        array<int> changed = new array<int>();
        PdaTunChanged(changed);
        if (changed.Count() == 0)
        {
            Hint("#STR_OZ_PDA_VPP_NO_CHANGES");
            return;
        }
        string ask = Widget.TranslateString("#STR_OZ_PDA_VPP_ASK_TUN_DROP");
        string list = PdaTunList(changed, true);
        Confirm(ask + " " + list, "#STR_OZ_PDA_VPP_YES_DROP", "pda:tundrop");
    }

    // The strip's "yes": the boxes as they are now go into the LOADED object -- Version
    // and fields the form does not know survive the save untouched -- and the file goes
    // to the server whole (cfg_set). The answer reads the file back (OnCfgApplied).
    private void PdaTunSaveNow()
    {
        if (!m_PdaTun)
        {
            Hint("#STR_OZ_PDA_VPP_H_NOT_LOADED");
            return;
        }

        for (int i = 0; i < m_PdaTunKeys.Count(); i++)
        {
            string key = m_PdaTunKeys[i];
            string typed = PdaTrimmed("Tun_" + key);
            if (key == "MapHome")
            {
                m_PdaTun.MapHome = typed;
                continue;
            }
            if (PdaIsInt(typed))
                PdaTunSet(m_PdaTun, key, typed.ToInt());
        }

        string body;
        string err;
        if (!JsonFileLoader<OZ_PdaTuning>.MakeData(m_PdaTun, body, err, false))
        {
            Hint("#STR_OZ_PDA_VPP_H_NO_JSON");
            return;
        }
        SendCfg("Tuning", body);
        PdaTunPaint();
    }

    // ================================================= Обладнання

    // The file arrived: the list follows it, the card finds its entry again by class --
    // the order may have changed -- and keeps the admin's unsaved edits.
    private void PdaOnHardwareText(string body)
    {
        OZ_PdaHardwareConfig hc = new OZ_PdaHardwareConfig();
        string err;
        if (!JsonFileLoader<OZ_PdaHardwareConfig>.LoadData(body, hc, err) || !hc)
        {
            if (CurrentPane() == PDA_PAGE_HW)
            {
                string bad = Widget.TranslateString("#STR_OZ_PDA_VPP_H_BAD_FILE");
                Hint("Hardware.json " + bad + " " + err);
            }
            return;
        }

        // The root was made by the script; every entry of the two lists by the
        // serializer, and the admin edits them long before saving (the core's
        // OZ_ConfigBase header). They move into script-made copies at once, while the
        // read is honest, and the lists themselves are the script's own.
        array<ref OZ_ModuleSpec> mods = new array<ref OZ_ModuleSpec>();
        array<ref OZ_CarrierSpec> cars = new array<ref OZ_CarrierSpec>();
        int hi;
        for (hi = 0; hc.Modules && hi < hc.Modules.Count(); hi++)
        {
            if (hc.Modules[hi])
                mods.Insert(hc.Modules[hi].Copy());
        }
        for (hi = 0; hc.Carriers && hi < hc.Carriers.Count(); hi++)
        {
            if (hc.Carriers[hi])
                cars.Insert(hc.Carriers[hi].Copy());
        }
        hc.Modules = mods;
        hc.Carriers = cars;

        // Asked of the card as the OLD file had it, before the new one replaces it.
        bool keep = PdaHwDirty();

        m_PdaHwCfg = hc;
        PdaHwRebuildList();
        PdaHwRefind(keep);
    }

    private void PdaHwRebuildList()
    {
        TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("HwList"));
        if (!lb || !m_PdaHwCfg)
            return;

        m_PdaRepaint = true;
        lb.ClearItems();
        m_PdaHwRowKind.Clear();
        m_PdaHwRowIdx.Clear();

        string modWord = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_MODULES");
        string modHead = modWord + ": " + m_PdaHwCfg.Modules.Count().ToString();
        int modRow = lb.AddItem(modHead, NULL, 0);
        lb.SetItemColor(modRow, 0, OZ_PdaVppMetal.MUTED);
        m_PdaHwRowKind.Insert("head");
        m_PdaHwRowIdx.Insert(-1);

        for (int i = 0; i < m_PdaHwCfg.Modules.Count(); i++)
        {
            OZ_ModuleSpec ms = m_PdaHwCfg.Modules[i];
            string mline = "M   " + ms.ClassName;
            mline = mline + "   " + PdaHwModSum(ms);
            lb.AddItem(mline, NULL, 0);
            m_PdaHwRowKind.Insert("module");
            m_PdaHwRowIdx.Insert(i);
        }

        string carWord = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_CARRIERS");
        string carHead = carWord + ": " + m_PdaHwCfg.Carriers.Count().ToString();
        int carRow = lb.AddItem(carHead, NULL, 0);
        lb.SetItemColor(carRow, 0, OZ_PdaVppMetal.MUTED);
        m_PdaHwRowKind.Insert("head");
        m_PdaHwRowIdx.Insert(-1);

        for (int c = 0; c < m_PdaHwCfg.Carriers.Count(); c++)
        {
            OZ_CarrierSpec cs = m_PdaHwCfg.Carriers[c];
            string cline = "C   " + cs.ClassName;
            cline = cline + "   " + PdaHwCarSum(cs);
            lb.AddItem(cline, NULL, 0);
            m_PdaHwRowKind.Insert("carrier");
            m_PdaHwRowIdx.Insert(c);
        }

        m_PdaRepaint = false;
    }

    // "gps, 5000 m, x1.6": the kind, the range and the spy minutes when they are set, the
    // power factor.
    private string PdaHwModSum(OZ_ModuleSpec ms)
    {
        string sum = ms.Kind;
        if (sum == "")
            sum = "?";
        if (ms.RangeM > 0)
        {
            string meters = Widget.TranslateString("#STR_OZ_PDA_VPP_UNIT_M");
            sum = sum + ", " + ms.RangeM.ToString();
            sum = sum + " " + meters;
        }
        if (ms.SpyMinutes > 0)
        {
            string minutes = Widget.TranslateString("#STR_OZ_PDA_VPP_UNIT_MIN");
            sum = sum + ", " + ms.SpyMinutes.ToString();
            sum = sum + " " + minutes;
        }
        sum = sum + ", x" + ms.PowerFactor.ToString();
        return sum;
    }

    // "writes, 6 records" or "read only, no limit".
    private string PdaHwCarSum(OZ_CarrierSpec cs)
    {
        string sum = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_READ_ONLY");
        if (cs.Writable)
            sum = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_WRITES");
        if (cs.MaxRecords > 0)
        {
            string records = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_RECORDS_N");
            sum = sum + ", " + cs.MaxRecords.ToString();
            sum = sum + " " + records;
            return sum;
        }
        string noLimit = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_NO_LIMIT");
        sum = sum + ", " + noLimit;
        return sum;
    }

    private void PdaHwPick(int row)
    {
        if (!m_PdaHwCfg || row < 0 || row >= m_PdaHwRowKind.Count())
            return;
        string kind = m_PdaHwRowKind[row];
        if (kind == "head")
            return;
        PdaHwFill(kind, m_PdaHwRowIdx[row]);
    }

    private void PdaHwFill(string kind, int idx)
    {
        if (!m_PdaHwCfg)
            return;

        m_PdaHwKind = kind;
        m_PdaHwIdx  = idx;
        m_PdaHwNew  = false;
        m_PdaFill   = true;

        if (kind == "module")
        {
            OZ_ModuleSpec ms = m_PdaHwCfg.Modules[idx];
            m_PdaHwClass = ms.ClassName;
            SetEdit("HwClass", ms.ClassName);
            SetEdit("HwName",  ms.DisplayName);
            SetEdit("HwKind",  ms.Kind);
            SetEdit("HwRange", ms.RangeM.ToString());
            SetEdit("HwPower", ms.PowerFactor.ToString());
            SetEdit("HwSpy",   ms.SpyMinutes.ToString());
            SetEdit("HwPages", PdaJoinPages(ms.EnablesPages));
        }
        else
        {
            OZ_CarrierSpec cs = m_PdaHwCfg.Carriers[idx];
            m_PdaHwClass = cs.ClassName;
            SetEdit("HwCarClass", cs.ClassName);
            SetEdit("HwCarName",  cs.DisplayName);
            // One number instead of two: a carrier counts RECORDS, of any kind
            // (OZ_CarrierSpec).
            SetEdit("HwMarks", cs.MaxRecords.ToString());
            m_PdaHwWritable = cs.Writable;
        }

        m_PdaFill = false;
        PdaHwPaintCard();
    }

    // A new entry: the box empty but for what a new entry starts with.
    private void PdaHwNewForm(string kind)
    {
        if (!m_PdaHwCfg)
        {
            Hint("#STR_OZ_PDA_VPP_H_NOT_LOADED");
            return;
        }

        m_PdaHwKind  = kind;
        m_PdaHwIdx   = -1;
        m_PdaHwClass = "";
        m_PdaHwNew   = true;
        m_PdaFill    = true;

        if (kind == "module")
        {
            SetEdit("HwClass", "");
            SetEdit("HwName",  "");
            // The kind is SUGGESTED, not guessed: the decryptor is the one kind a device
            // may lack and of which a second board breaks nothing. An empty kind would
            // meet Validate's complaint "the module attaches and does nothing".
            SetEdit("HwKind",  OZ_PdaConst.MOD_DECRYPTOR);
            SetEdit("HwRange", "0");
            SetEdit("HwPower", "1.0");
            SetEdit("HwSpy",   "0");
            SetEdit("HwPages", "");
        }
        else
        {
            SetEdit("HwCarClass", "");
            SetEdit("HwCarName",  "");
            SetEdit("HwMarks",    "0");
            m_PdaHwWritable = true;
        }

        m_PdaFill = false;
        PdaHwSelectPicked();
        PdaHwPaintCard();
        if (kind == "module")
            Hint("#STR_OZ_PDA_VPP_H_NEW_MOD");
        else
            Hint("#STR_OZ_PDA_VPP_H_NEW_CAR");
    }

    private void PdaHwClear()
    {
        m_PdaHwKind  = "";
        m_PdaHwIdx   = -1;
        m_PdaHwClass = "";
        m_PdaHwNew   = false;
        PdaHwSelectPicked();
        PdaHwPaintCard();
    }

    // The entry's index in its list, by class; -1: not there.
    private int PdaHwFind(string kind, string cls)
    {
        if (!m_PdaHwCfg)
            return -1;
        int i;
        if (kind == "module")
        {
            for (i = 0; i < m_PdaHwCfg.Modules.Count(); i++)
            {
                if (m_PdaHwCfg.Modules[i] && m_PdaHwCfg.Modules[i].ClassName == cls)
                    return i;
            }
            return -1;
        }
        for (i = 0; i < m_PdaHwCfg.Carriers.Count(); i++)
        {
            if (m_PdaHwCfg.Carriers[i] && m_PdaHwCfg.Carriers[i].ClassName == cls)
                return i;
        }
        return -1;
    }

    // After a new file: the card's entry again, by class. `keepForm`: the boxes hold the
    // admin's unsaved edits and stay as they are -- saving writes them into the entry
    // of the NEW file, found here.
    private void PdaHwRefind(bool keepForm)
    {
        if (m_PdaHwKind == "" || m_PdaHwNew)
        {
            PdaHwSelectPicked();
            PdaHwPaintCard();
            return;
        }

        bool shown = CurrentPane() == PDA_PAGE_HW;
        int at = PdaHwFind(m_PdaHwKind, m_PdaHwClass);
        if (at < 0)
        {
            PdaHwClear();
            if (shown)
                Hint("#STR_OZ_PDA_VPP_H_GONE");
            return;
        }

        m_PdaHwIdx = at;
        if (!keepForm)
        {
            PdaHwFill(m_PdaHwKind, at);
            PdaHwSelectPicked();
            return;
        }

        PdaHwSelectPicked();
        PdaHwPaintCard();
        if (shown)
            Hint("#STR_OZ_PDA_VPP_H_KEPT");
    }

    // Whether the boxes differ from the card's entry as the file has it. A new entry is
    // unsaved work by definition.
    private bool PdaHwDirty()
    {
        if (m_PdaHwNew)
            return true;
        if (!m_PdaHwCfg || m_PdaHwIdx < 0)
            return false;

        if (m_PdaHwKind == "module")
        {
            if (m_PdaHwIdx >= m_PdaHwCfg.Modules.Count())
                return false;
            OZ_ModuleSpec ms = m_PdaHwCfg.Modules[m_PdaHwIdx];
            if (PdaTrimmed("HwClass") != ms.ClassName)
                return true;
            if (PdaTrimmed("HwName") != ms.DisplayName)
                return true;
            if (PdaTrimmed("HwKind") != ms.Kind)
                return true;
            if (PdaTrimmed("HwRange") != ms.RangeM.ToString())
                return true;
            if (PdaTrimmed("HwPower") != ms.PowerFactor.ToString())
                return true;
            if (PdaTrimmed("HwSpy") != ms.SpyMinutes.ToString())
                return true;
            string pages = PdaJoinPages(ms.EnablesPages);
            return PdaTrimmed("HwPages") != pages;
        }

        if (m_PdaHwKind == "carrier")
        {
            if (m_PdaHwIdx >= m_PdaHwCfg.Carriers.Count())
                return false;
            OZ_CarrierSpec cs = m_PdaHwCfg.Carriers[m_PdaHwIdx];
            if (PdaTrimmed("HwCarClass") != cs.ClassName)
                return true;
            if (PdaTrimmed("HwCarName") != cs.DisplayName)
                return true;
            if (m_PdaHwWritable != cs.Writable)
                return true;
            string records = cs.MaxRecords.ToString();
            return PdaTrimmed("HwMarks") != records;
        }
        return false;
    }

    // The list's highlight on the card's entry; none for a new entry or an empty card.
    private void PdaHwSelectPicked()
    {
        TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("HwList"));
        if (!lb)
            return;
        int row = -1;
        if (!m_PdaHwNew)
        {
            for (int i = 0; i < m_PdaHwRowKind.Count(); i++)
            {
                if (m_PdaHwRowKind[i] == m_PdaHwKind && m_PdaHwRowIdx[i] == m_PdaHwIdx)
                    row = i;
            }
        }
        m_PdaRepaint = true;
        lb.SelectRow(row);
        m_PdaRepaint = false;
    }

    // The card: the module box or the carrier box, its title and badge, the line that
    // says a mod declared the entry, and save and delete under the open box.
    private void PdaHwPaintCard()
    {
        if (!M_SUB_WIDGET)
            return;

        bool isMod = m_PdaHwKind == "module";
        bool isCar = m_PdaHwKind == "carrier";
        bool any = isMod || isCar;

        PdaShow("PdaHwEmpty", !any);
        PdaShow("PdaHwCardTitle", any);
        PdaShow("PdaHwCardBadge", any);
        // A panel with an edge is two widgets: the generator draws the border as a
        // sibling, <Name>Edge, and it is shown and hidden with the panel.
        PdaShow("PdaHwModBox", isMod);
        PdaShow("PdaHwModBoxEdge", isMod);
        PdaShow("PdaHwCarBox", isCar);
        PdaShow("PdaHwCarBoxEdge", isCar);
        PdaShow("BtnHwSave", any);
        PdaShow("BtnHwDel", any);

        string title = m_PdaHwClass;
        if (m_PdaHwNew && isMod)
            title = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_NEW_MOD_TITLE");
        else if (m_PdaHwNew)
            title = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_NEW_CAR_TITLE");
        PdaText("PdaHwCardTitle", title, OZ_PdaVppMetal.TEXT);

        if (isMod)
            PdaText("PdaHwCardBadgeText", "#STR_OZ_PDA_VPP_HW_G_MODULE", OZ_PdaVppMetal.MUTED);
        else
            PdaText("PdaHwCardBadgeText", "#STR_OZ_PDA_VPP_HW_G_CARRIER", OZ_PdaVppMetal.MUTED);

        // A module another mod declared comes back with every load of the file; saving
        // makes it the admin's (an empty Origin), deleting does not help (PdaHwAskDelete).
        string origin = "";
        OZ_ModuleSpec declared = PdaHwPickedModule();
        if (declared && declared.Origin != "")
            origin = "#STR_OZ_PDA_VPP_HW_ORIGIN";
        PdaText("PdaHwOrigin", origin, OZ_PdaVppMetal.ALERT);

        // Save and delete under the open box: the carrier's box is shorter.
        float s = UnitScale();
        float y = 372;
        if (isCar)
            y = 286;
        PdaMove("BtnHwSave", 24 * s, y * s);
        PdaMove("BtnHwDel", 172 * s, y * s);
        PdaMove("PdaHwHint", 24 * s, (y + 44) * s);

        PdaHwPaintWritable();
        PdaHwPaintGameNames();
    }

    // The card's module as the file has it; null for a carrier, a new entry or none.
    private OZ_ModuleSpec PdaHwPickedModule()
    {
        if (!m_PdaHwCfg || m_PdaHwNew || m_PdaHwKind != "module")
            return null;
        if (m_PdaHwIdx < 0 || m_PdaHwIdx >= m_PdaHwCfg.Modules.Count())
            return null;
        return m_PdaHwCfg.Modules[m_PdaHwIdx];
    }

    // Writes: yes / no, a pair of buttons; the chosen one is lit.
    private void PdaHwPaintWritable()
    {
        int yesBg = OZ_PdaVppMetal.PANEL;
        int noBg = OZ_PdaVppMetal.RAISED;
        int yesInk = OZ_PdaVppMetal.MUTED;
        int noInk = OZ_PdaVppMetal.TEXT;
        int yesEdge = OZ_PdaVppMetal.EDGE;
        int noEdge = OZ_PdaVppMetal.ACCENT;
        if (m_PdaHwWritable)
        {
            yesBg = OZ_PdaVppMetal.RAISED;
            noBg = OZ_PdaVppMetal.PANEL;
            yesInk = OZ_PdaVppMetal.TEXT;
            noInk = OZ_PdaVppMetal.MUTED;
            yesEdge = OZ_PdaVppMetal.ACCENT;
            noEdge = OZ_PdaVppMetal.EDGE;
        }
        PdaColor("BtnHwWrYesBg", yesBg);
        PdaColor("BtnHwWrNoBg", noBg);
        PdaColor("BtnHwWrYesText", yesInk);
        PdaColor("BtnHwWrNoText", noInk);
        PdaColor("BtnHwWrYesEdge", yesEdge);
        PdaColor("BtnHwWrNoEdge", noEdge);
    }

    // Under the name for players: what the game makes of it -- a stringtable key read in
    // the admin's language, a plain name as it is.
    private void PdaHwPaintGameNames()
    {
        PdaHwGameName("HwName", "HwNameGame");
        PdaHwGameName("HwCarName", "HwCarNameGame");
    }

    private void PdaHwGameName(string box, string lineName)
    {
        string typed = PdaTrimmed(box);
        if (typed == "")
        {
            PdaText(lineName, "", OZ_PdaVppMetal.MUTED);
            return;
        }
        string word = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_IN_GAME");
        string shown = typed;
        if (typed.IndexOf("#") == 0)
            shown = Widget.TranslateString(typed);
        PdaText(lineName, word + " " + shown, OZ_PdaVppMetal.MUTED);
    }

    // The kind steps through the three the PDA knows; a mod's kind is typed in the box.
    private void PdaHwNextKind()
    {
        string now = PdaTrimmed("HwKind");
        string nextKind = OZ_PdaConst.MOD_GPS;
        if (now == OZ_PdaConst.MOD_GPS)
            nextKind = OZ_PdaConst.MOD_SPY;
        else if (now == OZ_PdaConst.MOD_SPY)
            nextKind = OZ_PdaConst.MOD_DECRYPTOR;
        SetEdit("HwKind", nextKind);
    }

    private string PdaHwFormClass()
    {
        if (m_PdaHwKind == "carrier")
            return PdaTrimmed("HwCarClass");
        return PdaTrimmed("HwClass");
    }

    private void PdaHwAskSave()
    {
        if (!m_PdaHwCfg)
        {
            Hint("#STR_OZ_PDA_VPP_H_NOT_LOADED");
            return;
        }
        if (m_PdaHwKind == "")
        {
            Hint("#STR_OZ_PDA_VPP_H_PICK");
            return;
        }
        string cls = PdaHwFormClass();
        if (cls == "" || cls.IndexOf(" ") != -1)
        {
            Hint("#STR_OZ_PDA_VPP_H_CLASS_WORD");
            return;
        }
        string ask = Widget.TranslateString("#STR_OZ_PDA_VPP_ASK_HW_SAVE");
        Confirm(ask + " " + cls + "?", "#STR_OZ_PDA_VPP_YES_SAVE", "pda:hwsave");
    }

    // The strip's "yes": the boxes as they are now go into the entry -- a new one joins
    // its list -- and the whole file goes to the server.
    private void PdaHwSaveNow()
    {
        if (!m_PdaHwCfg || m_PdaHwKind == "")
        {
            Hint("#STR_OZ_PDA_VPP_H_PICK");
            return;
        }
        string cls = PdaHwFormClass();
        if (cls == "" || cls.IndexOf(" ") != -1)
        {
            Hint("#STR_OZ_PDA_VPP_H_CLASS_WORD");
            return;
        }

        if (m_PdaHwKind == "module")
        {
            OZ_ModuleSpec ms;
            if (m_PdaHwNew)
            {
                ms = new OZ_ModuleSpec();
                ms.EnablesPages = new array<string>();
                m_PdaHwCfg.Modules.Insert(ms);
                m_PdaHwIdx = m_PdaHwCfg.Modules.Count() - 1;
                m_PdaHwNew = false;
            }
            else
            {
                if (m_PdaHwIdx < 0 || m_PdaHwIdx >= m_PdaHwCfg.Modules.Count())
                {
                    Hint("#STR_OZ_PDA_VPP_H_GONE");
                    return;
                }
                ms = m_PdaHwCfg.Modules[m_PdaHwIdx];
            }

            ms.ClassName   = cls;
            ms.DisplayName = PdaTrimmed("HwName");
            ms.Kind        = PdaTrimmed("HwKind");
            ms.RangeM      = PdaTrimmed("HwRange").ToFloat();
            ms.PowerFactor = PdaTrimmed("HwPower").ToFloat();
            ms.SpyMinutes  = PdaTrimmed("HwSpy").ToFloat();
            ms.EnablesPages = PdaSplitPages(GetEdit("HwPages"));

            // THE ADMIN'S EDIT TAKES THE ENTRY OVER. An entry with an Origin belongs to
            // a mod, and every load lays the mod's declaration over it again
            // (OZ_PdaHardware.Insert touches mod entries only) -- the apply is a load,
            // so saving it unchanged would be undone a moment later. An empty Origin is
            // the admin's entry.
            if (ms.Origin != "")
                ms.Origin = "";
        }
        else
        {
            OZ_CarrierSpec cs;
            if (m_PdaHwNew)
            {
                cs = new OZ_CarrierSpec();
                m_PdaHwCfg.Carriers.Insert(cs);
                m_PdaHwIdx = m_PdaHwCfg.Carriers.Count() - 1;
                m_PdaHwNew = false;
            }
            else
            {
                if (m_PdaHwIdx < 0 || m_PdaHwIdx >= m_PdaHwCfg.Carriers.Count())
                {
                    Hint("#STR_OZ_PDA_VPP_H_GONE");
                    return;
                }
                cs = m_PdaHwCfg.Carriers[m_PdaHwIdx];
            }

            cs.ClassName   = cls;
            cs.DisplayName = PdaTrimmed("HwCarName");
            cs.Writable    = m_PdaHwWritable;
            cs.MaxRecords  = PdaTrimmed("HwMarks").ToInt();
        }

        m_PdaHwClass = cls;
        PdaHwPush();
    }

    private void PdaHwAskDelete()
    {
        if (!m_PdaHwCfg || m_PdaHwKind == "")
        {
            Hint("#STR_OZ_PDA_VPP_H_PICK");
            return;
        }

        // Nothing of a new entry is in the file yet: the card just closes.
        if (m_PdaHwNew)
        {
            PdaHwClear();
            return;
        }

        // A MOD'S MODULE IS NOT DELETED -- IT COMES BACK. The mod declares it with every
        // load of the file, and the apply of a delete is such a load: the row vanished
        // and returned a moment later. It is switched off by an edit instead: saving
        // makes the entry the admin's, and the mod no longer touches it.
        OZ_ModuleSpec declared = PdaHwPickedModule();
        if (declared && declared.Origin != "")
        {
            Hint("#STR_OZ_PDA_VPP_H_MOD_DECLARED");
            return;
        }

        string askKey = "#STR_OZ_PDA_VPP_ASK_HW_DEL_CAR";
        if (m_PdaHwKind == "module")
            askKey = "#STR_OZ_PDA_VPP_ASK_HW_DEL_MOD";
        string ask = Widget.TranslateString(askKey);
        string tail = Widget.TranslateString("#STR_OZ_PDA_VPP_ASK_FROM_HW");
        string text = ask + " " + m_PdaHwClass;
        text = text + " " + tail;
        string action = "pda:hwdel:" + m_PdaHwKind;
        action = action + ":" + m_PdaHwClass;
        Confirm(text, "#STR_OZ_PDA_VPP_YES_DEL", action);
    }

    // The strip's "yes" for "<kind>:<class>". The strip may stay open while the admin
    // works on, so the entry is found again by class, and a gone one is said, not
    // replaced by whatever now stands at its index.
    private void PdaHwDeleteNow(string what)
    {
        int colon = what.IndexOf(":");
        if (colon < 0 || !m_PdaHwCfg)
            return;
        string kind = what.Substring(0, colon);
        string cls = what.Substring(colon + 1, what.Length() - colon - 1);

        int at = PdaHwFind(kind, cls);
        if (at < 0)
        {
            Hint("#STR_OZ_PDA_VPP_H_GONE");
            return;
        }

        // RemoveOrdered: Remove swaps with the last, and the file an admin reads by eye
        // would reshuffle with every delete.
        if (kind == "module")
            m_PdaHwCfg.Modules.RemoveOrdered(at);
        else
            m_PdaHwCfg.Carriers.RemoveOrdered(at);

        if (m_PdaHwKind == kind && m_PdaHwClass == cls)
        {
            m_PdaHwKind  = "";
            m_PdaHwIdx   = -1;
            m_PdaHwClass = "";
            m_PdaHwNew   = false;
        }
        else if (m_PdaHwKind == kind && !m_PdaHwNew)
        {
            m_PdaHwIdx = PdaHwFind(m_PdaHwKind, m_PdaHwClass);
        }
        PdaHwPush();
    }

    private void PdaHwPush()
    {
        string body;
        string err;
        if (!JsonFileLoader<OZ_PdaHardwareConfig>.MakeData(m_PdaHwCfg, body, err, false))
        {
            Hint("#STR_OZ_PDA_VPP_H_NO_JSON");
            return;
        }
        SendCfg("Hardware", body);
        PdaHwRebuildList();
        PdaHwSelectPicked();
        PdaHwPaintCard();
    }

    // ================================================= Профілі

    private void PdaAskProfiles()
    {
        m_PdaProfAsked = true;
        m_PdaProfWaiting = true;
        AskCfg("Profiles");
    }

    private void PdaOnProfilesText(string body)
    {
        if (!m_PdaProfWaiting)
            return;
        m_PdaProfWaiting = false;

        MultilineEditBoxWidget ed = MultilineEditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("PdaProfEdit"));
        if (!ed)
            return;
        ed.SetText(body);

        // THE EDITOR MEASURES ITSELF instead of trusting that it holds the file. An edit
        // box keeps a bounded number of bytes (dayz-modding gui-layouts §22: a 1371-byte
        // paste stayed 512), and a live server's Profiles.json is several thousand: apply
        // sent back what the box KEPT, a stump. Now less than the file turns apply off
        // until the next read, and the admin edits the file on disk or in Config files.
        string got;
        ed.GetText(got);
        bool cut = false;
        if (got.Length() < body.Length())
            cut = true;
        m_PdaProfCut = cut;

        if (CurrentPane() != PDA_PAGE_PROF)
            return;
        if (!cut)
        {
            Hint("#STR_OZ_PDA_VPP_H_PROF_LOADED");
            return;
        }
        string tooBig = Widget.TranslateString("#STR_OZ_PDA_VPP_H_PROF_CUT");
        string sizes = body.Length().ToString() + " / ";
        sizes = sizes + got.Length().ToString() + " b";
        Hint(tooBig + " (" + sizes + ")");
    }

    private void PdaProfAskApply()
    {
        if (m_PdaProfCut)
        {
            Hint("#STR_OZ_PDA_VPP_H_PROF_CUT");
            return;
        }
        Confirm("#STR_OZ_PDA_VPP_ASK_PROF", "#STR_OZ_VPP_YES_APPLY", "pda:prof");
    }

    private void PdaProfApplyNow()
    {
        if (m_PdaProfCut)
        {
            Hint("#STR_OZ_PDA_VPP_H_PROF_CUT");
            return;
        }
        MultilineEditBoxWidget ed = MultilineEditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("PdaProfEdit"));
        if (!ed)
            return;
        string body;
        ed.GetText(body);
        string trimmed = body.Trim();
        if (trimmed == "")
        {
            Hint("#STR_OZ_PDA_VPP_H_NOT_LOADED");
            return;
        }
        SendCfg("Profiles", PdaDropWraps(body));
    }

    // A LINE BREAK INSIDE A STRING VALUE IS THE EDITOR'S WRAP. A box with `lines` wraps
    // what is typed by INSERTING a real \n (dayz-modding gui-layouts §22), and in
    // Profiles.json it fell in the middle of a page id or a class name: "contac\nts" is
    // another page that no device finds. A raw break inside a JSON string is not allowed,
    // and between tokens it is plain whitespace, so only those in quotes go. Cut at \n
    // and \r (ASCII): every piece stays whole UTF-8.
    private string PdaDropWraps(string text)
    {
        string outp = "";
        int n = text.Length();
        int from = 0;
        bool inStr = false;
        bool esc = false;

        for (int i = 0; i < n; i++)
        {
            string ch = text.Substring(i, 1);
            if (esc)
            {
                esc = false;
                continue;
            }
            if (inStr && ch == "\\")
            {
                esc = true;
                continue;
            }
            if (ch == "\"")
            {
                inStr = !inStr;
                continue;
            }
            if (inStr && (ch == "\n" || ch == "\r"))
            {
                if (i > from)
                    outp += text.Substring(from, i - from);
                from = i + 1;
            }
        }

        if (from < n)
            outp += text.Substring(from, n - from);
        return outp;
    }

    // ================================================= Ядро › Файли конфігів

    // The core's files page, opened on one of the PDA's files: the raw text, paste and
    // copy, the server's apply. RawPick is the page's own pick: it empties the editor,
    // marks the row and asks for the file.
    private void PdaOpenFile(string name)
    {
        if (!m_Panes.Contains("raw"))
            return;
        ShowPane("raw");
        RawPick(name);
    }

    // ================================================= шматки

    private string PdaTrimmed(string name)
    {
        string t = GetEdit(name);
        return t.Trim();
    }

    // A whole number as an admin types it: digits, a minus in front.
    private bool PdaIsInt(string s)
    {
        int n = s.Length();
        if (n == 0)
            return false;
        string digits = "0123456789";
        for (int i = 0; i < n; i++)
        {
            string ch = s.Substring(i, 1);
            if (i == 0 && ch == "-" && n > 1)
                continue;
            if (digits.IndexOf(ch) == -1)
                return false;
        }
        return true;
    }

    private string PdaJoinPages(array<string> pages)
    {
        if (!pages)
            return "";
        string outp = "";
        for (int i = 0; i < pages.Count(); i++)
        {
            if (outp != "")
                outp += " ";
            outp += pages[i];
        }
        return outp;
    }

    private ref array<string> PdaSplitPages(string text)
    {
        array<string> outp = new array<string>();
        array<string> parts = new array<string>();
        text.Split(" ", parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            if (parts[i] != "")
                outp.Insert(parts[i]);
        }
        return outp;
    }

    private void PdaText(string name, string text, int ink)
    {
        TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget(name));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(ink);
    }

    private void PdaColor(string name, int color)
    {
        Widget w = M_SUB_WIDGET.FindAnyWidget(name);
        if (w)
            w.SetColor(color);
    }

    private void PdaShow(string name, bool show)
    {
        Widget w = M_SUB_WIDGET.FindAnyWidget(name);
        if (w)
            w.Show(show);
    }

    private void PdaSize(string name, float w, float h)
    {
        Widget x = M_SUB_WIDGET.FindAnyWidget(name);
        if (x)
            x.SetSize(w, h);
    }

    private void PdaMove(string name, float x, float y)
    {
        Widget m = M_SUB_WIDGET.FindAnyWidget(name);
        if (m)
            m.SetPos(x, y);
    }

    // A generator field is three widgets: <Name>Frame (the border), <Name>Fill one unit
    // in, and the edit box itself six units in (four from the top for a multiline one).
    private void PdaSizeField(string name, float w, float h)
    {
        float s = UnitScale();
        PdaSize(name + "Frame", w, h);
        PdaSize(name + "Fill", w - 2 * s, h - 2 * s);
        PdaSize(name, w - 12 * s, h - 8 * s);
    }

    // ================================================= розмір вікна

    // The window changed size (OZ_VppAdminMenu.ApplyWindowSize): every page is now w x h
    // pixels, the content area. The generator put everything in exact places, so the
    // heads and the bars are widened here (the actions anchored to their right edge
    // follow), and the hardware list and the profiles editor take the height. NOTHING
    // IS READ BACK: before the engine's first layout pass a widget reads 0 x 0
    // (dayz-modding gui-layouts §23a); every size comes from w, h and the layouts'
    // own numbers.
    override void OnWindowResized(float w, float h)
    {
        super.OnWindowResized(w, h);
        if (!M_SUB_WIDGET)
            return;

        float s = UnitScale();
        float head = 60 * s;

        PdaSize("PdaTunHead", w, head);
        PdaSize("PdaTunHeadRule", w, s);
        PdaSize("PdaTunFoot", w, head);
        PdaSize("PdaTunFootRule", w, s);

        PdaSize("PdaHwHead", w, head);
        PdaSize("PdaHwHeadRule", w, s);
        PdaSize("PdaHwDivider", s, h - head);
        float listH = h - 210 * s;
        if (listH < 60 * s)
            listH = 60 * s;
        PdaSize("HwList", 392 * s, listH);
        PdaMove("PdaHwListNote", 24 * s, listH + 110 * s);

        PdaSize("PdaProfHead", w, head);
        PdaSize("PdaProfHeadRule", w, s);
        PdaSize("PdaProfFoot", w, head);
        PdaSize("PdaProfFootRule", w, s);
        float edW = w - 48 * s;
        float edH = h - 152 * s;
        if (edW < 200 * s)
            edW = 200 * s;
        if (edH < 100 * s)
            edH = 100 * s;
        PdaSizeField("PdaProfEdit", edW, edH);
    }

    // ================================================= ввід

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && w.GetName() == "HwList")
        {
            if (!m_PdaRepaint)
                PdaHwPick(row);
            return true;
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_PdaFill)
        {
            string nm = w.GetName();
            if (nm.IndexOf("Tun_") == 0)
            {
                PdaTunPaint();
                return true;
            }
            if (nm == "HwName" || nm == "HwCarName")
            {
                PdaHwPaintGameNames();
                return true;
            }
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w || !M_SUB_WIDGET)
            return super.OnClick(w, x, y, button);

        string nm = w.GetName();

        if (nm == "BtnPdaTunSave")
        {
            PdaTunAskSave();
            return true;
        }
        if (nm == "BtnPdaTunDrop")
        {
            PdaTunAskDrop();
            return true;
        }
        if (nm == "BtnPdaTunOpen")
        {
            PdaOpenFile("Tuning");
            return true;
        }

        if (nm == "BtnHwNewMod")
        {
            PdaHwNewForm("module");
            return true;
        }
        if (nm == "BtnHwNewCar")
        {
            PdaHwNewForm("carrier");
            return true;
        }
        if (nm == "BtnHwKind")
        {
            PdaHwNextKind();
            return true;
        }
        if (nm == "BtnHwWrYes" || nm == "BtnHwWrNo")
        {
            bool writes = nm == "BtnHwWrYes";
            m_PdaHwWritable = writes;
            PdaHwPaintWritable();
            return true;
        }
        if (nm == "BtnHwSave")
        {
            PdaHwAskSave();
            return true;
        }
        if (nm == "BtnHwDel")
        {
            PdaHwAskDelete();
            return true;
        }

        if (nm == "BtnPdaProfReload")
        {
            PdaAskProfiles();
            return true;
        }
        if (nm == "BtnPdaProfApply")
        {
            PdaProfAskApply();
            return true;
        }
        if (nm == "BtnPdaProfOpen")
        {
            PdaOpenFile("Profiles");
            return true;
        }

        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
