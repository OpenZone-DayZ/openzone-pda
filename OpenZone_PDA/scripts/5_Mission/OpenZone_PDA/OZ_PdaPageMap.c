// Сторінка «Карта»: своя позиція, чужі маячки, перемикач транспондера.
//
// Масштаб і зсув веде сам MapWidget -- колесо й перетягування працюють без
// нашого коду. Наше -- позначки й те, коли карту центрувати.
//
// Центрування -- РАЗ НА ВІДКРИТТЯ, не при кожному перемальовуванні: доки
// вкладка стоїть відкритою, ані клік, ані пуш маячків, ані власний перепит
// карту не смикають -- лише BtnCenter і вхід на вкладку. Об'єкт сторінки
// живе між входами (закрив-відкрив -- це той самий OZ_PdaPageMap), тому
// защіпку скидає OnSelected, а не конструктор: інакше прилад, що дізнався
// GPS ПІСЛЯ першого відкриття мапи, назавжди лишався б відцентрованим на
// домівці замість гравця (звіт зі стенду 2026-09-09, п. 3 -- перше
// відкриття з GPS не центрувалось, бо защіпку поставила ще та мапа, що GPS
// не бачила). Той самий зсув ловить і Paint(): якщо GPS з'явився, поки
// сторінка вже стояла відкритою (див. OnRefresh), защіпку скидає він теж.

class OZ_PdaPageMap : OZ_PdaPage
{
    private MapWidget m_Map;
    private ButtonWidget m_BtnMode;
    private ButtonWidget m_BtnCenter;

    private EditBoxWidget m_Name;
    private ButtonWidget m_BtnMark;

    // Список міток: перемикач, панель, рядки, поля редагування.
    private ButtonWidget           m_BtnList;
    private Widget                 m_Panel;
    private Widget                 m_Rows;
    private EditBoxWidget          m_EditName;
    private MultilineEditBoxWidget m_EditDesc;
    // Що Pick поклав в опис і з якого тексту -- як тіло записки.
    private string                 m_DescShown = "";
    private string                 m_DescKept = "";
    private ButtonWidget           m_BtnSave;
    private ButtonWidget           m_BtnShare;
    private ButtonWidget           m_BtnToCar;
    private ButtonWidget           m_BtnDel;
    private ref array<Widget>      m_RowWgts;
    private ButtonWidget m_BtnTrack;
    private ButtonWidget m_BtnRouteAdd;
    private ButtonWidget m_BtnRouteClear;
    private ButtonWidget m_BtnRouteGo;
    private ButtonWidget m_BtnRouteToCar;
    private bool                   m_ListOpen = false;

    // Підпис списку з минулого разу. Стан приходить ЩОСЕКУНДИ (маячки
    // рухаються), а перебудовувати рядки щосекунди означало б скидати скрол
    // і мигтіти. Рядки перебудовуються лише коли мітки СПРАВДІ змінились.
    private string m_RowsSig = "~";

    // Точка натискання миші. Клік приходить після відпускання, і лише
    // порівнявши його з точкою натискання можна відрізнити «клацнув» від
    // «потягнув карту». -1 означає «натискання не бачили» -- тоді кліку
    // віримо (програмний клік не має натискання, а обманювати нема кому).
    private int m_DownX = -1;
    private int m_DownY = -1;
    private static const int DRAG_SLOP_PX = 12;

    // Повний розмір карти, знятий у момент побудови. Одиниці SetSize --
    // ті самі, що поверне GetSize, і саме тому міряємо, а не вписуємо
    // константу з розкладки: розкладка масштабується.
    private float m_MapW;
    private float m_MapH;

    // Скільки ширини лишається карті при відкритому списку: 948 з 1282
    // одиниць розкладки. Панель шириною 325 починається на 957, тобто між
    // стиснутою картою й списком лишається 9 одиниць повітря.
    private static const float LIST_SQUEEZE = 0.7397;

    // АДМІНСЬКИХ КНОПОК ТУТ БІЛЬШЕ НЕМАЄ.
    //
    // SET SPAWN і CLEAR ZONE переїхали на панель SPAWNS у вкладці VPP
    // (рішення власника 2026-09-01: «КПК взагалі не повинен мати адмінських
    // функцій»). Знято ПІСЛЯ того, як панель навчилась усього, що вміли ці
    // дві: слаг організації, слаг базової фракції, стейджинґ, запасну зону,
    // особисту точку й видиму відмову. Раніше -- означало б лишити сервер
    // без ЄДИНОГО способу поставити зону.
    //
    // Разом із ними пішло подвійне призначення поля MarkName: воно знову
    // просто ім'я мітки, і нічого більше.

    private ref OZ_MapState m_State;
    private bool m_Centred = false;

    // GpsKnows() з минулого Paint(). Потрібен лише для одного переходу:
    // false -> true, "прилад щойно дізнався, де він", поки сторінка вже
    // стояла відкритою -- тоді Paint() скидає m_Centred сам, не чекаючи
    // наступного відкриття вкладки (звіт зі стенду 2026-09-09, п. 3).
    private bool m_HadGps = false;

    // Раз на 5 тіків OnRefresh (тобто раз на ~5 с -- сторінки-сусідки
    // рахують так само, аудит 2026-08-30) сторінка перепитує стан САМА,
    // не чекаючи операції гравця. Причина -- у коментарі перед OnRefresh.
    private int m_Beat = 0;

    // Обрана мітка. Порожньо -- нічого не обрано, і кнопка ставить нову.
    private string m_PickedId = "";

    // МІТКА В ДОРОЗІ -- від відправки marker_add до першого стану, що вже
    // її знає. Поки вона в дорозі, нова не шлеться (SendAdd).
    //
    //   0 -- нічого не летить;
    //   1 -- marker_add пішов із ЦІЄЇ сторінки, відповіді ще немає;
    //   2 -- відповідь «так» прийшла, а стану з новою міткою ще немає.
    //
    // Подвійний клік по порожній карті -- це два кліки, і другий приходив
    // раніше, ніж стан приносив першу мітку: MarkerNear її ще не бачив, і
    // йшло друге додавання -- дві мітки, дві комірки пам'яті. Друга сходинка
    // не зайва: між відповіддю на додавання і станом, що її показує, лежить
    // ще одна дорога до сервера й назад. Відповіді йдуть у порядку запитів
    // (гарантований канал ядра), тож перший стан ПІСЛЯ відповіді вже містить
    // мітку.
    //
    // Сходинка 1 заразом каже, чия відповідь прийшла: поле назви чистить
    // лише власне додавання (див. OnResponse).
    private int m_AddStep = 0;

    // Ванільні іконки: своя й чужа мітки мусять відрізнятись з першого
    // погляду, і кольором тут не обійтись -- на карті кольорів і так вистачає.
    private static const string ICON_SELF   = "\\DZ\\gear\\navigation\\data\\map_tshelter_ca.paa";
    private static const string ICON_BEACON = "\\DZ\\gear\\navigation\\data\\map_transmitter_ca.paa";
    private static const string ICON_MARK   = "\\DZ\\gear\\navigation\\data\\map_tsign_ca.paa";

    override string LayoutPath()
    {
        return "OpenZone_PDA/gui/layouts/oz_pda_page_map.layout";
    }

    override void OnBuilt()
    {
        m_Map       = MapWidget.Cast(Wgt("Map"));
        m_BtnCenter = ButtonWidget.Cast(Wgt("BtnCenter"));
        m_BtnMode   = ButtonWidget.Cast(Wgt("BtnMode"));
        m_Name      = EditBoxWidget.Cast(Wgt("MarkName"));
        m_BtnMark   = ButtonWidget.Cast(Wgt("BtnMark"));

        SetText("BtnCenterText", "#STR_OZ_MAP_CENTER");

        m_BtnList  = ButtonWidget.Cast(Wgt("BtnList"));
        if (m_Map)
            m_Map.GetSize(m_MapW, m_MapH);
        m_Panel    = Wgt("MarkerPanel");
        m_Rows     = Wgt("MarkerRows");
        m_EditName = EditBoxWidget.Cast(Wgt("MarkerName"));
        m_EditDesc = MultilineEditBoxWidget.Cast(Wgt("MarkerDesc"));
        m_BtnSave  = ButtonWidget.Cast(Wgt("BtnMarkSave"));
        m_BtnShare = ButtonWidget.Cast(Wgt("BtnMarkShare"));
        SetText("BtnMarkShareText", "#STR_OZ_MAP_SHARE");
        m_BtnToCar = ButtonWidget.Cast(Wgt("BtnMarkToCar"));
        SetText("BtnMarkToCarText", "#STR_OZ_TO_CARRIER");
        m_BtnDel   = ButtonWidget.Cast(Wgt("BtnMarkDel"));
        m_BtnTrack = ButtonWidget.Cast(Wgt("BtnMarkTrack"));
        m_BtnRouteAdd   = ButtonWidget.Cast(Wgt("BtnRouteAdd"));
        SetText("BtnRouteAddText", "#STR_OZ_ROUTE_ADD");
        m_BtnRouteClear = ButtonWidget.Cast(Wgt("BtnRouteClear"));
        SetText("BtnRouteClearText", "#STR_OZ_ROUTE_CLEAR");
        m_BtnRouteGo    = ButtonWidget.Cast(Wgt("BtnRouteGo"));
        m_BtnRouteToCar = ButtonWidget.Cast(Wgt("BtnRouteToCar"));
        SetText("BtnRouteToCarText", "#STR_OZ_ROUTE_TO_CAR");
        m_RowWgts  = new array<Widget>();

        SetText("BtnListText", "#STR_OZ_MAP_LIST");
        SetText("BtnMarkSaveText", "#STR_OZ_MAP_SAVE");
        SetText("BtnMarkDelText", "#STR_OZ_MAP_DELETE");
    }

    // Точку натискання пам'ятаємо ЛИШЕ для карти: решті віджетів вона ні до
    // чого, а подію мусить побачити й рушій -- тому false завжди.
    override bool OnPageMouseDown(Widget w, int x, int y)
    {
        if (w == m_Map)
        {
            m_DownX = x;
            m_DownY = y;
        }
        return false;
    }

    // «Клік» по карті живе на відпусканні: OnClick MapWidget не породжує
    // (зміряно; ваніль слухає OnDoubleClick обробником на самому віджеті).
    // false завжди -- рушію відпускання потрібне для його власного стану.
    override bool OnPageMouseUp(Widget w, int x, int y)
    {
        // Клік -- це НАТИСКАННЯ НА КАРТІ плюс відпускання на ній же. Без
        // цієї умови відпускання миші, натиснутої деінде (кнопка, край
        // панелі), ставило мітку там, де палець зіслизнув з карти.
        if (w == m_Map && m_DownX >= 0)
            MapClick(x, y);
        return false;
    }

    override void OnSelected()
    {
        // Відмова на дію, якої гравець уже не пам'ятає, ні до чого:
        // вкладку перемкнули -- тримати підказку більше нема сенсу.
        ClearHintHold();

        // ФОКУС ЗНІМАЄМО, І ЦЕ НЕ КОСМЕТИКА.
        //
        // Рушій сам дає фокус першому багаторядковому полю при побудові
        // (зміряно на живому клієнті, той самий факт тримає OZ_PdaPageNotes):
        // тут це MarkerDesc. Поле з фокусом забирає клавіатуру собі, і до
        // OnKeyPress меню Escape уже не доходить -- саме тому КПК на цій
        // сторінці не закривався клавішею (звіт власника 2026-09-09, дефект 4).
        // Вихід із меню тепер тримає ще й опит UAUIBack в OZ_PdaMenu.Update,
        // а цей рядок прибирає ще й червону рамку фокуса на порожньому полі.
        SetFocus(null);

        // ЗАЩІПКУ ЦЕНТРУВАННЯ -- ТЕЖ СКИДАЄМО. Об'єкт сторінки живе між
        // входами на вкладку (закрив-відкрив -- та сама m_Centred), а
        // правило -- "з GPS центруємо на кожному відкритті, без GPS -- на
        // домівці мапи" (рішення власника 2026-09-09). Без цього рядка
        // прилад, що дізнався GPS уже ПІСЛЯ першого відкриття мапи,
        // лишався б стояти на домівці назавжди.
        m_Centred = false;
        m_Beat = 0;

        Request();
    }

    // ЩОСЕКУНДИ НЕ ПОЛИТЬ: стан питається при відкритті та після операцій
    // (кожна операція й так перепитує сама), а маячки сервер ПУШИТЬ окремим
    // конвертом -- аудит 2026-08-30 нарахував ~59 зайвих запитів на
    // хвилину відкритої карти на гравця.
    //
    // Є рівно одне, чого жоден із цих шляхів не покриває: залізо (модуль
    // GPS) не штовхає пуш і не супроводжує ЖОДНОЇ дії цієї сторінки -- плату
    // вставляють чи виймають на пристрої, не на карті. Без перепиту гравець,
    // що сидить на відкритій мапі саме в цю мить, побачив би наслідок лише
    // вийшовши з вкладки й зайшовши знову (звіт зі стенду 2026-09-09,
    // зауваження 2). Тому тут -- та сама страховка, що в Contacts і Device:
    // не щосекунди, а раз на 5 тіків.
    //
    // Цей же опит везе й маячки приладу в руках: посилка beacons -- про
    // надітий, і такому екранові її не беремо (див. OnResponse).
    override void OnRefresh()
    {
        m_Beat++;
        if (m_Beat % 5 != 0)
            return;
        Request();
    }

    private void Request()
    {
        OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "state", "{}");
    }

    override bool OnPageClick(Widget w, int x, int y)
    {
        if (!w)
            return false;

        if (w == m_BtnCenter)
        {
            CentreOnSelf();
            return true;
        }

        // Кліка по самій карті ТУТ немає: MapWidget -- не кнопка, OnClick
        // по ньому не приходить (зміряно). Він збирається в OnPageMouseUp.

        // Зони спавна більше не ставлять звідси -- див. коментар угорі про
        // те, куди вони поїхали й чому саме в цьому порядку.

        if (w == m_BtnList)
        {
            m_ListOpen = !m_ListOpen;
            if (m_Panel)
                m_Panel.Show(m_ListOpen);

            // Карта СТИСКАЄТЬСЯ, а не ховається під панель: MapWidget малює
            // себе поверх усього -- і сусідів, і власних дітей, незалежно
            // від priority. Виміряно на стенді: кнопка з priority 4 над
            // картою не малювалась узагалі. Тож поверх карти не малює НІХТО,
            // і місце списку звільняє сама карта.
            if (m_Map && m_MapW > 0)
            {
                if (m_ListOpen)
                    m_Map.SetSize(m_MapW * LIST_SQUEEZE, m_MapH);
                else
                    m_Map.SetSize(m_MapW, m_MapH);
            }

            if (m_ListOpen)
                RebuildRows(true);
            return true;
        }

        // Рядок списку міток. Ім'я віджета -- це Id мітки, див. RebuildRows.
        if (w.GetUserID() == 5)
        {
            Pick(w.GetName());
            return true;
        }

        if (w == m_BtnSave)
        {
            SendMarkerEdit();
            return true;
        }

        if (w == m_BtnToCar)
        {
            if (m_PickedId == "")
            {
                SetHintSticky("MapHint", "#STR_OZ_MAP_PICK_FIRST");
                return true;
            }

            OZ_MarkerRef cref = new OZ_MarkerRef();
            cref.Id = m_PickedId;

            string cjson;
            string cerr;
            if (JsonFileLoader<OZ_MarkerRef>.MakeData(cref, cjson, cerr, false))
                OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "carrier_add", cjson);
            return true;
        }

        if (w == m_BtnShare)
        {
            if (m_PickedId == "")
            {
                SetHintSticky("MapHint", "#STR_OZ_MAP_PICK_FIRST");
                return true;
            }

            OZ_MapMarker mk = FindMarker(m_PickedId);
            if (!mk)
                return true;

            // Формат розбирає одержувач: "[MARK] назва @ x z — опис".
            vector mp = mk.Pos.ToVector();
            int sx = Math.Round(mp[0]);
            int sz = Math.Round(mp[2]);

            string line = "[MARK] " + mk.Name + " @ " + sx.ToString() + " " + sz.ToString();
            // Розділювач ASCII навмисно: типографське тире губилось десь
            // між EditBox і відправкою, і опис не доїжджав (живий тест
            // 2026-08-29). Парсер розуміє обидва написання.
            if (mk.Desc != "")
                line += " -- " + mk.Desc;

            OZ_PdaCompose.Put(line);

            OZ_PdaMenu menu = OZ_PdaMenu.Cast(GetGame().GetUIManager().FindMenu(OZ_PdaConst.MENU_PDA));
            if (menu)
                menu.Select(OZ_PdaConst.PAGE_CHAT);
            return true;
        }

        if (w == m_BtnRouteAdd)
        {
            if (m_PickedId == "")
            {
                SetHintSticky("MapHint", "#STR_OZ_MAP_PICK_FIRST");
                return true;
            }

            OZ_MarkerRef ra = new OZ_MarkerRef();
            ra.Id = m_PickedId;

            string raj;
            string rerr;
            if (JsonFileLoader<OZ_MarkerRef>.MakeData(ra, raj, rerr, false))
                OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "route_add", raj);
            return true;
        }

        if (w == m_BtnRouteClear)
        {
            OZ_PdaRoute.Stop();
            OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "route_clear", "{}");
            return true;
        }

        if (w == m_BtnRouteGo)
        {
            // Неактивний -- АКТИВУВАТИ; активний -- ПРОЙДЕНО (ручна
            // позначка поточної точки). Кінець нитки гасить її сам.
            if (OZ_PdaRoute.Active)
            {
                OZ_PdaRoute.Advance();
            }
            else
            {
                if (!m_State || !m_State.Route || m_State.Route.Count() == 0)
                {
                    SetHintSticky("MapHint", "#STR_OZ_ERR_ROUTE_EMPTY");
                    return true;
                }
                OZ_PdaRoute.Start(m_State.Route);
            }
            RebuildRows(true);
            return true;
        }

        if (w == m_BtnRouteToCar)
        {
            OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "route_write", "{}");
            return true;
        }

        if (w == m_BtnTrack)
        {
            if (m_PickedId == "")
            {
                SetHintSticky("MapHint", "#STR_OZ_MAP_PICK_FIRST");
                return true;
            }

            // Той самий клік знімає ведення з уже веденої мітки.
            if (OZ_PdaTrack.Id == m_PickedId)
            {
                OZ_PdaTrack.Id   = "";
                OZ_PdaTrack.Name = "";
                OZ_PdaTrack.Point  = "";
            }
            else
            {
                OZ_MapMarker tmk = FindMarker(m_PickedId);
                if (tmk)
                {
                    OZ_PdaTrack.Id   = tmk.Id;
                    OZ_PdaTrack.Name = tmk.Name;
                    OZ_PdaTrack.Point  = tmk.Pos;
                }
            }

            RebuildRows(true);
            return true;
        }

        if (w == m_BtnDel)
        {
            if (m_PickedId == "")
            {
                SetHintSticky("MapHint", "#STR_OZ_MAP_PICK_FIRST");
                return true;
            }
            SendMarkerDelete();
            return true;
        }

        if (w == m_BtnMark)
        {
            if (m_PickedId != "")
                SendMarkerDelete();
            else
                SendMarkerAdd();
            return true;
        }

        if (w == m_BtnMode)
        {
            OZ_TransponderOp op = new OZ_TransponderOp();
            NextSet(op.Set);

            string json;
            string err;
            if (JsonFileLoader<OZ_TransponderOp>.MakeData(op, json, err, false))
                OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "transponder", json);
            return true;
        }

        return false;
    }

    // ПОДВІЙНИЙ КЛІК ПО РЯДКУ СПИСКУ -- ПОКАЗАТИ ЦЮ МІТКУ: карта стає так,
    // щоб мітка опинилась у центрі (рішення власника 2026-09-09).
    //
    // Одинарний клік лишається тим, чим був, -- обрати й підсвітити, карти не
    // рухати. Саме тому, що карту смикав КОЖЕН вибір, SetMapPos із Pick() і
    // забрали (звіт власника 2026-09-09, дефект 1); тепер «покажи мені її» --
    // окремий жест, про який гравець просить сам, а не наслідок вибору.
    //
    // МАСШТАБ НЕ ЧІПАЄМО (рішення власника 2026-09-09): наскільки близько
    // дивитись, гравець вирішив сам. Ціна -- притиск рушія: він не дає
    // поставити центр так, щоб за краєм мапи лишилась порожнеча, тож на
    // далекому масштабі мітка біля краю світу стане не рівно посередині (той
    // самий притиск, через який Paint() спершу наближає, а потім цілиться, --
    // зміряно на стенді 2026-09-09). Це межа рушія, не наша.
    //
    // GPS тут ні до чого: стати на МІТКУ -- не те саме, що сказати «ти тут»
    // (ТЗ-4 R-B2.2), тож жест працює й на приладі, який не знає, де він.
    override bool OnPageDoubleClick(Widget w, int x, int y)
    {
        // Тільки рядки списку: UserID 5, ім'я віджета -- Id мітки, як в
        // OnPageClick. Діти рядка всі ignorepointer (розкладка
        // oz_pda_marker_row), тож під курсором завжди сам рядок -- шукати
        // предка з UserID 5 нема потреби.
        //
        // Подвійний клік по самій карті лишається тим, чим був, -- парою
        // одинарних, які збирає OnPageMouseUp. Мітку з такої пари ставить
        // лише перший: другий застає її в дорозі (m_AddStep).
        if (!w || w.GetUserID() != 5)
            return false;

        OZ_MapMarker m = FindMarker(w.GetName());
        if (!m || !m_Map)
            return false;

        // Вибір і підсвітка -- ті самі, що від одинарного кліку. Перший клік
        // пари їх, найпевніше, уже поставив, але порядку подій рушія ми не
        // міряли, тож жест робить це сам.
        if (m_PickedId != m.Id)
            Pick(m.Id);

        // ПОЗИЦІЯ -- ОСТАННІМ РЯДКОМ, і це не випадковість. Pick() тягне за
        // собою Paint(), а той на першому кадрі вкладки сам ставить масштаб і
        // центр; наше прохання мусить лягти ПІСЛЯ нього, інакше карту зсунуть
        // уже після нас. Той самий порядок «спершу масштаб, потім позиція»,
        // що й у Paint() (Task 85).
        m_Map.SetMapPos(m.Pos.ToVector());
        return true;
    }

    // Коло від найтихішого до найгучнішого (ТЗ-4 R-A3.1, R-A3.2):
    //   contacts -> faction -> faction+contacts -> public -> contacts.
    // Випадкове натискання підвищує гучність на один крок, а не вмикає одразу
    // «всім». Кроки з "faction" існують лише тоді, коли сервер сказав, що
    // мод фракцій є (R-A3.4): без нього перемикача немає, а не «не працює».
    //
    // СХОДИНКИ «ВИМКНЕНО» В КОЛІ НЕМАЄ (рішення власника 2026-09-09): прилад
    // веде завжди, коли він увімкнений і має GPS, а кнопка обирає лише КОМУ.
    // Вимикачем тепер є сам прилад -- і вийнятий модуль GPS.
    private string SetKey(array<string> s)
    {
        if (s && s.Find("public") != -1)
            return "public";

        bool f = false;
        bool c = false;
        if (s)
        {
            f = s.Find("faction") != -1;
            c = s.Find("contacts") != -1;
        }

        if (f && c)
            return "both";
        if (f)
            return "faction";
        // Порожній набір -- це запис, якого ще не торкнувся ремонт на
        // сервері (OZ_PdaAudience). Показуємо те, чим він стане, а не
        // «вимкнено», якого більше немає.
        return "contacts";
    }

    private void NextSet(array<string> outSet)
    {
        string cur = OZ_PdaConst.TRANS_DEFAULT;
        bool factions = false;
        if (m_State)
        {
            cur = SetKey(m_State.TransponderSet);
            factions = m_State.FactionsPresent;
        }

        outSet.Clear();

        if (cur == "contacts")
        {
            if (factions)
            {
                outSet.Insert("faction");
                return;
            }
            outSet.Insert("public");
            return;
        }
        if (cur == "faction")
        {
            outSet.Insert("faction");
            outSet.Insert("contacts");
            return;
        }
        if (cur == "both")
        {
            outSet.Insert("public");
            return;
        }

        // "public" -> найтихіше коло, і коло замкнулось.
        outSet.Insert("contacts");
    }

    // Куди клікнули у світових координатах, і що там уже стоїть.
    private void MapClick(int x, int y)
    {
        if (!m_Map || !m_State)
            return;

        // ПЕРЕТЯГУВАННЯ -- НЕ КЛІК. Клік приходить після відпускання, тобто
        // й наприкінці кожного зсуву карти теж. Якби він ставив мітку, кожен
        // зсув лишав би по мітці там, де палець відпустив. Порівнюємо з
        // точкою натискання: зсунулись далі за поріг -- це був зсув.
        if (m_DownX >= 0)
        {
            int moved = Math.AbsInt(x - m_DownX) + Math.AbsInt(y - m_DownY);
            m_DownX = -1;
            m_DownY = -1;
            if (moved > DRAG_SLOP_PX)
                return;
        }

        vector at = m_Map.ScreenToMap(Vector(x, y, 0));

        string hit = MarkerNear(at);
        if (hit != "")
        {
            // Повторний клік по обраній -- зняти вибір. Інакше «нічого не
            // обрано» не досягалося б узагалі.
            if (hit == m_PickedId)
                Pick("");
            else
                Pick(hit);
            return;
        }

        // Порожнє місце -- СТАВИМО МІТКУ ТУТ, одразу. Назва з поля внизу,
        // опис додається потім через панель. Промах коштує два кліки:
        // обрати й видалити.
        OZ_MapMarker m = new OZ_MapMarker();
        m.Pos = at.ToString(false);
        if (m_Name)
            m.Name = m_Name.GetText();

        SendAdd(m);
    }

    // ОДНА ДОРОГА ДЛЯ НОВОЇ МІТКИ -- і з кліку по карті, і з кнопки. Тут же
    // стоїть защіпка «мітка в дорозі» (m_AddStep): друга мітка з подвійного
    // кліку не йде, поки перша не повернулась станом.
    private void SendAdd(OZ_MapMarker m)
    {
        if (m_AddStep != 0)
        {
            OZ_Log.Dbg("map: a mark is still on its way - the next add is dropped");
            return;
        }

        string json;
        string err;
        if (!JsonFileLoader<OZ_MapMarker>.MakeData(m, json, err, false))
            return;

        m_AddStep = 1;
        OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "marker_add", json);
    }

    // Обрати мітку (або зняти вибір порожнім id): підсвітити на карті й у
    // списку, заповнити поля редагування.
    //
    // КАРТУ ЦЕ БІЛЬШЕ НЕ РУХАЄ (звіт власника 2026-09-09, дефект 1). Тут
    // стояв SetMapPos на позицію обраної мітки, і через нього КОЖЕН клік
    // по мітці сіпав карту з-під пальця: мітки здебільшого стоять там, де
    // гравець стояв, тож збоку це виглядало як «карта весь час вертається
    // на мене». Карта лишається там, куди її поставив гравець; повернути її
    // до себе -- окрема кнопка (BtnCenter), і лише вона.
    private void Pick(string id)
    {
        m_PickedId = id;

        OZ_MapMarker m = FindMarker(id);
        if (m)
        {
            if (m_EditName)
                m_EditName.SetText(m.Name);
            // Як у записках: розриви Wrap склеїть Read на збереженні, а
            // нетронутий опис Read поверне збереженим.
            if (m_EditDesc)
            {
                m_DescKept  = m.Desc;
                m_DescShown = OZ_Unwrap.Wrap(m.Desc, m_EditDesc, TextWidget.Cast(Wgt("DescRuler")));
                m_EditDesc.SetText(m_DescShown);
            }
        }
        else
        {
            if (m_EditName)
                m_EditName.SetText("");
            if (m_EditDesc)
                m_EditDesc.SetText("");
        }

        PaintMarkButton();
        Paint();

        // ПІДСВІТКУ ПЕРЕМАЛЬОВУЄМО НА МІСЦІ, А НЕ ПЕРЕБУДОВОЮ СПИСКУ.
        //
        // Тут стояло RebuildRows(true): вибір міняє в рядку рівно одну річ --
        // смужку RowPick, -- а перебудова вбивала (Unlink) УСІ рядки й робила
        // на їхньому місці нові. Оку це коштувало скинутий скрол і мигтіння,
        // а подвійному кліку -- усього жесту: перший клік пари знищував саме
        // той віджет, на який мав прийти другий (рішення власника
        // 2026-09-09 -- подвійний клік центрує карту). Рядки мусять пережити
        // жест, тому вибір їх більше не перестворює.
        PaintPick();
    }

    private OZ_MapMarker FindMarker(string id)
    {
        if (id == "" || !m_State || !m_State.Markers)
            return null;

        for (int i = 0; i < m_State.Markers.Count(); i++)
        {
            if (m_State.Markers[i].Id == id)
                return m_State.Markers[i];
        }
        return null;
    }

    private void SendMarkerEdit()
    {
        if (m_PickedId == "")
        {
            SetHintSticky("MapHint", "#STR_OZ_MAP_PICK_FIRST");
            return;
        }

        OZ_MapMarker m = new OZ_MapMarker();
        m.Id = m_PickedId;
        if (m_EditName)
            m.Name = m_EditName.GetText();
        if (m_EditDesc)
        {
            // GetText багаторядкового поля пише в out-параметр, а не
            // повертає: він успадкований від іншого прото, ніж у EditBox.
            // Через розклейку: опис теж обростав переносами посеред слів.
            string d = OZ_Unwrap.Read(m_EditDesc, TextWidget.Cast(Wgt("DescRuler")), m_DescShown, m_DescKept);
            m.Desc = d;
        }

        string json;
        string err;
        if (JsonFileLoader<OZ_MapMarker>.MakeData(m, json, err, false))
            OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "marker_edit", json);
    }

    // Спрайт рядка мітки. OZ_MapMarker (OZ_PdaTypes.c) поки несе лише
    // Id/Name/Pos/Desc -- поля виду немає, тож завжди базова крапка;
    // mk_stash/mk_danger/mk_route чекають на це поле в мітці.
    private string SpriteFor(OZ_MapMarker m)
    {
        return "mk_marker";
    }

    // Підсвітка обраного рядка -- НА МІСЦІ, без перестворення рядків: єдине,
    // що в рядку залежить від вибору, -- смужка RowPick. Ім'я рядка -- це Id
    // мітки (див. RebuildRows), тому шукати нічого не треба.
    //
    // Разом із нею -- напис кнопки ведення: він теж залежить від вибору, і
    // більше ніде не ставиться.
    private void PaintPick()
    {
        for (int i = 0; m_RowWgts && i < m_RowWgts.Count(); i++)
        {
            Widget row = m_RowWgts[i];
            if (!row)
                continue;

            Widget pick = row.FindAnyWidget("RowPick");
            if (pick)
                pick.Show(row.GetName() == m_PickedId);
        }

        if (m_BtnTrack)
        {
            if (m_PickedId != "" && m_PickedId == OZ_PdaTrack.Id)
                SetText("BtnMarkTrackText", "#STR_OZ_MAP_UNTRACK");
            else
                SetText("BtnMarkTrackText", "#STR_OZ_MAP_TRACK");
        }
    }

    // Перебудувати рядки списку. force -- перебудувати завжди (список щойно
    // відкрили, ведення чи маршрут змінились); без force -- лише коли самі
    // мітки змінилися, бо стан приходить щосекунди.
    //
    // ВИБОРУ В ПІДПИСІ БІЛЬШЕ НЕМАЄ: рядки переживають зміну вибору, а
    // підсвітку кладе PaintPick -- див. коментар у Pick().
    private void RebuildRows(bool force)
    {
        if (!m_Rows || !m_ListOpen)
            return;

        string sig = "";
        if (m_State && m_State.Markers)
        {
            for (int i = 0; i < m_State.Markers.Count(); i++)
            {
                OZ_MapMarker mk = m_State.Markers[i];
                sig += mk.Id + "|" + mk.Name + "|" + mk.Desc + ";";
            }
        }
        sig += "#" + OZ_PdaTrack.Id;
        if (m_State && m_State.Route)
            sig += "$" + m_State.Route.Count().ToString();
        sig += "&" + OZ_PdaRoute.At.ToString() + OZ_PdaRoute.Active.ToString();

        if (!force && sig == m_RowsSig)
            return;
        m_RowsSig = sig;

        for (int r = 0; r < m_RowWgts.Count(); r++)
        {
            if (m_RowWgts[r])
                m_RowWgts[r].Unlink();
        }
        m_RowWgts.Clear();
        m_Rows.Update();

        int n = 0;
        int limit = 0;
        if (m_State)
        {
            if (m_State.Markers)
                n = m_State.Markers.Count();
            limit = m_State.MarkerLimit;
        }
        SetText("SecMarksLbl", Widget.TranslateString("#STR_OZ_MAP_LIST") + "  " + n.ToString() + "/" + limit.ToString());

        // Гравець, від якого міряються відстані. Дистанція в списку --
        // знімок на мить перемальовування; ЖИВА цифра веденої мітки живе
        // під мінікартою.
        vector meAt = vector.Zero;
        PlayerBase mePl = PlayerBase.Cast(GetGame().GetPlayer());
        if (mePl)
            meAt = mePl.GetPosition();

        for (int k = 0; k < n; k++)
        {
            OZ_MapMarker mrk = m_State.Markers[k];

            Widget row = GetGame().GetWorkspace().CreateWidgets("OpenZone_PDA/gui/layouts/oz_pda_marker_row.layout", m_Rows);
            if (!row)
                break;

            // Ім'я віджета -- Id мітки: саме його читають і OnPageClick, і
            // OnPageDoubleClick, і PaintPick.
            row.SetName(mrk.Id);
            row.SetUserID(5);
            m_RowWgts.Insert(row);

            ImageWidget ic = ImageWidget.Cast(row.FindAnyWidget("RowIcon"));
            if (ic)
                ic.LoadImageFile(0, "set:oz_pda_icons image:" + SpriteFor(mrk));

            TextWidget name = TextWidget.Cast(row.FindAnyWidget("RowName"));
            if (name)
            {
                if (mrk.Name != "")
                    name.SetText(mrk.Name);
                else
                    name.SetText("#STR_OZ_MAP_UNNAMED");
            }

            TextWidget where = TextWidget.Cast(row.FindAnyWidget("RowWhere"));
            if (where)
            {
                vector p = mrk.Pos.ToVector();
                int px = Math.Round(p[0]);
                int pz = Math.Round(p[2]);

                string wtxt = px.ToString() + " " + pz.ToString();
                if (mePl && GpsKnows())
                {
                    int dm = Math.Round(vector.Distance(Vector(meAt[0], 0, meAt[2]), Vector(p[0], 0, p[2])));
                    wtxt += "  " + dm.ToString() + " m";
                }
                if (mrk.Id == OZ_PdaTrack.Id)
                    wtxt = ">> " + wtxt;
                where.SetText(wtxt);
            }

            // Опис у рядку РІЖЕТЬСЯ: сервер дозволяє 160 байтів
            // (MarkerDescMaxBytes), а другий рядок тримає 261 одиницю, тобто
            // 33 українські літери у 13 pt (зміряно на стенді 2026-09-05:
            // 33 літери = 257.8). 64 байти -- це 32 літери, і вони влазять із
            // запасом. Хвіст не губиться: цілий опис показує поле MarkerDesc,
            // щойно рядок обрано. Clip ріже по межі символу, а не байта.
            TextWidget desc = TextWidget.Cast(row.FindAnyWidget("RowDesc"));
            if (desc)
                desc.SetText(OZ_Text.Clip(mrk.Desc, 64));
        }

        // Стопка складається САМА: MarkerRows -- WrapSpacer із «Size To
        // Content V» 1, рядки в ньому пропорційні (size 1 42), і після
        // Update() його висота дорівнює сумі рядків. Ні SetPos, ні SetSize
        // тут більше немає. Старий коментар звинувачував спейсер у намотці
        // власного розміру (нібито 105 613 юнітів); зміряно наново на стенді
        // 2026-09-04 -- 30 рядків дають рівно 30 x 42, а скрол прокручує.
        m_Rows.Update();

        // Свіжі рядки народжуються без підсвітки -- кладемо її одним місцем
        // на всю сторінку, тим самим, яким її міняє вибір.
        PaintPick();

        int rpts = 0;
        if (m_State && m_State.Route)
            rpts = m_State.Route.Count();

        string rhead = Widget.TranslateString("#STR_OZ_ROUTE") + "  " + rpts.ToString();
        if (OZ_PdaRoute.Active)
            rhead += "  [" + (OZ_PdaRoute.At + 1).ToString() + "/" + OZ_PdaRoute.CountSafe().ToString() + "]";
        SetText("RouteHead", rhead);

        if (m_BtnRouteGo)
        {
            if (OZ_PdaRoute.Active)
                SetText("BtnRouteGoText", "#STR_OZ_ROUTE_PASS");
            else
                SetText("BtnRouteGoText", "#STR_OZ_ROUTE_GO");
        }
    }

    private string MarkerNear(vector at)
    {
        if (!m_State.Markers)
            return "";

        for (int i = 0; i < m_State.Markers.Count(); i++)
        {
            OZ_MapMarker m = m_State.Markers[i];
            vector p = m.Pos.ToVector();

            // По ПЛОЩИНІ: висота мітки й висота кліку по карті -- різні речі,
            // і додавати її у відстань означало б не влучати на схилах.
            float dx = p[0] - at[0];
            float dz = p[2] - at[2];
            if (Math.Sqrt(dx * dx + dz * dz) <= OZ_PdaConst.MARKER_PICK_M)
                return m.Id;
        }
        return "";
    }

    private void SendMarkerAdd()
    {
        // Кнопка ставить мітку ТАМ, ДЕ СТОЇШ. «Позначити місце, де я стою» --
        // найчастіша дія в Зоні. Мітку В ІНШОМУ місці ставить клік по карті.
        //
        // АЛЕ «ДЕ СТОЇШ» ЗНАЄ ЛИШЕ ПРИЛАД З GPS (ТЗ-4 R-B2.2). Позицію кнопка
        // бере з локальної сутності гравця, і приладу вона не питала: КПК без
        // GPS клав мітку рівно під ноги, а список показував її координати --
        // тобто казав гравцеві точне «ти тут» обхідним шляхом. Клік по карті
        // лишається: там місце обирає сам гравець, і прилад нічого йому не
        // підказує.
        if (m_State && !GpsKnows())
        {
            // Капсула «де я» теж не знає, але причина в неї своя, і назвати
            // треба саме її: будь-яку зміну міток капсулі відмовляє сервер.
            if (m_State.Frozen)
                SetHintSticky("MapHint", "#STR_OZ_ERR_FROZEN");
            else
                SetHintSticky("MapHint", "#STR_OZ_MAP_NO_GPS");
            return;
        }

        string at = "";
        if (m_State)
            at = LocalSelfPos();

        if (at == "")
        {
            SetHintSticky("MapHint", "#STR_OZ_MAP_MARK_HINT");
            return;
        }

        OZ_MapMarker m = new OZ_MapMarker();
        m.Pos = at;
        if (m_Name)
            m.Name = m_Name.GetText();

        SendAdd(m);
    }

    private void SendMarkerDelete()
    {
        OZ_MarkerRef r = new OZ_MarkerRef();
        r.Id = m_PickedId;

        string json;
        string err;
        if (JsonFileLoader<OZ_MarkerRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZ_PdaConst.PAGE_MAP, "marker_del", json);
    }

    private void PaintMarkButton()
    {
        if (m_PickedId != "")
            SetText("BtnMarkText", "#STR_OZ_MAP_DELETE");
        else
            SetText("BtnMarkText", "#STR_OZ_MAP_MARK");
    }

    // «До мене» -- єдине місце, яке має право рухати карту після відкриття.
    // Без GPS приладу нема куди вести: він не знає, де він (ТЗ-4 R-B2.2),
    // і кнопка на цей час схована (див. Paint).
    private void CentreOnSelf()
    {
        if (!GpsKnows())
            return;

        string selfPos = LocalSelfPos();
        if (!m_Map || selfPos == "")
            return;

        m_Map.SetMapPos(selfPos.ToVector());
    }

    override void OnResponse(string op, bool ok, string json, string error)
    {
        // Пуш маячків: жива частина стану їде сама, поки сторінка відкрита.
        //
        // АЛЕ ПОСИЛКА -- ПРО НАДІТИЙ ПРИЛАД: вона годує мінікарту худа. Ця ж
        // сторінка говорить про прилад ЕКРАНА -- той, про який відповідає
        // state, спершу руки. Коли це різні прилади, посилка з надітого
        // переписувала екран приладу в руках: КПК без GPS у руці раптом
        // «чув» ефір, а шпигунська плата надітого вливала в нього всі
        // транспондери. Тоді сторінці вистачає власного опиту state
        // (OnRefresh), який про її прилад і питає.
        if (op == "beacons" && ok)
        {
            if (!ScreenIsWorn())
                return;

            OZ_BeaconPush bp = new OZ_BeaconPush();
            string berr;
            if (JsonFileLoader<OZ_BeaconPush>.LoadData(json, bp, berr) && bp && m_State)
            {
                // Копія поелементно: маячки лишаються в стані сторінки й
                // малюються на кожному перемалюванні.
                m_State.Beacons = new array<ref OZ_MapBeacon>();
                if (bp.Beacons)
                {
                    for (int nb = 0; nb < bp.Beacons.Count(); nb++)
                    {
                        if (bp.Beacons[nb])
                            m_State.Beacons.Insert(bp.Beacons[nb].Copy());
                    }
                }
                Paint();
            }
            return;
        }

        if (op == "route_add" || op == "route_clear" || op == "route_write" || op == "route_take")
        {
            if (!ok)
                SetHintSticky("MapHint", "#" + error);
            else if (op == "route_write")
                SetHintSticky("MapHint", "#STR_OZ_CARRIER_SAVED");
            Request();
            return;
        }

        if (op == "carrier_add")
        {
            if (ok)
                SetHintSticky("MapHint", "#STR_OZ_CARRIER_SAVED");
            else
                SetHintSticky("MapHint", "#" + error);
            return;
        }

        if (op == "transponder" || op == "marker_add" || op == "marker_del" || op == "marker_edit")
        {
            // Чи це відповідь на НАШЕ додавання (див. m_AddStep). Мітка, що
            // лягла, лишається «в дорозі» до наступного стану; відмова --
            // чекати більше нічого.
            bool ownAdd = false;
            if (op == "marker_add" && m_AddStep == 1)
            {
                ownAdd = true;
                if (ok)
                    m_AddStep = 2;
                else
                    m_AddStep = 0;
            }

            if (!ok)
            {
                SetHintSticky("MapHint", "#" + error);
            }
            else if (op == "marker_add")
            {
                // Поставили -- поле підпису чистимо, інакше наступна мітка
                // мовчки успадкує чужу назву.
                //
                // ЛИШЕ СВОЄ ДОДАВАННЯ. Поле належить цій сторінці, а відповідь
                // map/marker_add колись приходила й на запит сторінки чату --
                // і стирала назву, яку гравець саме набирав тут.
                if (ownAdd && m_Name)
                    m_Name.SetText("");
            }
            else if (op == "marker_del")
            {
                Pick("");
            }

            PaintMarkButton();
            Request();
            return;
        }

        if (op != "state")
            return;

        // Стан ПІСЛЯ відповіді на додавання: мітка в ньому вже є -- або не
        // ляже ніколи, якщо стану відмовили. Чекати більше нічого.
        if (m_AddStep == 2)
            m_AddStep = 0;

        if (!ok)
        {
            SetHintSticky("MapHint", "#" + error);
            return;
        }

        string err;
        OZ_MapState st = new OZ_MapState();
        if (!JsonFileLoader<OZ_MapState>.LoadData(json, st, err))
        {
            OZ_Log.Error("map state unreadable: " + err);
            return;
        }

        // Копія: вкладене виділив серіалізатор, а тримаємо ми це між кадрами.
        m_State = st.Copy();
        Paint();
    }

    // Своя позиція клієнтові відома без сервера; з відповіді вона
    // прибрана з ужитку -- інакше точка «ти тут» жила б минулою секундою.
    private string LocalSelfPos()
    {
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        if (!me)
            return "";
        return me.GetPosition().ToString(false);
    }

    // Чи прилад ЕКРАНА -- той самий, що НАДІТИЙ. Прилад екрана питаємо тим
    // самим правилом, яким сервер відповідає на state (OZ_PdaHud.Device(),
    // спершу руки); надітий -- тим, яким сервер збирає посилку маячків.
    //
    // Екран сьогодні відкривається лише дією над приладом у руках
    // (OZ_PdaMenu.m_Device), тож поки меню відкрите, відповідь -- «ні», і
    // сторінка живе самим state. Гілку посилки все одно не викидаємо:
    // правило «посилка -- лише екранові надітого» не мусить триматися на
    // тому, як сьогодні відкривається меню.
    private bool ScreenIsWorn()
    {
        OZ_PDA_Base worn = OZ_PdaLookup.WornBy(PlayerBase.Cast(GetGame().GetPlayer()));
        if (!worn)
            return false;
        return OZ_PdaHud.Device() == worn;
    }

    private void Paint()
    {
        // GPS, ЩО З'ЯВИВСЯ, ПОКИ СТОРІНКА ВЖЕ СТОЯЛА ВІДКРИТОЮ, скидає
        // защіпку сам, не чекаючи наступного OnSelected: без цього мапа,
        // відкрита ДО того, як плату вставили (OnRefresh чи операція
        // підняли свіжий стан), лишалась би відцентрованою на першій
        // мітці, хоча прилад уже знає гравця (звіт зі стенду 2026-09-09,
        // п. 3). Зворотний перехід (GPS зник) нічого не скидає -- рухати
        // карту нікуди, а "де стояла, там і лишилась" -- те саме правило,
        // що й для кожного іншого перемальовування.
        bool gpsNow = GpsKnows();
        if (gpsNow && !m_HadGps)
            m_Centred = false;
        m_HadGps = gpsNow;

        SetText("BtnModeText", ModeLabel(SetKey(m_State.TransponderSet)));
        string selfPos = LocalSelfPos();

        if (m_Map)
        {
            // Стираємо ВСЕ й малюємо заново: маячки рухаються, і додавати
            // поверх старих означало б лишити на карті сліди там, де людини
            // вже немає.
            m_Map.ClearUserMarks();

            // AddUserMark НЕ розгортає ключі перекладу: те, що SetText
            // показав би як «you», тут вилізло б на карту як
            // #STR_OZ_MAP_YOU. Розгортаємо самі -- саме для цього
            // Widget.TranslateString і є.
            if (selfPos != "" && GpsKnows())
                m_Map.AddUserMark(selfPos.ToVector(), Widget.TranslateString("#STR_OZ_MAP_YOU"), OZ_PdaConst.MARK_SELF, ICON_SELF);

            for (int i = 0; m_State.Beacons && i < m_State.Beacons.Count(); i++)
            {
                OZ_MapBeacon b = m_State.Beacons[i];
                m_Map.AddUserMark(b.Pos.ToVector(), b.Name, OZ_PdaConst.MARK_BEACON, ICON_BEACON);
            }

            for (int k = 0; m_State.Markers && k < m_State.Markers.Count(); k++)
            {
                OZ_MapMarker m = m_State.Markers[k];

                // Обрана мітка світиться -- інакше після кліку не видно, яку
                // саме зараз видалить кнопка. Колір ОКРЕМИЙ (MARK_PICK), а не
                // MARK_SELF: тим самим помаранчевим намальовано «ти тут», і
                // обрана мітка зливалася з ним у ту саму пляму (звіт власника
                // 2026-09-09, дефект 6). MARK_PICK -- це $accent із токенів,
                // той самий колір, яким підсвічений рядок у списку.
                int colour = OZ_PdaConst.MARK_PLAIN;
                if (m.Id == m_PickedId)
                    colour = OZ_PdaConst.MARK_PICK;

                m_Map.AddUserMark(m.Pos.ToVector(), m.Name, colour, ICON_MARK);
            }

            for (int rr = 0; m_State.Route && rr < m_State.Route.Count(); rr++)
            {
                OZ_MapMarker rm = m_State.Route[rr];
                // Нитка нумерована прямо в підписі: порядок і є маршрут.
                m_Map.AddUserMark(rm.Pos.ToVector(), (rr + 1).ToString() + ". " + rm.Name, OZ_PdaConst.MARK_ROUTE, ICON_MARK);
            }

            // Кожне відкриття (m_Centred скидає OnSelected -- і Paint(), якщо
            // GPS з'явився щойно; див. коментарі при обох) -- показуємо
            // гравцеві, де він. Далі карта лишається там, куди її поставив
            // він сам, і жодне перемальовування її не рухає, доки вкладку
            // не покинули й не відкрили знову.
            //
            // Без GPS показувати нема чого: прилад не знає, де він (ТЗ-4
            // R-B2.2), і відкрити карту рівно на гравцеві означало б сказати
            // йому те, чого прилад не знає. Тоді мапа стає на ДОМІВКУ --
            // одне й те саме місце, яке назвав адмін ключем MapHome, а без
            // нього центр карти. ЯКОРЯ НА ПЕРШІЙ МІТЦІ БІЛЬШЕ НЕМАЄ (рішення
            // власника 2026-09-09): він робив із чужої мітки на носії
            // випадкову домівку, а прилад без міток не рухався взагалі --
            // тобто відкривався там, де його лишив попередній екран.
            if (!m_Centred)
            {
                vector centreAt;
                bool have = false;

                if (GpsKnows())
                {
                    // Сутність гравця може бути ще не відома клієнтові --
                    // тоді не рухаємо нічого й пробуємо наступним кадром:
                    // порожній рядок дав би нуль, тобто ріг карти.
                    if (selfPos != "")
                    {
                        centreAt = selfPos.ToVector();
                        have = true;
                    }
                }
                else
                {
                    centreAt = OZ_PdaHome.Point(m_State.MapHome);
                    have = true;
                }

                if (have)
                {
                    m_Centred = true;

                    // МАСШТАБ ПЕРШИМ, ПОТІМ ПОЗИЦІЯ, і порядок тут не
                    // косметика.
                    //
                    // Свіжий MapWidget відкривається на своєму поставочному
                    // масштабі, а на ньому вся Чорнарусь ледве влазить у
                    // ширину віджета -- і рушій НЕ ДАЄ поставити центр так,
                    // щоб за краєм мапи лишилась порожнеча: він притискає
                    // прохання до середини. Тому SetMapPos, покликаний
                    // ПЕРШИМ, отримував 12887 по x і давав 7680, а SetScale
                    // потім наближав уже не туди (зміряно на стенді
                    // 2026-09-09: без GPS мапа ставала на домівку правильно
                    // -- домівка і є центр, -- а з GPS «ти тут» опинявся за
                    // краєм екрана, на 4 км від центру).
                    //
                    // Наблизивши спершу, ми звужуємо видиме вікно, і те саме
                    // прохання вже проходить без притискання.
                    m_Map.SetScale(0.35);
                    m_Map.SetMapPos(centreAt);
                }
            }
        }

        PaintTransponder();

        // «До мене» без GPS нікуди не веде -- ховаємо разом із транспондером.
        if (m_BtnCenter)
            m_BtnCenter.Show(GpsKnows());

        PaintMarkButton();
        RebuildRows(false);
        SetHint("MapHint", Hint());
    }

    // ВОРОТА ТРАНСПОНДЕРА НА ЕКРАНІ -- ТІ САМІ, ЩО НА СЕРВЕРІ (ТЗ-4 R-A2.4).
    //
    // Окремого «модуля транспондера» в моді немає й ніколи не було: передає
    // МОДУЛЬ GPS -- прилад, який не знає, де він, не може сказати цього нікому
    // (R-A2.5), а дальність стоїть у його ж записі заліза. Кнопка ж стояла
    // безумовно: гравець без GPS крутив коло глядачів, сервер його слухняно
    // записував, а маячка не було й бути не могло -- звіт власника
    // 2026-09-09, дефект 3.
    //
    // Ховаємо, а не гасимо написом: причина вже сказана словом у смузі
    // підказки (STR_OZ_MAP_NO_GPS), і другий раз про те саме кнопка мовчати
    // не мусить.
    private void PaintTransponder()
    {
        if (!m_BtnMode)
            return;

        // По одній умові на рядок: `x = a && b` у Enforce ненадійне, а `if (a ||`
        // з переносом парсер відкидає взагалі.
        bool can = true;
        if (!m_State)
            can = false;
        else if (m_State.Frozen)
            can = false;
        else if (!m_State.HasGps)
            can = false;
        else if (m_State.TransponderRangeM <= 0)
            can = false;

        m_BtnMode.Show(can);
    }

    // Три різні «нікого не видно», і гравець мусить розрізняти їх:
    // немає GPS -- слухати нема чим;
    // GPS є, нікого немає -- нікого й немає;
    // GPS є, хтось є -- скільки саме.
    private string Hint()
    {
        // Капсула: світ у ній зупинився (ТЗ-4 R-B1) -- ні живих маячків, ні
        // «ти тут»; є лише записане на приладі.
        if (m_State.Frozen)
        {
            string cap = Marks();
            cap += "   ";
            cap += "#STR_OZ_MAP_CAPSULE";
            return cap;
        }

        // Без GPS прилад не знає, де він (R-B2.4): карта є, мітки й маршрут
        // є, а «ти тут», відстані й транспондер -- ні. Один рядок на все
        // це: відколи приймач і передавач -- один модуль, двох різних
        // «нема чим» більше не буває.
        if (!m_State.HasGps)
        {
            string noGps = Marks();
            noGps += "   ";
            noGps += "#STR_OZ_MAP_NO_GPS";
            return noGps;
        }

        int n = 0;
        if (m_State.Beacons)
            n = m_State.Beacons.Count();

        int km = Math.Round(m_State.TransponderRangeM);

        string s = Marks();
        s += "   ";
        s += "#STR_OZ_MAP_RANGE";
        s += "  " + km.ToString() + " m";

        if (n == 0)
        {
            s += "   ";
            s += "#STR_OZ_MAP_NOBODY";
            return s;
        }

        s += "   ";
        s += "#STR_OZ_MAP_BEACONS";
        s += "  " + n.ToString();
        return s;
    }

    // «Ти тут» і відстані -- лише коли прилад знає, де він (GPS), і не є
    // капсулою (ТЗ-4 R-B1.1, R-B2.2).
    private bool GpsKnows()
    {
        if (!m_State)
            return false;
        return m_State.HasGps && !m_State.Frozen;
    }

    private string Marks()
    {
        int have = 0;
        if (m_State.Markers)
            have = m_State.Markers.Count();

        string s = "#STR_OZ_MAP_MARKS";
        s += "  " + have.ToString();
        s += "/" + m_State.MarkerLimit.ToString();
        return s;
    }

    // Напис на кнопці -- КОЛО ГЛЯДАЧІВ, і більш нічого: рядка «транспондер
    // вимкнено» немає ні тут, ні в таблиці рядків (рішення власника
    // 2026-09-09). Кнопка, яку видно, завжди називає когось.
    private string ModeLabel(string mode)
    {
        if (mode == "public")
            return "#STR_OZ_TRANS_PUBLIC";
        if (mode == "faction")
            return "#STR_OZ_TRANS_FACTION";
        if (mode == "both")
            return "#STR_OZ_TRANS_BOTH";
        return "#STR_OZ_TRANS_CONTACTS";
    }
}
