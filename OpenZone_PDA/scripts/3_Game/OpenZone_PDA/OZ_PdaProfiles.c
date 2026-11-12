// Профілі пристроїв: що вміє конкретний КПК.
//
// Тир задає ПРЕДМЕТ, а не гравець: ПДА новачка й ПДА долговця -- різні
// класнейми з різними наборами сторінок, лімітів і живлення.
//
// ClassNames -- список, і в ньому може стояти класнейм БУДЬ-ЯКОГО мода.
// Пізнаємо пристрій звіркою obj.GetType() зі списком, і НІКОЛИ спорідненістю
// з нашим класом: інакше адмін не зміг би навести мод на чужий предмет, а
// це половина сенсу «максимально гнучкого».

class OZ_PdaLimits
{
    // ЯЧЕЙКИ ПАМ'ЯТІ, спільні на все, що прилад тримає в собі.
    //
    // Було по стелі на кожен рід -- Markers і Notes окремо. Це вимагало від
    // МОДЕЛІ ПРИЛАДУ знати наперед про кожен майбутній рід даних: поки родів
    // три й усі свої, це працює, а перший же чужий модуль опиняється або без
    // стелі, або з полем, вписаним у чужий конфіг. Рівно та сама пастка, з
    // якої вже вийшов носій.
    //
    // Тепер одне число, і ціна однакова: мітка -- ячейка, нотатка -- ячейка,
    // частота -- ячейка, маршрут -- ячейка (він лише ПОРЯДОК зв'язків між
    // мітками, а не самі мітки; тому перенести маршрут означає перенести його
    // мітки ПЛЮС його самого).
    //
    // Друзі й групові розмови сюди НЕ входять: вони не лежать у пам'яті
    // приладу, а живуть на сервері й у мосту.
    // НУЛЬ, а не число: профіль, що не оголосив пам'ять, отримує
    // задокументоване умовчання з WARNING (ТЗ-4 R-F1.1--R-F1.3), а не тихе
    // число з іншого файлу. Два числа у двох місцях -- це те, з чого все
    // почалось.
    int Memory     = 0;
    int Friends    = 20;
    int GroupChats = 2;

    // Копія в об'єкт, який зробив скрипт (шапка OZ_ConfigBase ядра).
    //
    // Ключ, якого У ФАЙЛІ НЕМАЄ, копія переносить нулем, а не числом з
    // ініціалізатора нижче: після копії «немає у файлі» й «нуль у файлі»
    // невідрізненні. Тому ініціалізатори цих трьох полів -- ДОКУМЕНТАЦІЯ, а
    // умовчання ставить Validate, і робить це для ВСІХ ТРЬОХ однаково
    // (ТЗ-4 R-F1.6): нуль -- це «ключа немає», про нього є рядок у лозі з
    // іменем профілю, і задокументоване число пишеться назад у файл.
    //
    // Чому саме так, а не «нуль = без межі»: нуль на дроті означає «без
    // межі» і в записнику (OZ_PdaContactSwap.Full), і в мосту
    // (OZ_ChatAskGroup.Max), тож забутий ключ не звужував межу, а
    // СКАСОВУВАВ її -- мовчки. Той самий хід, яким ядро закрило
    // Health01/QuickBar (ТЗ-3 R4.2).
    //
    // ЩО ЛИШАЄТЬСЯ ЗА НУЛЕМ: випадок, де його ставить не профіль, а сам
    // випадок. Засновник, у якого профілю немає взагалі, досі їде до моста
    // з Max = 0, і міст читає це як «без межі», -- саме той шлях, заради
    // якого нуль на дроті й потрібен.
    //
    // Кому потрібен записник без стелі в САМОМУ профілі -- пише велике
    // число, і воно видно у файлі, а не вгадується з відсутності рядка.
    OZ_PdaLimits Copy()
    {
        OZ_PdaLimits c = new OZ_PdaLimits();
        c.Memory     = Memory;
        c.Friends    = Friends;
        c.GroupChats = GroupChats;
        return c;
    }
}

class OZ_PdaProfile
{
    string            Id          = "";
    ref array<string> ClassNames;
    string            DisplayName = "";
    ref array<string> Pages;
    ref array<string> BatteryClassNames;
    ref OZ_PdaLimits  Limits;

    // RequiresPower, Theme і PinProtectedPages ЗНЯТІ (ревізія 2026-09-06):
    // оголошувались, валідувались і не читались ніде -- тема одна на серію
    // (ui/tokens.json ядра), а «сторінки, що просять код ще раз» ворота
    // OZ_PdaAccess не питали ніколи. Поле, яке обіцяє адмінові важіль і
    // нічого не робить, гірше за відсутнє.

    // БАЗОВА ВИТРАТА ЖИВЛЕННЯ, одиниць енергії за хвилину (ТЗ-5 R-B2.2).
    //
    // Знята тією ж ревізією разом із трьома вище -- і повернена, бо це
    // ЄДИНЕ з чотирьох, що ТЗ прямо замовляло оживити, а не прибрати.
    // Витрата приладу = це число x добуток PowerFactor вставлених плат
    // (OZ_PDA_Base.OZ_ApplyDrain). Тир нарешті означає щось і тут:
    // трислотовий прилад із трьома платами сідає помітно швидше за
    // однослотовий із порожнім гніздом.
    //
    // Нуль -- «ключа немає»: Validate поставить умовчання й скаже про це
    // вголос. Прилад, що не їсть нічого, задається дуже малим числом, а не
    // нулем: вимкнути витрату повністю означало б вічну батарею, а такого
    // ТЗ не замовляло.
    float PowerDrainPerMin = 0;

    // ДОВЖИНА ПІН-КОДУ (ТЗ-5 R-B3.3). Умовчання 4; сервер перевіряє довжину
    // й склад саме за цим числом, а пад малює за ним свої крапки. Раніше
    // чотири були правилом КЛІЄНТА, а сервер приймав будь-який рядок.
    int PinLength = 0;

    // Скільки модульних відсіків видно на цій моделі. ГОЛОВНИЙ важіль тиру:
    // один відсік змушує вибирати, яку одну плату нести, три дозволяють
    // нести все. Стеля -- MODULE_SLOTS_MAX, бо слоти не
    // додаються в рантаймі.
    //
    // Нуль -- «ключа немає» (див. OZ_PdaLimits про те, чому після Copy() це
    // невідрізненне): Validate поставить один відсік і скаже про це вголос.
    // Досі забутий ключ давав модель, у якої не працює жоден відсік, --
    // і ні рядка в лозі.
    int ModuleSlots = 1;

    // --- замок ---
    // Через скільки хвилин після того, як пристрій прибрали з рук, він
    // замикається сам.
    //
    // НУЛЬ -- «КЛЮЧА НЕМАЄ», а «ніколи» пишеться ВІД'ЄМНИМ числом (-1).
    // Нуль означав «автоблокування вимкнено», і саме через це забутий ключ
    // у профілі чужого приладу мовчки вимикав замок-таймер -- безпекову
    // річ, про яку адмін не знав, що її вимкнув. Тепер нуль лікується до
    // умовчання з рядком у лозі, а вимкнути свідомо -- це -1 у файлі.
    float LockAfterMinutes = 5;
    // Сервер забороняє гравцеві вимикати автоблокування. Для серверів, де
    // залутаний КПК має лишатись цінністю, а не безкоштовним трофеєм.
    bool  ForceAutoLock    = false;

    // --- запечатаний пристрій ---
    //
    // Це КПК із чужої історії: мертвого сталкера, кинутої схованки, квесту.
    // Він приходить у світ із кодом, якого НІХТО не знає -- сервер ставить
    // випадковий і нікому його не каже. Підібрати його не можна: він не
    // «складний», його просто не існує в жодній голові.
    //
    // Відкрити можна ЛИШЕ дешифратором, і тільки часом. Тому в такого КПК
    // цінність не в залізі, а в тому, що на ньому записано.
    bool  Sealed       = false;
    // Нуль -- «ключа немає»: миттєвий злам не режим, а забутий рядок.
    float CrackSeconds = 120;

    // Що на ньому вже записано, коли він з'явився у світі. Пишеться ОДИН раз,
    // при першій появі предмета -- інакше кожен рестарт відновлював би
    // стерті гравцем мітки.
    ref array<ref OZ_MapMarker> PresetMarkers;

    // СТОРІНКИ, ЯКІ ЧУЖИЙ МОД УЖЕ ПРОПОНУВАВ ЦЬОМУ ПРОФІЛЮ (OZ_PdaProfiles
    // .OfferPage). Пропозиція лягає в Pages РІВНО ОДИН раз: після цього
    // рішення за адміном, і сторінка, яку він прибрав, назад не повертається.
    // Без цього списку мод фракцій не мав способу дати свою вкладку жодному
    // приладу -- профілі поставки про неї не знають (сторінка поїхала в
    // склейку 2026-09-02), і на свіжому сервері вкладки не було ніде.
    ref array<string> OfferedSeen;

    // s_Cfg живе весь запуск сервера, і ForClass читає ці поля на кожному
    // запиті кожної сторінки -- через години після розбору файла.
    OZ_PdaProfile Copy()
    {
        OZ_PdaProfile c = new OZ_PdaProfile();
        c.Id               = Id;
        c.DisplayName      = DisplayName;
        c.ModuleSlots      = ModuleSlots;
        c.PowerDrainPerMin = PowerDrainPerMin;
        c.PinLength        = PinLength;
        c.LockAfterMinutes = LockAfterMinutes;
        c.ForceAutoLock    = ForceAutoLock;
        c.Sealed           = Sealed;
        c.CrackSeconds     = CrackSeconds;

        int i;
        c.ClassNames = new array<string>();
        if (ClassNames)
        {
            for (i = 0; i < ClassNames.Count(); i++)
                c.ClassNames.Insert(ClassNames[i]);
        }

        c.Pages = new array<string>();
        if (Pages)
        {
            for (i = 0; i < Pages.Count(); i++)
                c.Pages.Insert(Pages[i]);
        }

        c.BatteryClassNames = new array<string>();
        if (BatteryClassNames)
        {
            for (i = 0; i < BatteryClassNames.Count(); i++)
                c.BatteryClassNames.Insert(BatteryClassNames[i]);
        }

        if (Limits)
            c.Limits = Limits.Copy();
        else
            c.Limits = new OZ_PdaLimits();

        c.PresetMarkers = new array<ref OZ_MapMarker>();
        if (PresetMarkers)
        {
            for (i = 0; i < PresetMarkers.Count(); i++)
            {
                if (PresetMarkers[i])
                    c.PresetMarkers.Insert(PresetMarkers[i].Copy());
            }
        }

        c.OfferedSeen = new array<string>();
        if (OfferedSeen)
        {
            for (i = 0; i < OfferedSeen.Count(); i++)
                c.OfferedSeen.Insert(OfferedSeen[i]);
        }

        return c;
    }
}

// ТУТ БУВ КЛАС OZ_PdaVirtualDevice -- блок VirtualDevice у Profiles.json
// (Enabled/Pages/Factions), «КПК без предмета» (D132). Рішення власника
// 2026-09-08: вигаданого термінала немає, і конфіг про нього мовчить.
//
// СТАРІ ФАЙЛИ ЦЕЙ КЛЮЧ ЩЕ НЕСУТЬ, і це нормально: JsonFileLoader пропускає
// поле, якого немає в класі, мовчки й без помилки. Прибирати його з чужих
// Profiles.json ми не ходимо -- перший перезапис файлу (Validate щось
// полагодив) прибере його сам.
class OZ_PdaProfilesConfig : OZ_ConfigBase
{
    ref array<ref OZ_PdaProfile> Profiles;

    override int LatestVersion()
    {
        return OZ_PdaConst.SCHEMA_PROFILES;
    }

    override void LoadDefaults()
    {
        Version  = LatestVersion();
        Profiles = new array<ref OZ_PdaProfile>();

        OZ_PdaProfile p = new OZ_PdaProfile();
        p.Id          = "novice";
        p.DisplayName = "#STR_OZ_PDA_NOVICE";

        p.ClassNames = new array<string>();
        p.ClassNames.Insert("OZ_PDA_Novice");

        p.Pages = new array<string>();
        p.Pages.Insert(OZ_PdaConst.PAGE_DEVICE);
        p.Pages.Insert(OZ_PdaConst.PAGE_QUESTS);
        p.Pages.Insert(OZ_PdaConst.PAGE_CONTACTS);
        p.Pages.Insert(OZ_PdaConst.PAGE_NOTES);
        p.Pages.Insert(OZ_PdaConst.PAGE_MAP);
        p.Pages.Insert(OZ_PdaConst.PAGE_CHAT);
        p.Pages.Insert(OZ_PdaConst.PAGE_NEWS);

        p.BatteryClassNames = new array<string>();
        p.BatteryClassNames.Insert("Battery9V");

        p.Limits            = new OZ_PdaLimits();
        // Поставочні числа (ТЗ-4 R-F1.4) -- стартова точка балансу, не договір:
        // novice 25, advanced 60, sealed 10. Правляться в Profiles.json.
        p.Limits.Memory     = 25;
        p.Limits.Friends    = OZ_PdaConst.FRIENDS_DEFAULT;
        p.Limits.GroupChats = OZ_PdaConst.GROUPCHATS_DEFAULT;
        p.PowerDrainPerMin  = OZ_PdaConst.DRAIN_DEFAULT;
        p.PinLength         = OZ_PdaConst.PIN_LENGTH_DEFAULT;
        Profiles.Insert(p);

        OZ_PdaProfile a = new OZ_PdaProfile();
        a.Id          = "advanced";
        a.DisplayName = "#STR_OZ_PDA_ADVANCED";

        a.ClassNames = new array<string>();
        a.ClassNames.Insert("OZ_PDA_Advanced");

        a.Pages = new array<string>();
        a.Pages.Insert(OZ_PdaConst.PAGE_DEVICE);
        a.Pages.Insert(OZ_PdaConst.PAGE_QUESTS);
        a.Pages.Insert(OZ_PdaConst.PAGE_CONTACTS);
        a.Pages.Insert(OZ_PdaConst.PAGE_NOTES);
        a.Pages.Insert(OZ_PdaConst.PAGE_MAP);
        a.Pages.Insert(OZ_PdaConst.PAGE_CHAT);
        a.Pages.Insert(OZ_PdaConst.PAGE_NEWS);

        a.BatteryClassNames = new array<string>();
        a.BatteryClassNames.Insert("Battery9V");

        a.Limits            = new OZ_PdaLimits();
        a.Limits.Memory     = 60;
        a.Limits.Friends    = 60;
        a.Limits.GroupChats = 8;
        a.ModuleSlots       = OZ_PdaConst.MODULE_SLOTS_MAX;
        a.LockAfterMinutes  = 5;
        // Просунутий прилад їсть більше й тримає довший код: три відсіки
        // під плати -- це і три множники PowerFactor на витраті.
        a.PowerDrainPerMin  = 0.8;
        a.PinLength         = OZ_PdaConst.PIN_LENGTH_DEFAULT;
        Profiles.Insert(a);

        // Запечатаний КПК: приклад, який одразу працює, і водночас зразок
        // для моддера. Класнейм навмисно окремий -- профілі шукають за
        // класом, і «квестовий» КПК мусить бути іншим предметом, а не
        // позначкою на звичайному.
        OZ_PdaProfile q = new OZ_PdaProfile();
        q.Id          = "sealed";
        q.DisplayName = "#STR_OZ_PDA_SEALED";

        q.ClassNames = new array<string>();
        q.ClassNames.Insert("OZ_PDA_Sealed");

        q.Pages = new array<string>();
        q.Pages.Insert(OZ_PdaConst.PAGE_DEVICE);
        q.Pages.Insert(OZ_PdaConst.PAGE_MAP);
        q.Pages.Insert(OZ_PdaConst.PAGE_NOTES);

        q.BatteryClassNames = new array<string>();
        q.BatteryClassNames.Insert("Battery9V");

        q.Limits            = new OZ_PdaLimits();
        q.Limits.Memory     = 10;
        q.ModuleSlots       = 2;
        q.LockAfterMinutes  = 1;
        q.Sealed            = true;
        q.CrackSeconds      = 90;
        q.Limits.Friends    = OZ_PdaConst.FRIENDS_DEFAULT;
        q.Limits.GroupChats = OZ_PdaConst.GROUPCHATS_DEFAULT;
        q.PowerDrainPerMin  = OZ_PdaConst.DRAIN_DEFAULT;
        q.PinLength         = OZ_PdaConst.PIN_LENGTH_DEFAULT;

        q.PresetMarkers = new array<ref OZ_MapMarker>();
        Profiles.Insert(q);
    }

    // Один рядок про забутий ключ, і всі троє кажуть його однаково.
    private string LimitMissing(string id, string key, int fallback)
    {
        string m = "profile \"" + id + "\" declares no ";
        m += key + " - writing the documented ";
        m += fallback.ToString();
        return m;
    }

    // WARNINGS -- ЦЕ «Я ЩОСЬ ПОЛАГОДИВ», А НЕ «Я ЩОСЬ ПОМІТИВ».
    //
    // Лоадер ядра пише файл назад саме за цим числом
    // (OZ_ConfigLoader.Load: warnings > 0 && rewriteOnFix -> Save), і сенс
    // того запису один -- закріпити на диску те, що Validate виправив у
    // пам'яті. Скарги ж, які нічого не міняють, -- сторінка чужого мода, що
    // не зареєстрована, класнейм, якого немає в CfgVehicles, -- повторюються
    // КОЖЕН бут і кожен бут переписували файл байт у байт, разом із
    // бекапом. Стенд КПК крутив це на трьох файлах щоразу (task-57d, §2).
    //
    // Тепер warnings++ стоїть ЛИШЕ там, де об'єкт у пам'яті став іншим.
    // Наслідок: перезапис відбувається один раз -- той, що дописує в файл
    // забутий ключ, -- і на наступному буті скарги вже немає, бо ключ є.
    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Profiles)
            Profiles = new array<ref OZ_PdaProfile>();

        // ВКЛАДЕНЕ -- У СТВОРЕНЕ СКРИПТОМ, І ДО ПЕРШОГО ЖЕ ВИДІЛЕННЯ.
        //
        // Лоадер кличе Validate одразу після розбору, поки читання ще чесне;
        // усе нижче в цьому методі складає рядки попереджень і заводить
        // масиви, а s_Cfg потім живе весь запуск сервера. Null лишаємо null:
        // на нього нижче є своя скарга, яка й полагодить файл.
        for (int rs = 0; rs < Profiles.Count(); rs++)
        {
            if (Profiles[rs])
                Profiles.Set(rs, Profiles[rs].Copy());
        }

        // NULL-ЕЛЕМЕНТ -- ЗАКОННИЙ JSON, а ForClass розіменовує кожен запис
        // на кожну операцію кожної сторінки. Викидаємо ззаду наперед і до
        // всіх інших перевірок -- та сама причина, що в Hardware.json.
        for (int pn = Profiles.Count() - 1; pn >= 0; pn--)
        {
            if (Profiles[pn])
                continue;
            OZ_Log.Warn("Profiles.json has a null entry in Profiles - dropped");
            Profiles.Remove(pn);
            warnings++;
        }

        for (int i = 0; i < Profiles.Count(); i++)
        {
            OZ_PdaProfile p = Profiles[i];

            // СКАРГА, ЯКА НІЧОГО НЕ ПОЛАГОДИЛА, НЕ РАХУЄТЬСЯ (див. шапку
            // Validate): порожній ClassNames лікує адмін, а не ми.
            if (!p.ClassNames || p.ClassNames.Count() == 0)
                OZ_Log.Warn("profile \"" + p.Id + "\" has no ClassNames - nothing will ever match it");

            for (int c = 0; p.ClassNames && c < p.ClassNames.Count(); c++)
            {
                // Лише існування в конфізі гри. Спорідненість не перевіряємо
                // навмисно -- див. коментар угорі файлу.
                if (!GetGame().ConfigIsExisting("CfgVehicles " + p.ClassNames[c]))
                {
                    string wc = "profile \"" + p.Id;
                    wc += "\": class \"" + p.ClassNames[c];
                    wc += "\" is not in CfgVehicles";
                    OZ_Log.Warn(wc);
                }
            }

            if (!p.Pages)
                p.Pages = new array<string>();

            for (int g = 0; g < p.Pages.Count(); g++)
            {
                if (!OZ_PageRegistry.Has(p.Pages[g]))
                {
                    string wp = "profile \"" + p.Id;
                    wp += "\": page \"" + p.Pages[g];
                    wp += "\" is not registered";
                    OZ_Log.Warn(wp);
                }
            }

            if (!p.Limits)
                p.Limits = new OZ_PdaLimits();

            // ТРИ КЛЮЧІ LIMITS -- ОБОВ'ЯЗКОВІ, і це те, що дає нулю зміст.
            //
            // Після Copy() ключ, якого у файлі немає, читається нулем, а нуль
            // на дроті означає «без межі» -- і в записнику
            // (OZ_PdaContactSwap.Full), і в мосту (OZ_ChatAskGroup.Max). Тобто
            // забутий ключ не звужував межу, а СКАСОВУВАВ її, причому мовчки.
            //
            // Розв'язок той самий, що ядро застосувало до Health01/QuickBar
            // (ТЗ-3 R4.2): нуль у профілі трактується як відсутній ключ,
            // називається вголос і замінюється задокументованим умовчанням.
            // warnings++ тут ПОТРІБЕН: він і є той запис назад, після якого
            // ключ у файлі з'являється -- рівно ОДИН перезапис, а не кожен
            // бут. Далі файл несе всі три ключі, скарги нема, і «нуль без
            // межі» лишається чинним там, де його ставить не профіль, а сам
            // випадок: засновник без профілю досі їде до моста з Max = 0.
            if (p.Limits.Memory <= 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "Memory", OZ_PdaConst.MEMORY_DEFAULT) + " cells");
                p.Limits.Memory = OZ_PdaConst.MEMORY_DEFAULT;
                warnings++;
            }

            if (p.Limits.Friends <= 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "Friends", OZ_PdaConst.FRIENDS_DEFAULT) + " contacts");
                p.Limits.Friends = OZ_PdaConst.FRIENDS_DEFAULT;
                warnings++;
            }

            if (p.Limits.GroupChats <= 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "GroupChats", OZ_PdaConst.GROUPCHATS_DEFAULT) + " conversations");
                p.Limits.GroupChats = OZ_PdaConst.GROUPCHATS_DEFAULT;
                warnings++;
            }

            // Витрата живлення (ТЗ-5 R-B2.2) і довжина коду (R-B3.3) --
            // рівно та сама розмова про нуль, що й у Limits вище.
            if (p.PowerDrainPerMin <= 0)
            {
                string wdr = "profile \"" + p.Id + "\" declares no PowerDrainPerMin - using ";
                wdr += OZ_PdaConst.DRAIN_DEFAULT.ToString() + " per minute; set it in Profiles.json";
                OZ_Log.Warn(wdr);
                p.PowerDrainPerMin = OZ_PdaConst.DRAIN_DEFAULT;
                warnings++;
            }

            if (p.PinLength <= 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "PinLength", OZ_PdaConst.PIN_LENGTH_DEFAULT) + " digits");
                p.PinLength = OZ_PdaConst.PIN_LENGTH_DEFAULT;
                warnings++;
            }
            else if (p.PinLength < OZ_PdaConst.PIN_LENGTH_MIN || p.PinLength > OZ_PdaConst.PIN_LENGTH_MAX)
            {
                string wpl = "profile \"" + p.Id + "\" asks for a PIN of ";
                wpl += p.PinLength.ToString() + " digits; the pad draws ";
                wpl += OZ_PdaConst.PIN_LENGTH_MIN.ToString() + ".." + OZ_PdaConst.PIN_LENGTH_MAX.ToString();
                OZ_Log.Warn(wpl);
                p.PinLength = Math.Clamp(p.PinLength, OZ_PdaConst.PIN_LENGTH_MIN, OZ_PdaConst.PIN_LENGTH_MAX);
                warnings++;
            }

            if (!p.BatteryClassNames)
                p.BatteryClassNames = new array<string>();
            if (!p.PresetMarkers)
                p.PresetMarkers = new array<ref OZ_MapMarker>();
            if (!p.OfferedSeen)
                p.OfferedSeen = new array<string>();

            // NULL-МІТКА В ПРЕСЕТІ -- законний JSON, і посів її розіменовує
            // (OZ_PDA_Base.OZ_SeedFromProfile). Викидаємо тут, щоб предмет
            // не помирав під час появи у світі.
            for (int pm = p.PresetMarkers.Count() - 1; pm >= 0; pm--)
            {
                if (p.PresetMarkers[pm])
                    continue;
                OZ_Log.Warn("profile \"" + p.Id + "\" has a null entry in PresetMarkers - dropped");
                p.PresetMarkers.Remove(pm);
                warnings++;
            }

            // ТРИ ПОЛЯ МОДЕЛІ -- та сама розмова про нуль, що й у Limits:
            // після Copy() забутий ключ читається нулем, і кожен із цих нулів
            // мав зміст, якого ніхто не замовляв -- модель без жодного
            // робочого відсіку, миттєвий злам, вимкнений замок-таймер.
            if (p.CrackSeconds <= 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "CrackSeconds", 120) + " seconds");
                p.CrackSeconds = 120;
                warnings++;
            }

            if (p.ModuleSlots <= 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "ModuleSlots", 1) + " bay");
                p.ModuleSlots = 1;
                warnings++;
            }
            else if (p.ModuleSlots > OZ_PdaConst.MODULE_SLOTS_MAX)
            {
                string wm = "profile \"" + p.Id;
                wm += "\" asks for " + p.ModuleSlots.ToString();
                wm += " module bays; the config declares at most ";
                wm += OZ_PdaConst.MODULE_SLOTS_MAX.ToString();
                OZ_Log.Warn(wm);
                p.ModuleSlots = OZ_PdaConst.MODULE_SLOTS_MAX;
                warnings++;
            }

            // Від'ємне -- свідоме «ніколи» (див. поле), його не чіпаємо.
            if (p.LockAfterMinutes == 0)
            {
                OZ_Log.Warn(LimitMissing(p.Id, "LockAfterMinutes", 5) + " minutes; write -1 to switch the auto-lock off on purpose");
                p.LockAfterMinutes = 5;
                warnings++;
            }
        }

        // ОДИН КЛАС -- ОДИН ПРОФІЛЬ. ForClass віддає перший збіг і мовчить про
        // решту, тож клас, вписаний у два профілі, працював би за тим, що вище
        // у файлі, а адмін правив би нижній. Скарга нічого не лагодить (хто
        // правий -- знає лише адмін), тому warnings тут не росте.
        array<string> seenCls = new array<string>();
        array<string> seenIn  = new array<string>();
        for (int d = 0; d < Profiles.Count(); d++)
        {
            OZ_PdaProfile dp = Profiles[d];
            for (int dc = 0; dp.ClassNames && dc < dp.ClassNames.Count(); dc++)
            {
                string cls = dp.ClassNames[dc];
                int was = seenCls.Find(cls);
                if (was == -1)
                {
                    seenCls.Insert(cls);
                    seenIn.Insert(dp.Id);
                    continue;
                }

                string wd = "class \"" + cls + "\" is in profile \"" + seenIn[was];
                wd += "\" and again in \"" + dp.Id + "\" - only the first one is used";
                OZ_Log.Warn(wd);
            }
        }
    }
}

class OZ_PdaProfiles
{
    private static ref OZ_PdaProfilesConfig s_Cfg;

    static OZ_PdaProfilesConfig Get()
    {
        return s_Cfg;
    }

    static int Count()
    {
        if (!s_Cfg)
            return 0;
        return s_Cfg.Profiles.Count();
    }

    // Чи можна писати файл назад: false -- лоадер його не зрозумів і не
    // зміг відкласти, і перезапис дефолтами знищив би чуже.
    private static bool s_Writable = false;

    // Покоління конфіга -- та сама ручка, що в OZ_PdaHardware: ввімкнений
    // прилад звіряє його в OnWork і перераховує витрату (база профілю,
    // відсіки) без вимикання.
    private static int s_Gen = 0;

    static int Gen() { return s_Gen; }

    // rewrite = false -- the read of "reload every module" (OZ_PdaFacts.Reload): neither
    // Validate's fixes nor a page another mod offers are written; both stay in memory
    // until a load that writes.
    static void ServerLoad(bool rewrite = true)
    {
        s_Cfg = new OZ_PdaProfilesConfig();
        s_Writable = OZ_ConfigLoader<OZ_PdaProfilesConfig>.Load(OZ_PdaConst.PROFILES, "Profiles", s_Cfg, true, rewrite);

        // Пропозиції, що прийшли ДО завантаження (порядок OnMissionStart між
        // модами не гарантований), лягають тут.
        ApplyOffers(rewrite);

        s_Gen++;
    }

    // ------------------------------------------------ сторінки чужих модів
    //
    // Мод, що приносить сторінку, просить поставити її на прилади поруч із
    // `afterPageId`. Профіль, у якому такої якірної сторінки немає, пропозицію
    // бачить, але не бере: квестовий прилад без контактів не мусить отримати
    // вкладку фракції.
    //
    // КОЖЕН ПРОФІЛЬ БАЧИТЬ ПРОПОЗИЦІЮ ОДИН РАЗ (OZ_PdaProfile.OfferedSeen):
    // далі сторінка -- рішення адміна, і прибрана ним назад не приходить.
    //
    // Статики -- ЛІНИВО, а не `new` у декларації: на клієнті такий
    // ініціалізатор уже раз не виконався (OZ_PdaRoute у худі, 2026-08-30).
    private static ref array<string> s_OfferPage;
    private static ref array<string> s_OfferAfter;

    static void OfferPage(string pageId, string afterPageId)
    {
        if (pageId == "")
            return;

        if (!s_OfferPage)
        {
            s_OfferPage  = new array<string>();
            s_OfferAfter = new array<string>();
        }

        if (s_OfferPage.Find(pageId) == -1)
        {
            s_OfferPage.Insert(pageId);
            s_OfferAfter.Insert(afterPageId);
        }

        ApplyOffers();
    }

    private static void ApplyOffers(bool write = true)
    {
        if (!s_Cfg || !s_Cfg.Profiles || !s_OfferPage)
            return;

        bool changed = false;

        for (int i = 0; i < s_Cfg.Profiles.Count(); i++)
        {
            OZ_PdaProfile p = s_Cfg.Profiles[i];
            if (!p || !p.Pages)
                continue;
            if (!p.OfferedSeen)
                p.OfferedSeen = new array<string>();

            for (int o = 0; o < s_OfferPage.Count(); o++)
            {
                string page = s_OfferPage[o];
                if (p.OfferedSeen.Find(page) != -1)
                    continue;

                p.OfferedSeen.Insert(page);
                changed = true;

                if (p.Pages.Find(page) != -1)
                    continue;

                int anchor = p.Pages.Find(s_OfferAfter[o]);
                if (anchor == -1)
                    continue;

                p.Pages.InsertAt(page, anchor + 1);
                OZ_Log.Info("profile \"" + p.Id + "\": page \"" + page + "\" added after \"" + s_OfferAfter[o] + "\" at its mod's request");
            }
        }

        if (changed && s_Writable && write)
            OZ_ConfigLoader<OZ_PdaProfilesConfig>.Save(OZ_PdaConst.PROFILES, "Profiles", s_Cfg);
    }

    // --------------------------------------------- відсіки, які бачить клієнт
    //
    // Інвентар гри питає про видимість гнізда КЛІЄНТА
    // (OZ_PDA_Base.CanDisplayAttachmentSlot), а профілі живуть лише на
    // сервері: без цього клієнт показував три відсіки будь-якому приладу, і
    // плата, покладена в «зайвий», мовчки не працювала. Число їде в пакеті
    // синхронізації ядра (OZ_PdaConst.SYNC_SLOTS) рядком "<клас>:<число>;".
    private static ref map<string, int> s_ClientSlots;

    // Сервер: усі класи всіх профілів одним рядком. Значення в пакеті ядра
    // ріжеться розбирачем на 1023 байтах, тож хвіст понад тисячу не їде: клас,
    // що не доїхав, клієнт показує з усіма відсіками, як і досі, а сервер
    // однаково не прийме плату в схований відсік.
    static string PackSlots()
    {
        string s = "";
        if (!s_Cfg || !s_Cfg.Profiles)
            return s;

        for (int i = 0; i < s_Cfg.Profiles.Count(); i++)
        {
            OZ_PdaProfile p = s_Cfg.Profiles[i];
            if (!p || !p.ClassNames)
                continue;

            for (int c = 0; c < p.ClassNames.Count(); c++)
            {
                string one = p.ClassNames[c] + ":" + p.ModuleSlots.ToString() + ";";
                if (s.Length() + one.Length() > 1000)
                {
                    OZ_Log.Warn("pda: too many device classes to tell the client their bays - the rest shows every bay");
                    return s;
                }
                s += one;
            }
        }
        return s;
    }

    // Клієнт: розібрати рядок із пакета.
    static void ApplyClientSlots(string packed)
    {
        if (!s_ClientSlots)
            s_ClientSlots = new map<string, int>();
        s_ClientSlots.Clear();

        array<string> parts = new array<string>();
        packed.Split(";", parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            string part = parts[i];
            int at = part.IndexOf(":");
            if (at <= 0)
                continue;

            string cls = part.Substring(0, at);
            int n = part.Substring(at + 1, part.Length() - at - 1).ToInt();
            s_ClientSlots.Set(cls, n);
        }
    }

    // Статик переживає місію: наступний сервер не мусить успадкувати наші
    // числа.
    static void ForgetClientSlots()
    {
        if (s_ClientSlots)
            s_ClientSlots.Clear();
    }

    // Скільки відсіків у цього класу -- з ОБОХ боків. Сервер питає профіль,
    // клієнт -- те, що приїхало в пакеті. Класу, про який не знає ніхто,
    // відповідає стеля: так поводився прилад і до цієї правки.
    static int ModuleSlotsOf(string cls)
    {
        if (s_Cfg)
        {
            OZ_PdaProfile p = ForClass(cls);
            if (p)
                return p.ModuleSlots;
            return OZ_PdaConst.MODULE_SLOTS_MAX;
        }

        int n;
        if (s_ClientSlots && s_ClientSlots.Find(cls, n))
            return n;
        return OZ_PdaConst.MODULE_SLOTS_MAX;
    }

    // Класи, про які ми вже поскаржились. Скарга потрібна ОДНА на клас за
    // сеанс: цю функцію питають на кожну операцію кожної сторінки, і без
    // пам'яті вона залила б лог тим самим рядком.
    private static ref array<string> s_Unknown;

    static OZ_PdaProfile ForClass(string cls)
    {
        if (!s_Cfg)
            return null;

        for (int i = 0; i < s_Cfg.Profiles.Count(); i++)
        {
            if (!s_Cfg.Profiles[i])
                continue;
            array<string> names = s_Cfg.Profiles[i].ClassNames;
            for (int c = 0; names && c < names.Count(); c++)
            {
                if (names[c] == cls)
                    return s_Cfg.Profiles[i];
            }
        }

        // КЛАС НАЗИВАЄТЬСЯ ВГОЛОС.
        //
        // Прилад без запису в OZ_PDA_Profiles.json не робить нічого: сторінок
        // немає, стелі немає, живлення немає. Мовчазний null лишав адміна з
        // приладом, який просто «не працює», і жодної підказки, що виправити
        // треба ім'я класу в конфізі.
        if (!s_Unknown)
            s_Unknown = new array<string>();

        if (s_Unknown.Find(cls) == -1)
        {
            s_Unknown.Insert(cls);
            OZ_Log.Warn("pda profile: class \"" + cls + "\" is in no profile of OZ_PDA_Profiles.json - the device will do nothing");
        }

        return null;
    }
}
