// Модуль «КПК» в адмінському вікні OpenZone (VPP rework 2026-11-12, design 5.3): три
// сторінки модуля "pda" на рейці вікна -- Налаштування (Tuning.json формою), Обладнання
// (Hardware.json: модулі й носії) і Профілі (сирий Profiles.json, доки його не візьме
// Ядро › Файли конфігів). Ядро про КПК не знає, КПК докладає свої сторінки сам цим
// modded-шаром.
//
// The three layouts are converted from the mockup's boards (css2layout, 2026-11-13;
// docs/specs/2026-11-13-openzone-vpp-page-guide.md): PdaTuning.dc.html, PdaHardware.dc.html
// and PdaProfiles.html under docs/specs/2026-11-12-openzone-vpp-mockup/project. They carry
// their own colours and follow the window by themselves; what this file names is in the
// reports ui/css2layout/oz_pda_vpp_*.txt. The list of modules and carriers is rows made
// from two templates (a group's caption, an entry).
//
// Усе, що йде на сервер або викидає роботу адміна, питає на смузі підтвердження вікна
// (design 3.4): зберегти налаштування чи запис, відкинути незбережені налаштування,
// видалити запис, застосувати сирі профілі. «Так» вертається в OnConfirmed дією "pda:".
// What a page says goes to the window's journal and its status bar (Hint).
//
// Гарди: NO_GUI -- сервер компілює Mission без UI; AVPPAdminTools і OpenZone_VPP --
// імена класів CfgMods (їх авто-дефайнить рушій).
//
// Члени цього шару починаються з Pda (поля -- m_Pda): той самий клас модифікує кожна
// склейка, і ім'я, яке мають двоє, не компілюється. Імена віджетів теж унікальні на все
// вікно. Fields are declared bare and set in OnCreate (dayz-modding enforce-script.md,
// field initialisers).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string PDA_PAGE_TUN  = "pda.tuning";
    static const string PDA_PAGE_HW   = "pda.hardware";
    static const string PDA_PAGE_PROF = "pda.profiles";

    // Numbers of the converted layouts, in layout units. The engine does not read the
    // boards, so they live twice: convert a page again, check these against its layout.
    // BAR: the bar at the bottom of Налаштування and Профілі (the confirm strip stands
    // over it). HW_*: the card of Обладнання inside PdaHwCard -- where a box stands, how
    // tall the module's and the carrier's are, the room under a box, where the title
    // starts. LEG_*: a caption over a box's border -- its room left and right of the
    // words, the height of its ground and of its line. Rows of the list: 407 wide, a
    // group's caption 24 high, an entry 52.
    static const float PDA_BAR_H      = 61;
    static const float PDA_HW_X       = 24;
    static const float PDA_HW_BOX_Y   = 57;
    static const float PDA_HW_MOD_H   = 306;
    static const float PDA_HW_CAR_H   = 204;
    static const float PDA_HW_GAP     = 14;
    static const float PDA_HW_TITLE_Y = 16;
    static const float PDA_HW_TITLE_H = 27;
    static const float PDA_HW_BADGE_Y = 21;
    static const float PDA_HW_BADGE_H = 18;
    static const float PDA_LEG_PAD    = 6;
    static const float PDA_LEG_BAR_H  = 8;
    static const float PDA_LEG_H      = 15;
    static const float PDA_ROW_W      = 407;
    static const float PDA_HEAD_H     = 24;
    static const float PDA_ROW_H      = 52;

    // A group of the list shows this many entries; its caption says when there are more.
    static const int PDA_HW_ROWS_MAX = 150;

    // The three panes, as created.
    private Widget m_PdaTunPane;
    private Widget m_PdaHwPane;
    private Widget m_PdaProfPane;

    // What is fitted to its words waits for the page to be on screen: frames left to try.
    private int m_PdaFitTries;

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
    private bool m_PdaFill;

    // ------------------------------------------------- Обладнання
    private ref OZ_PdaHardwareConfig m_PdaHwCfg;
    // The list: the captions of the two groups and the entries. An entry's key is
    // "<kind>:<index>", kind "module" or "carrier".
    private ref OZ_VppFlexRows m_PdaHwHeads;
    private ref OZ_VppFlexRows m_PdaHwRows;
    // The entry on the card: its kind, its index, its class as the file has it, and
    // whether it is new (not in the file yet).
    private string m_PdaHwKind;
    private int    m_PdaHwIdx;
    private string m_PdaHwClass;
    private bool   m_PdaHwNew;
    private bool   m_PdaHwWritable;

    // ------------------------------------------------- Профілі
    // The editor holds less than the whole file: applying would cut it.
    private bool m_PdaProfCut;
    // The page asked for the file: the answer goes into the editor. Ядро › Файли
    // конфігів asks for the same file, and its answer leaves this editor alone.
    private bool m_PdaProfWaiting;
    private bool m_PdaProfAsked;
    // The file's size and what the editor kept of it, in bytes; -1: no file came yet.
    private int  m_PdaProfBytes;
    private int  m_PdaProfKept;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;

        m_PdaFitTries = 0;
        m_PdaFill = false;
        m_PdaHwKind = "";
        m_PdaHwIdx = -1;
        m_PdaHwClass = "";
        m_PdaHwNew = false;
        m_PdaHwWritable = true;
        m_PdaProfCut = false;
        m_PdaProfWaiting = false;
        m_PdaProfAsked = false;
        m_PdaProfBytes = -1;
        m_PdaProfKept = 0;

        m_PdaTunKeys   = new array<string>();
        m_PdaTunLabels = new array<string>();
        m_PdaTunGroups = new array<string>();
        PdaTunTable();

        m_PdaTunDef = new OZ_PdaTuning();
        m_PdaTunDef.LoadDefaults();

        m_PdaTunPane  = PdaAddPage(PDA_PAGE_TUN,  "#STR_OZ_PDA_VPP_PAGE_TUN",  "oz_pda_vpp_tuning",   "PdaTunPane");
        m_PdaHwPane   = PdaAddPage(PDA_PAGE_HW,   "#STR_OZ_PDA_VPP_PAGE_HW",   "oz_pda_vpp_hardware", "PdaHwPane");
        m_PdaProfPane = PdaAddPage(PDA_PAGE_PROF, "#STR_OZ_PDA_VPP_PAGE_PROF", "oz_pda_vpp_profiles", "PdaProfPane");

        // The list of Обладнання: two row templates into one spacer, in the order made.
        if (m_PdaHwPane)
        {
            Widget hwHost = m_PdaHwPane.FindAnyWidget("PdaHwListRows");
            if (hwHost)
            {
                m_PdaHwHeads = new OZ_VppFlexRows(m_Flex, hwHost, "OpenZone_PDA_VPP/gui/layouts/oz_pda_vpp_hw_head.layout", "OZ_PdaVppHwHead", PDA_ROW_W, PDA_HEAD_H);
                m_PdaHwRows  = new OZ_VppFlexRows(m_Flex, hwHost, "OpenZone_PDA_VPP/gui/layouts/oz_pda_vpp_hw_row.layout", "OZ_PdaVppHwRow", PDA_ROW_W, PDA_ROW_H);
            }
            else
                OZ_Log.Error("pda vpp page: the hardware list has no PdaHwListRows");
        }

        // The captions and defaults of the form, the empty card and the editor's line,
        // before any file arrives.
        PdaTunCaptions();
        PdaTunPaint();
        PdaHwPaintCard();
        PdaProfPaintState();
    }

    // A page of the module: its layout under the window's root, registered on the rail.
    // No hint label of its own: a page's lines go to the journal, and the status bar
    // shows the newest.
    private Widget PdaAddPage(string id, string label, string layout, string rootName)
    {
        string path = "OpenZone_PDA_VPP/gui/layouts/" + layout + ".layout";
        Widget pane = OZ_VppFlex.Create(path, M_SUB_WIDGET, rootName);
        if (!pane)
        {
            OZ_Log.Error("pda vpp page: layout failed to load: " + path);
            return null;
        }
        RegisterPage("pda", "#STR_OZ_PDA_VPP_MOD", id, label, pane, "");
        return pane;
    }

    // A page of the module is shown: its file, unless the page holds the admin's work.
    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);

        if (id == PDA_PAGE_TUN)
        {
            m_PdaFitTries = 30;
            array<int> changed = new array<int>();
            PdaTunChanged(changed);
            if (changed.Count() == 0)
                AskCfg("Tuning");
            return;
        }

        if (id == PDA_PAGE_HW)
        {
            m_PdaFitTries = 30;
            PdaHwPlace();
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

    // Налаштування and Профілі have a bar of their own at the bottom: the confirm strip
    // stands over it, not on it.
    override float ConfirmLift()
    {
        if (m_Shown == PDA_PAGE_TUN || m_Shown == PDA_PAGE_PROF)
            return PDA_BAR_H;
        return super.ConfirmLift();
    }

    // Every frame while the window is open: what waits to be fitted to its words.
    override void Tick()
    {
        super.Tick();
        if (m_PdaFitTries > 0)
            PdaFitTick();
    }

    // The captions over the boxes' borders and the card's title are as wide as their
    // words in the game's language. A text measures only once it is on screen, so the
    // open page is tried for a few frames after it is shown.
    private void PdaFitTick()
    {
        m_PdaFitTries = m_PdaFitTries - 1;

        bool done = true;
        if (m_Shown == PDA_PAGE_TUN)
        {
            for (int g = 0; g < 6; g++)
            {
                if (!PdaFitLegend(m_PdaTunPane, "TunLeg" + g.ToString()))
                    done = false;
            }
        }
        else if (m_Shown == PDA_PAGE_HW)
        {
            if (!PdaFitLegend(m_PdaHwPane, "PdaHwModLeg"))
                done = false;
            if (!PdaFitLegend(m_PdaHwPane, "PdaHwCarLeg"))
                done = false;
            if (!PdaHwFlowTitle())
                done = false;
        }

        if (done)
            m_PdaFitTries = 0;
    }

    // One caption over a border: its ground (<name>Bar hides the border under the words)
    // and its line (<name>Text) as wide as the words. False: the words do not measure
    // yet. A caption that is hidden with its box is left for the time it shows.
    private bool PdaFitLegend(Widget pane, string name)
    {
        if (!pane)
            return true;
        Widget bar = pane.FindAnyWidget(name + "Bar");
        TextWidget t = TextWidget.Cast(pane.FindAnyWidget(name + "Text"));
        if (!bar || !t)
            return true;
        if (!t.IsVisibleHierarchy())
            return true;

        int tw = OZ_VppFlex.TextWidth(t);
        if (tw <= 0)
            return false;

        float s = UnitScale();
        bar.SetSize(tw + 2 * PDA_LEG_PAD * s, PDA_LEG_BAR_H * s);
        t.SetSize(tw + 2, PDA_LEG_H * s);
        return true;
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

    // The named fields, one after another; `most` > 0 names that many and counts the rest.
    private string PdaTunList(array<int> idx, bool withValues, int most)
    {
        string list = "";
        int named = idx.Count();
        if (most > 0 && named > most)
            named = most;
        for (int i = 0; i < named; i++)
        {
            string item = PdaTunItem(idx[i], withValues);
            if (list != "")
                list = list + "; ";
            list = list + item;
        }
        int rest = idx.Count() - named;
        if (rest > 0)
            list = list + "; +" + rest.ToString();
        return list;
    }

    // The captions of the fields: the layout leaves them to the script, because three
    // fields share their Ukrainian caption and differ in English.
    private void PdaTunCaptions()
    {
        if (!m_PdaTunPane)
            return;
        int cInk = ARGB(255, 224, 223, 218);
        for (int i = 0; i < m_PdaTunKeys.Count(); i++)
            PdaPaneText(m_PdaTunPane, "TunL_" + m_PdaTunKeys[i], m_PdaTunLabels[i], cInk);
    }

    // Under each caption the field's default, in the alert colour with "changed" while
    // the box differs from the file, and the box's frame with it; the bar names what is
    // not saved yet.
    private void PdaTunPaint()
    {
        if (!m_PdaTunPane || !m_PdaTunKeys)
            return;

        int cMuted = ARGB(255, 160, 159, 153);
        int cAlert = ARGB(255, 245, 199, 64);
        int cEdge = ARGB(255, 60, 60, 58);

        string defWord = Widget.TranslateString("#STR_OZ_PDA_VPP_DEFAULT");
        string changedWord = Widget.TranslateString("#STR_OZ_PDA_VPP_CHANGED");
        string homeDef = Widget.TranslateString("#STR_OZ_PDA_VPP_DEF_HOME");
        string dayWord = Widget.TranslateString("#STR_OZ_PDA_VPP_DEF_DAY");
        array<int> changed = new array<int>();

        string key;
        string line;
        bool dirty;
        int ink;
        int edge;
        for (int i = 0; i < m_PdaTunKeys.Count(); i++)
        {
            key = m_PdaTunKeys[i];
            dirty = false;
            if (m_PdaTun)
                dirty = PdaTunFieldChanged(i);

            line = defWord + " " + PdaTunText(m_PdaTunDef, key);
            if (key == "MapHome")
                line = homeDef;
            // A day in seconds reads better with the word beside it.
            if (key == "GroupInviteTtlSeconds" && m_PdaTunDef.GroupInviteTtlSeconds == 86400)
                line = line + " — " + dayWord;

            ink = cMuted;
            edge = cEdge;
            if (dirty)
            {
                line = line + " · " + changedWord;
                ink = cAlert;
                edge = cAlert;
                changed.Insert(i);
            }
            PdaPaneText(m_PdaTunPane, "TunD_" + key, line, ink);
            OZ_VppFlex.Paint(m_PdaTunPane, "TunBox_" + key + "Edge", edge);
        }

        if (changed.Count() == 0)
        {
            PdaPaneText(m_PdaTunPane, "PdaTunDirty", "#STR_OZ_PDA_VPP_NO_CHANGES", cMuted);
            return;
        }
        string unsaved = Widget.TranslateString("#STR_OZ_PDA_VPP_UNSAVED");
        string list = PdaTunList(changed, false, 4);
        PdaPaneText(m_PdaTunPane, "PdaTunDirty", unsaved + " " + list, cAlert);
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
        string list = PdaTunList(changed, true, 0);
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
        string list = PdaTunList(changed, true, 0);
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

    // The list: the modules under their caption, then the carriers under theirs. An
    // entry shows its class, the name the game makes of it and a summary.
    private void PdaHwRebuildList()
    {
        if (!m_PdaHwHeads || !m_PdaHwRows || !m_PdaHwCfg)
            return;

        m_PdaHwHeads.Clear();
        m_PdaHwRows.Clear();

        string modWord = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_HEAD_MOD");
        string carWord = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_HEAD_CAR");
        Widget row;
        // Every translated piece goes into a local before it meets another (two
        // translations in one expression lose one: dayz-modding mod-structure.md).
        string shownName;
        string sum;

        int mods = m_PdaHwCfg.Modules.Count();
        PdaHwHeadRow(modWord, mods);
        for (int m = 0; m < mods && m < PDA_HW_ROWS_MAX; m++)
        {
            OZ_ModuleSpec ms = m_PdaHwCfg.Modules[m];
            row = m_PdaHwRows.Add("module:" + m.ToString());
            if (!row)
                continue;
            shownName = PdaHwShownName(ms.DisplayName);
            sum = PdaHwModSum(ms);
            PdaHwRowFill(row, "M", ms.ClassName, shownName, sum);
        }

        int cars = m_PdaHwCfg.Carriers.Count();
        PdaHwHeadRow(carWord, cars);
        for (int c = 0; c < cars && c < PDA_HW_ROWS_MAX; c++)
        {
            OZ_CarrierSpec cs = m_PdaHwCfg.Carriers[c];
            row = m_PdaHwRows.Add("carrier:" + c.ToString());
            if (!row)
                continue;
            shownName = PdaHwShownName(cs.DisplayName);
            sum = PdaHwCarSum(cs);
            PdaHwRowFill(row, "C", cs.ClassName, shownName, sum);
        }

        m_PdaHwHeads.Fit();
        m_PdaHwRows.Fit();
        PdaHwMarkRows();
    }

    // A group's caption: "МОДУЛІ · 3", and how many of them the list shows when it is cut.
    private void PdaHwHeadRow(string word, int count)
    {
        Widget head = m_PdaHwHeads.Add("head");
        if (!head)
            return;
        string text = word + " · " + count.ToString();
        int most = PDA_HW_ROWS_MAX;
        if (count > most)
        {
            string shown = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_SHOWN");
            text = text + ", " + shown;
            text = text + " " + most.ToString();
        }
        PdaRowText(head, "HwHeadText", text, ARGB(255, 160, 159, 153));
    }

    private void PdaHwRowFill(Widget row, string mark, string cls, string shown, string sum)
    {
        int cInk = ARGB(255, 224, 223, 218);
        int cMuted = ARGB(255, 160, 159, 153);
        int cAccent = ARGB(255, 232, 120, 43);
        PdaRowText(row, "HwRowMark", mark, cAccent);
        PdaRowText(row, "HwRowCls", cls, cInk);
        PdaRowText(row, "HwRowName", shown, cMuted);
        PdaRowText(row, "HwRowSum", sum, cMuted);
    }

    // The picked entry's row is lit (the accent's frame and soft ground, as the template
    // carries them); the others lie on the list's plain ground. None is lit for a new
    // entry or an empty card.
    private void PdaHwMarkRows()
    {
        if (!m_PdaHwRows)
            return;

        int cAccent = ARGB(255, 232, 120, 43);
        int cPicked = ARGB(255, 47, 29, 17);
        int cHead = ARGB(255, 19, 19, 18);

        string pick = "";
        if (!m_PdaHwNew && m_PdaHwKind != "")
            pick = m_PdaHwKind + ":" + m_PdaHwIdx.ToString();

        Widget row;
        for (int i = 0; i < m_PdaHwRows.Count(); i++)
        {
            row = m_PdaHwRows.Row(i);
            if (!row)
                continue;
            if (pick != "" && m_PdaHwRows.Key(i) == pick)
            {
                OZ_VppFlex.Paint(row, "OZ_PdaVppHwRowEdge", cAccent);
                OZ_VppFlex.Paint(row, "OZ_PdaVppHwRowBg", cPicked);
            }
            else
            {
                OZ_VppFlex.Paint(row, "OZ_PdaVppHwRowEdge", cHead);
                OZ_VppFlex.Paint(row, "OZ_PdaVppHwRowBg", cHead);
            }
        }
    }

    // A name for players as the game shows it: a stringtable key read in the admin's
    // language, a plain name as it is.
    private string PdaHwShownName(string name)
    {
        if (name.IndexOf("#") == 0)
            return Widget.TranslateString(name);
        return name;
    }

    // "gps · 5000 м · ×1.6": the kind, the range and the spy minutes when they are set,
    // the power factor.
    private string PdaHwModSum(OZ_ModuleSpec ms)
    {
        string sum = ms.Kind;
        if (sum == "")
            sum = "?";
        if (ms.RangeM > 0)
        {
            string meters = Widget.TranslateString("#STR_OZ_PDA_VPP_UNIT_M");
            sum = sum + " · " + ms.RangeM.ToString();
            sum = sum + " " + meters;
        }
        if (ms.SpyMinutes > 0)
        {
            string minutes = Widget.TranslateString("#STR_OZ_PDA_VPP_UNIT_MIN");
            sum = sum + " · " + ms.SpyMinutes.ToString();
            sum = sum + " " + minutes;
        }
        sum = sum + " · ×" + ms.PowerFactor.ToString();
        return sum;
    }

    // "6 записів", "без ліміту"; a carrier that cannot be written says so first.
    private string PdaHwCarSum(OZ_CarrierSpec cs)
    {
        string sum = "";
        if (!cs.Writable)
        {
            string readOnly = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_READ_ONLY");
            sum = readOnly + " · ";
        }
        if (cs.MaxRecords > 0)
        {
            string records = PdaHwRecordsWord(cs.MaxRecords);
            sum = sum + cs.MaxRecords.ToString();
            sum = sum + " " + records;
            return sum;
        }
        string noLimit = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_NO_LIMIT");
        sum = sum + noLimit;
        return sum;
    }

    // The word after a count of records, as Ukrainian counts: 1 запис, 21 запис, 2-4
    // записи, 5-20 записів. English keeps "record" for one alone.
    private string PdaHwRecordsWord(int n)
    {
        if (n == 1)
            return Widget.TranslateString("#STR_OZ_PDA_VPP_HW_RECORDS_ONE");

        int tens = n % 100;
        int ones = n % 10;
        string key = "#STR_OZ_PDA_VPP_HW_RECORDS_N";
        if (tens < 11 || tens > 14)
        {
            if (ones == 1)
                key = "#STR_OZ_PDA_VPP_HW_RECORDS_1";
            else if (ones >= 2 && ones <= 4)
                key = "#STR_OZ_PDA_VPP_HW_RECORDS_2";
        }
        return Widget.TranslateString(key);
    }

    // A row of the list was clicked: its entry on the card.
    private void PdaHwPickKey(string key)
    {
        if (!m_PdaHwCfg)
            return;
        int colon = key.IndexOf(":");
        if (colon < 1)
            return;
        string kind = key.Substring(0, colon);
        int idx = key.Substring(colon + 1, key.Length() - colon - 1).ToInt();

        if (kind == "module" && idx >= 0 && idx < m_PdaHwCfg.Modules.Count())
            PdaHwFill(kind, idx);
        else if (kind == "carrier" && idx >= 0 && idx < m_PdaHwCfg.Carriers.Count())
            PdaHwFill(kind, idx);
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
        PdaHwMarkRows();
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
        PdaHwMarkRows();
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
        PdaHwMarkRows();
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
            PdaHwMarkRows();
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
            return;
        }

        PdaHwMarkRows();
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

    // The card: the module's box or the carrier's, one at a time, with its title and
    // badge, save and delete under it and, beside them, the line that says another mod
    // declared the entry. With no entry the title asks to pick one.
    private void PdaHwPaintCard()
    {
        if (!m_PdaHwPane)
            return;

        int cInk = ARGB(255, 224, 223, 218);
        int cMuted = ARGB(255, 160, 159, 153);
        int cAlert = ARGB(255, 245, 199, 64);

        bool isMod = m_PdaHwKind == "module";
        bool isCar = m_PdaHwKind == "carrier";
        bool any = isMod || isCar;

        PdaShow("PdaHwCardBadge", any);
        PdaShow("PdaHwModBox", isMod);
        PdaShow("PdaHwCarBox", isCar);
        PdaShow("PdaHwActs", any);

        if (!any)
            PdaPaneText(m_PdaHwPane, "PdaHwCardTitle", "#STR_OZ_PDA_VPP_HW_EMPTY", cMuted);
        else if (m_PdaHwNew && isMod)
            PdaPaneText(m_PdaHwPane, "PdaHwCardTitle", "#STR_OZ_PDA_VPP_HW_NEW_MOD_TITLE", cInk);
        else if (m_PdaHwNew)
            PdaPaneText(m_PdaHwPane, "PdaHwCardTitle", "#STR_OZ_PDA_VPP_HW_NEW_CAR_TITLE", cInk);
        else
            PdaPaneText(m_PdaHwPane, "PdaHwCardTitle", m_PdaHwClass, cInk);

        if (isMod)
            PdaPaneText(m_PdaHwPane, "PdaHwCardBadgeText", "#STR_OZ_PDA_VPP_HW_G_MODULE", cMuted);
        else
            PdaPaneText(m_PdaHwPane, "PdaHwCardBadgeText", "#STR_OZ_PDA_VPP_HW_G_CARRIER", cMuted);

        // A module another mod declared comes back with every load of the file; saving
        // makes it the admin's (an empty Origin), deleting does not help (PdaHwAskDelete).
        string origin = "";
        OZ_ModuleSpec declared = PdaHwPickedModule();
        if (declared && declared.Origin != "")
            origin = "#STR_OZ_PDA_VPP_HW_ORIGIN";
        PdaPaneText(m_PdaHwPane, "PdaHwOrigin", origin, cAlert);

        PdaHwPlace();
        PdaHwPaintWritable();
        PdaHwPaintGameNames();
        PdaHwPaintKindNote();

        // The title and the badge follow the words; so do the captions of a box that was
        // hidden until now.
        m_PdaFitTries = 30;
    }

    // The carrier's box stands in the module's place (they show one at a time) and the
    // actions under the open box. The window's resize rule puts both back where the
    // layout has them, so this runs again after every resize.
    private void PdaHwPlace()
    {
        if (!m_PdaHwPane)
            return;

        float s = UnitScale();
        float under = PDA_HW_BOX_Y + PDA_HW_MOD_H + PDA_HW_GAP;
        if (m_PdaHwKind == "carrier")
            under = PDA_HW_BOX_Y + PDA_HW_CAR_H + PDA_HW_GAP;

        PdaMove("PdaHwCarBox", PDA_HW_X * s, PDA_HW_BOX_Y * s);
        PdaMove("PdaHwActs", PDA_HW_X * s, under * s);
        PdaHwFlowTitle();
    }

    // The badge hugs its word and stands 12 units after the title's last letter. False:
    // the words do not measure yet (the page is not on screen).
    private bool PdaHwFlowTitle()
    {
        if (!m_PdaHwPane)
            return true;
        TextWidget title = TextWidget.Cast(m_PdaHwPane.FindAnyWidget("PdaHwCardTitle"));
        Widget badge = m_PdaHwPane.FindAnyWidget("PdaHwCardBadge");
        TextWidget word = TextWidget.Cast(m_PdaHwPane.FindAnyWidget("PdaHwCardBadgeText"));
        if (!title || !badge || !word)
            return true;

        // The title's line is as wide as the card allows: its words are of any length.
        float s = UnitScale();
        title.SetSize(820 * s, PDA_HW_TITLE_H * s);
        if (!badge.IsVisible())
            return true;

        int titleW = OZ_VppFlex.TextWidth(title);
        int wordW = OZ_VppFlex.TextWidth(word);
        if (titleW <= 0 || wordW <= 0)
            return false;

        float badgeW = wordW + 16 * s;
        badge.SetPos(PDA_HW_X * s + titleW + 12 * s, PDA_HW_BADGE_Y * s);
        badge.SetSize(badgeW, PDA_HW_BADGE_H * s);
        Widget ground = m_PdaHwPane.FindAnyWidget("PdaHwCardBadgeBg");
        if (ground)
            ground.SetSize(badgeW, PDA_HW_BADGE_H * s);
        word.SetPos(8 * s, 2 * s);
        word.SetSize(wordW + 2, 14 * s);
        return true;
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
        if (!m_PdaHwPane)
            return;

        int cInk = ARGB(255, 224, 223, 218);
        int cMuted = ARGB(255, 160, 159, 153);
        int cLit = ARGB(255, 53, 35, 22);
        int cPlain = ARGB(255, 19, 19, 18);

        if (m_PdaHwWritable)
        {
            OZ_VppFlex.Paint(m_PdaHwPane, "BtnHwWrYesBg", cLit);
            OZ_VppFlex.Paint(m_PdaHwPane, "BtnHwWrNoBg", cPlain);
            PdaColor("BtnHwWrYesText", cInk);
            PdaColor("BtnHwWrNoText", cMuted);
            return;
        }
        OZ_VppFlex.Paint(m_PdaHwPane, "BtnHwWrYesBg", cPlain);
        OZ_VppFlex.Paint(m_PdaHwPane, "BtnHwWrNoBg", cLit);
        PdaColor("BtnHwWrYesText", cMuted);
        PdaColor("BtnHwWrNoText", cInk);
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
        int cMuted = ARGB(255, 160, 159, 153);
        string typed = PdaTrimmed(box);
        if (typed == "")
        {
            PdaText(lineName, "", cMuted);
            return;
        }
        string word = Widget.TranslateString("#STR_OZ_PDA_VPP_HW_IN_GAME");
        string shown = PdaHwShownName(typed);
        PdaText(lineName, word + " " + shown, cMuted);
    }

    // Under the pages a module opens: what its kind does, for the three kinds the PDA
    // knows, a kind of another mod and none at all.
    private void PdaHwPaintKindNote()
    {
        string kind = PdaTrimmed("HwKind");
        string note = "#STR_OZ_PDA_VPP_HW_K_OTHER";
        if (kind == "")
            note = "#STR_OZ_PDA_VPP_HW_K_NONE";
        else if (kind == OZ_PdaConst.MOD_GPS)
            note = "#STR_OZ_PDA_VPP_HW_K_GPS";
        else if (kind == OZ_PdaConst.MOD_SPY)
            note = "#STR_OZ_PDA_VPP_HW_K_SPY";
        else if (kind == OZ_PdaConst.MOD_DECRYPTOR)
            note = "#STR_OZ_PDA_VPP_HW_K_DEC";
        PdaText("PdaHwKindNote", note, ARGB(255, 160, 159, 153));
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
        PdaHwPaintKindNote();
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
        m_PdaProfBytes = body.Length();
        m_PdaProfKept = got.Length();
        PdaProfPaintState();

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

    // The bar's line: no file yet, the file's size, or that the editor holds less than
    // the file and applying is off.
    private void PdaProfPaintState()
    {
        if (!m_PdaProfPane)
            return;

        int cMuted = ARGB(255, 160, 159, 153);
        int cAlert = ARGB(255, 245, 199, 64);

        if (m_PdaProfBytes < 0)
        {
            PdaPaneText(m_PdaProfPane, "PdaProfState", "#STR_OZ_PDA_VPP_H_NOT_LOADED", cMuted);
            return;
        }

        string bytesWord = Widget.TranslateString("#STR_OZ_PDA_VPP_BYTES");
        string line;
        if (m_PdaProfCut)
        {
            string tooBig = Widget.TranslateString("#STR_OZ_PDA_VPP_H_PROF_CUT");
            line = tooBig + " (" + m_PdaProfBytes.ToString();
            line = line + " / " + m_PdaProfKept.ToString();
            line = line + " " + bytesWord + ")";
            PdaPaneText(m_PdaProfPane, "PdaProfState", line, cAlert);
            return;
        }

        string loaded = Widget.TranslateString("#STR_OZ_PDA_VPP_H_PROF_LOADED");
        line = loaded + " · " + m_PdaProfBytes.ToString();
        line = line + " " + bytesWord;
        PdaPaneText(m_PdaProfPane, "PdaProfState", line, cMuted);
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

    // A text of the window by its name: its words and its colour.
    private void PdaText(string name, string text, int ink)
    {
        PdaPaneText(M_SUB_WIDGET, name, text, ink);
    }

    // The same, looked up under one page (a page is a small tree, the window a large one).
    private void PdaPaneText(Widget pane, string name, string text, int ink)
    {
        if (!pane)
            return;
        TextWidget t = TextWidget.Cast(pane.FindAnyWidget(name));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(ink);
    }

    // A text of a list's row.
    private void PdaRowText(Widget row, string name, string text, int ink)
    {
        TextWidget t = TextWidget.Cast(row.FindAnyWidget(name));
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

    private void PdaMove(string name, float x, float y)
    {
        Widget m = M_SUB_WIDGET.FindAnyWidget(name);
        if (m)
            m.SetPos(x, y);
    }

    // ================================================= розмір вікна

    // The window changed size (OZ_VppAdminMenu.ApplyWindowSize). The pages follow by the
    // rule their layouts carry; the list's rows are made as wide as the list again, and
    // what the card moved by script goes back to its place (the rule puts every widget
    // where the layout has it).
    override void OnWindowResized(float w, float h)
    {
        super.OnWindowResized(w, h);
        if (!M_SUB_WIDGET)
            return;

        if (m_PdaHwHeads)
            m_PdaHwHeads.Fit();
        if (m_PdaHwRows)
            m_PdaHwRows.Fit();
        PdaHwPlace();
    }

    // ================================================= ввід

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
            if (nm == "HwKind")
            {
                PdaHwPaintKindNote();
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

        // A row of the hardware list: its entry on the card.
        if (m_PdaHwRows)
        {
            int hwRow = m_PdaHwRows.IndexOf(w);
            if (hwRow != -1)
            {
                PdaHwPickKey(m_PdaHwRows.Key(hwRow));
                return true;
            }
        }

        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
