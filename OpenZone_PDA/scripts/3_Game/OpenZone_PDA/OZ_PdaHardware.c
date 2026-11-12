// Залізо, яке вставляється в КПК.
//
// Три різні речі, а не один список слотів:
//
//   ЖИВЛЕННЯ   -- батарея. Слот ванільний, рушій сам втикає в неї пристрій.
//   СХОВИЩЕ    -- носій даних. Свій слот: це вміст, а не здатність.
//   МОДУЛІ     -- GPS (із транспондером), шпигунська плата, дешифратор і
//                 що завгодно від інших модів. Ділять ОБМЕЖЕНЕ число відсіків.
//
// Відсіків обмежено навмисно: це головний важіль тиру. У ПДА новачка один, у
// долговського три -- і гравець сам вирішує, яку одну плату нести. Список
// слотів без обмеження такого вибору не дає. (Радіометр і дозиметр зняті
// рішенням власника 2026-09-09.)
//
// Що саме влазить у відсік і що це дає -- таблиця класнеймів нижче. Пізнаємо
// ЗВІРКОЮ КЛАСНЕЙМА, ніколи спорідненістю: тоді модуль може принести будь-який
// мод, а адмін -- навести КПК на предмет, про який ми не чули.

class OZ_ModuleSpec
{
    string ClassName   = "";
    string DisplayName = "";

    // ХТО ВОЛОДІЄ ЗАПИСОМ (D94, друга половина). Порожньо -- запис адміна:
    // файл головніший, чуже оголошення того самого класу його не чіпає.
    // Непорожньо -- запис приніс інший мод (Declare ставить назву мода або
    // "mod"): він потрапляє у файл, щоб адмін його БАЧИВ у редакторі, але
    // належить модові -- наступне оголошення того ж класу його замінює.
    // Хочеш перекрити чужий модуль назавжди -- зітри Origin: запис стає
    // твоїм, і файл знову виграє.
    string Origin = "";

    // Що це за прилад. Рядком, а не числом: мод-постачальник не мусить знати
    // наших констант, а адмін бачить у JSON слово, а не код.
    //
    //   "gps"         -- прилад знає, де він, і веде транспондер; дає радіус
    //   ("radiometer" і "dosimeter" знято 2026-09-09 -- запис із таким родом
    //    у файлі лише попереджає, а клас без мода прибирається зачисткою)
    //   "spy"         -- шпигунське око на чужі транспондери, на лічені хвилини
    //   будь-що інше  -- чужий модуль, КПК просто вмикає його сторінки
    //
    // ВИДУ "antenna" БІЛЬШЕ НЕМАЄ: рішення власника 2026-09-09 злило антену
    // з GPS в один модуль. Запис зі старим словом лишається у файлі, але не
    // означає нічого -- Validate каже про це вголос.
    string Kind = "";

    // ШПИГУНСЬКИЙ транспондер: скільки хвилин активної роботи в платі.
    // Нуль -- звичайний модуль. Вичерпаний ресурс СПАЛЮЄ плату, як
    // дешифратор: одноразова розкіш, не вічне око.
    float SpyMinutes = 0;

    // Радіус упевненого прийому транспондера в метрах. Має сенс лише для
    // "gps": прилад, який не знає, де він, не має чого вести. Нуль у записі
    // GPS Validate замінює поставочним OZ_PdaConst.GPS_RANGE_DEFAULT --
    // причина в коментарі при тій константі. У записі виду "spy" число не
    // означає нічого, і Validate його обнуляє.
    float RangeM = 0;

    // У СКІЛЬКИ РАЗІВ ця плата піднімає витрату живлення приладу
    // (ТЗ-5 R-B2.2). Витрата = PowerDrainPerMin профілю x добуток цього
    // числа по всіх вставлених платах; рахує OZ_PDA_Base.OZ_ApplyDrain на
    // кожному вставленні, витягуванні й вмиканні.
    //
    // Було інертним полем: жодне місце коду його не читало, тож адмін
    // піднімав множник у Hardware.json чи у формі VPP і не бачив у грі
    // нічого (знахідка 98, задача 52). Одиниця -- «нічого не додає».
    float PowerFactor = 1.0;

    // Які сторінки модуль вмикає. Антена вмикає "radio", радіометр -- свою
    // шкалу; поле загальне, щоб чужий модуль міг увімкнути свою сторінку.
    ref array<string> EnablesPages;

    // Копія в об'єкт, який зробив скрипт (шапка OZ_ConfigBase ядра). s_Cfg
    // живе весь запуск сервера, і ModuleFor читає ці поля на кожному
    // під'єднанні модуля -- через години після розбору файла.
    OZ_ModuleSpec Copy()
    {
        OZ_ModuleSpec c = new OZ_ModuleSpec();
        c.ClassName   = ClassName;
        c.DisplayName = DisplayName;
        c.Origin      = Origin;
        c.Kind        = Kind;
        c.SpyMinutes  = SpyMinutes;
        c.RangeM      = RangeM;
        c.PowerFactor = PowerFactor;

        c.EnablesPages = new array<string>();
        if (EnablesPages)
        {
            for (int i = 0; i < EnablesPages.Count(); i++)
                c.EnablesPages.Insert(EnablesPages[i]);
        }

        return c;
    }
}

class OZ_CarrierSpec
{
    string ClassName   = "";
    string DisplayName = "";
    // Чи можна перезаписати носій із КПК. Одноразовий чип із чужої схованки
    // перезаписувати не можна -- у цьому половина його цінності.
    bool   Writable    = true;
    // Місткість -- ОДНЕ число, в записах, байдуже яких. Мітка -- запис,
    // нотатка -- запис, точка маршруту -- запис, частота -- запис. 0 -- без
    // стелі.
    //
    // Раніше стелі були окремі на кожен відомий рід, і саме це робило носій
    // закритим: щоб мод поклав на нього своє, він мусив би прописати собі
    // стелю в КОЖНОМУ класі носія, а автор носія -- знати всі майбутні роди.
    // Спільний лічильник знімає обидві половини цієї вимоги.
    //
    // Другий важіль тиру після відсіків: дискета на шість записів і польовий
    // накопичувач -- різні речі за той самий слот.
    int    MaxRecords  = 0;

    // ЦІНА КОПІЇ, ЯКУ ТУТ ВИДНО НАЙКРАЩЕ: ключа, якого У ФАЙЛІ НЕМАЄ, копія
    // переносить нулем, а не значенням з ініціалізатора, -- отже носій без
    // "Writable" стане НЕперезаписуваним. Відрізнити «немає» від «нуль»
    // Validate не може, і вгадувати на користь true не має права: одноразовий
    // чип, який раптом можна переписати, втрачає половину своєї цінності.
    // Файл, який пише сам мод, несе всі ключі; це про обрізаний вручну.
    OZ_CarrierSpec Copy()
    {
        OZ_CarrierSpec c = new OZ_CarrierSpec();
        c.ClassName   = ClassName;
        c.DisplayName = DisplayName;
        c.Writable    = Writable;
        c.MaxRecords  = MaxRecords;
        return c;
    }
}

class OZ_PdaHardwareConfig : OZ_ConfigBase
{
    ref array<ref OZ_ModuleSpec>  Modules;
    ref array<ref OZ_CarrierSpec> Carriers;

    override int LatestVersion()
    {
        return 1;
    }

    override void LoadDefaults()
    {
        Version  = LatestVersion();
        Modules  = new array<ref OZ_ModuleSpec>();
        Carriers = new array<ref OZ_CarrierSpec>();

        // РАДІОМЕТРА Й ДОЗИМЕТРА ТУТ БІЛЬШЕ НЕМАЄ (рішення власника
        // 2026-09-09). Обидві плати були екранами для чисел, які дає лише мод
        // радіації, і без такого мода стояли з чесним «немає даних» -- відсік
        // з'їдений, користі нуль. Радіація пішла з ними цілком: договору
        // OZ_PdaRadiation у моді теж немає. Мод, якому вона потрібна, оголосить
        // свою плату і свою сторінку так само, як це робить рація.
        //
        // GPS -- ОДИН МОДУЛЬ НА ДВІ КОЛИШНІ ПЛАТИ (рішення власника
        // 2026-09-09). Без нього прилад НЕ ЗНАЄ, де він (ТЗ-4 R-B2.2): мітки
        // й маршрут працюють, а «ти тут», відстані й транспондер -- ні. Окрема
        // «антена» тут була другою половиною тієї самої вимоги: без неї
        // прилад теж не вів нічого, отже вибору між ними не було, а відсік
        // з'їдали два.
        //
        // Транспондер -- функція самого пристрою, і ставити його в залежність
        // від мода рації означало б віддати КПК без того, заради чого його
        // носять. Далекі вежі приносить OpenZone Radio; вони просто мають
        // більший RangeM.
        OZ_ModuleSpec gps = new OZ_ModuleSpec();
        gps.ClassName    = "OZ_Module_GPS";
        gps.DisplayName  = "#STR_OZ_MOD_GPS";
        gps.Kind         = OZ_PdaConst.MOD_GPS;
        gps.RangeM       = OZ_PdaConst.GPS_RANGE_DEFAULT;
        // Приймач плюс передавач їдять більше за будь-який датчик, і це має
        // бути видно по батареї.
        gps.PowerFactor  = 1.6;
        gps.EnablesPages = new array<string>();
        Modules.Insert(gps);

        // Шпигунська плата ЗАПИСАНА в залізі (ТЗ-4 R-F4.1): предмет спавнився,
        // а запису не мав -- і з коробки не робив нічого. Свій вид, а не
        // чужий: дальності вона не дає й не потребує, а робить одне -- лічені
        // хвилини ока на всіх (SpyMinutes), потім плата згорає.
        OZ_ModuleSpec spy = new OZ_ModuleSpec();
        spy.ClassName    = "OZ_Module_SpyAntenna";
        spy.DisplayName  = "#STR_OZ_MOD_SPY";
        spy.Kind         = OZ_PdaConst.MOD_SPY;
        spy.SpyMinutes   = 60;
        spy.PowerFactor  = 2.0;
        spy.EnablesPages = new array<string>();
        Modules.Insert(spy);

        // Дешифратор. Відкриває запечатані КПК -- і тільки їх; на звичайному
        // пристрої він просто займає відсік. Їсть багато: він рахує.
        OZ_ModuleSpec dec = new OZ_ModuleSpec();
        dec.ClassName    = "OZ_Module_Decryptor";
        dec.DisplayName  = "#STR_OZ_MOD_DECRYPTOR";
        dec.Kind         = "decryptor";
        dec.PowerFactor  = 2.0;
        dec.EnablesPages = new array<string>();
        Modules.Insert(dec);

        // Три класи -- три місткості. Числа СТЕНДОВІ: баланс задає адмін
        // у Hardware.json, а не цей файл.
        OZ_CarrierSpec floppy = new OZ_CarrierSpec();
        floppy.ClassName   = "OZ_DataCarrier_Floppy";
        floppy.DisplayName = "#STR_OZ_CARRIER_FLOPPY";
        floppy.Writable    = true;
        floppy.MaxRecords  = 6;
        Carriers.Insert(floppy);

        OZ_CarrierSpec chip = new OZ_CarrierSpec();
        chip.ClassName   = "OZ_DataCarrier_Chip";
        chip.DisplayName = "#STR_OZ_CARRIER_CHIP";
        chip.Writable    = true;
        chip.MaxRecords  = 24;
        Carriers.Insert(chip);

        OZ_CarrierSpec drive = new OZ_CarrierSpec();
        drive.ClassName   = "OZ_DataCarrier_Drive";
        drive.DisplayName = "#STR_OZ_CARRIER_DRIVE";
        drive.Writable    = true;
        Carriers.Insert(drive);
    }

    // Індекси-дублікати в переліку класнеймів, ЗЗАДУ НАПЕРЕД -- саме в тому
    // порядку, у якому їх можна видаляти, не зсуваючи решту.
    //
    // Одна петля на обидва масиви: правило однакове, а два її списки жили
    // поруч і розходилися б від першої ж правки в одному з них.
    private void DupsBack(array<string> names, array<int> outDrop)
    {
        for (int d = names.Count() - 1; d >= 0; d--)
        {
            for (int f = 0; f < d; f++)
            {
                if (names[f] == names[d])
                {
                    outDrop.Insert(d);
                    break;
                }
            }
        }
    }

    // WARNINGS -- ЦЕ «Я ЩОСЬ ПОЛАГОДИВ», А НЕ «Я ЩОСЬ ПОМІТИВ». Те саме
    // правило й та сама причина, що в OZ_PdaProfilesConfig.Validate: лоадер
    // ядра пише файл назад саме за цим числом, і скарга, після якої об'єкт
    // у пам'яті не змінився, переписувала Hardware.json байт у байт на
    // кожному буті (task-57d, §2).
    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Modules)
            Modules = new array<ref OZ_ModuleSpec>();
        if (!Carriers)
            Carriers = new array<ref OZ_CarrierSpec>();

        // ВКЛАДЕНЕ -- У СТВОРЕНЕ СКРИПТОМ, І ДО ПЕРШОГО ЖЕ ВИДІЛЕННЯ.
        //
        // Лоадер кличе Validate одразу після розбору, тобто поки читання ще
        // чесне; далі в цьому ж методі складаються попередження й заводяться
        // масиви, а s_Cfg потім живе весь запуск сервера. Null лишаємо null:
        // на нього нижче є своя скарга, яка й полагодить файл.
        int rs;
        for (rs = 0; rs < Modules.Count(); rs++)
        {
            if (Modules[rs])
                Modules.Set(rs, Modules[rs].Copy());
        }
        for (rs = 0; rs < Carriers.Count(); rs++)
        {
            if (Carriers[rs])
                Carriers.Set(rs, Carriers[rs].Copy());
        }

        // NULL-ЕЛЕМЕНТ -- ЦІЛКОМ ЗАКОННИЙ JSON, і саме він валив усе далі.
        //
        // `"Modules": [null]` розбирається без помилки, а перша ж перевірка
        // нижче розіменовує його -- і сервер отримує виняток не на буті, а на
        // кожному запиті, який спитає ModuleFor. Викидаємо ззаду наперед, до
        // будь-якої іншої перевірки: після цього ніхто в цьому файлі не мусить
        // питати про null.
        for (int mn = Modules.Count() - 1; mn >= 0; mn--)
        {
            if (Modules[mn])
                continue;
            OZ_Log.Warn("Hardware.json has a null entry in Modules - dropped");
            Modules.Remove(mn);
            warnings++;
        }
        for (int cn = Carriers.Count() - 1; cn >= 0; cn--)
        {
            if (Carriers[cn])
                continue;
            OZ_Log.Warn("Hardware.json has a null entry in Carriers - dropped");
            Carriers.Remove(cn);
            warnings++;
        }

        // ДУБЛІКАТ КЛАСНЕЙМА ВІДКИДАЄМО, і кажемо про це вголос.
        //
        // Пошук завжди повертає ПЕРШИЙ запис, тож другий не працює ніколи --
        // а форма в адмінці показує його як збережений. Адмін правив другий,
        // бачив його і в списку, і у файлі, і не розумів, чому прилад
        // поводиться по-старому. Лишаємо саме перший -- той, який і працює.
        array<string> mnames = new array<string>();
        for (int mi = 0; mi < Modules.Count(); mi++)
            mnames.Insert(Modules[mi].ClassName);

        array<int> mdrop = new array<int>();
        DupsBack(mnames, mdrop);
        for (int md = 0; md < mdrop.Count(); md++)
        {
            OZ_Log.Warn("module \"" + mnames[mdrop[md]] + "\" is declared twice in Hardware.json - only the first entry ever worked, the later one is dropped");
            Modules.Remove(mdrop[md]);
            warnings++;
        }

        // НАШІ ВЛАСНІ МЕРТВІ ЗАПИСИ ВИКИДАЄМО, і лише їх.
        //
        // Трьох наших плат більше немає (рішення власника 2026-09-09): антени,
        // радіометра й дозиметра. Рядки про них у файлі -- наш же слід: їх
        // поклав туди LoadDefaults попередніх версій. Класів немає в жодному
        // моді, види "antenna", "radiometer" і "dosimeter" не означають нічого,
        // і лишити рядки означало б скаржитись на них КОЖЕН бут.
        //
        // ПЕРЕЛІК ІМЕНАМИ, А НЕ ПЕРЕВІРКОЮ ConfigIsExisting. Виклик є й дешевий
        // -- він стоїть нижче в цьому ж методі, -- але відповідає на інше
        // питання: «класу немає в ЗАВАНТАЖЕНИХ конфігах». Це рівно те, що каже
        // плата чужого мода, якого на цьому стенді немає навмисно (у нас це
        // OZ_Module_Radio, і стенд КПК крутиться саме так). Викидати її означало
        // б стирати чужий запис адміна щоразу, коли той мод не піднявся, --
        // тобто мовчки псувати файл при частковому наборі модів. Перелік
        // натомість називає те, що ми ЗНАЄМО мертвим назавжди: наші власні
        // класи, які ми самі й прибрали.
        //
        // Ззаду наперед -- як і решта викидань у цьому методі.
        array<string> retired = new array<string>();
        retired.Insert("OZ_Module_Antenna");
        retired.Insert("OZ_Module_Radiometer");
        retired.Insert("OZ_Module_Dosimeter");

        for (int an = Modules.Count() - 1; an >= 0; an--)
        {
            if (retired.Find(Modules[an].ClassName) == -1)
                continue;
            OZ_Log.Warn("module \"" + Modules[an].ClassName + "\" no longer exists in this mod - the entry is dropped");
            Modules.Remove(an);
            warnings++;
        }

        // Локальна копія числа: ToString() кличемо на змінній, а не на
        // виразі доступу до статичної константи.
        float gpsRange = OZ_PdaConst.GPS_RANGE_DEFAULT;

        for (int i = 0; i < Modules.Count(); i++)
        {
            OZ_ModuleSpec m = Modules[i];

            // СКАРГА БЕЗ ПОЧИНКИ НЕ РАХУЄТЬСЯ (див. шапку Validate): плату
            // з непідвантаженого мода лікує склад сервера, а не перезапис
            // файла нашою ж копією.
            if (!GetGame().ConfigIsExisting("CfgVehicles " + m.ClassName))
                OZ_Log.Warn("module class \"" + m.ClassName + "\" is not in CfgVehicles - is its mod loaded?");

            if (!m.EnablesPages)
                m.EnablesPages = new array<string>();

            if (m.Kind == "")
                OZ_Log.Warn("module \"" + m.ClassName + "\" has no Kind - it will attach but do nothing");

            // НАША шпигунська плата дістає СВІЙ вид (рішення власника
            // 2026-09-09). Ремонт по класнейму, а не по виду взагалі: сам
            // клас наш, ми знаємо, чим він мусить бути, а чужа плата з
            // будь-яким видом лишається такою, якою її оголосили.
            if (m.ClassName == "OZ_Module_SpyAntenna" && m.Kind != OZ_PdaConst.MOD_SPY)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" had Kind \"" + m.Kind + "\", the spy plate now has its own kind - set to \"" + OZ_PdaConst.MOD_SPY + "\"");
                m.Kind = OZ_PdaConst.MOD_SPY;
                warnings++;
            }

            // ВИДИ "antenna", "radiometer" І "dosimeter" МЕРТВІ, і мовчати про
            // це не можна: запис із таким видом лишається у файлі, виглядає
            // робочим і не робить нічого. Скарга БЕЗ ПОЧИНКИ (див. шапку
            // Validate): що робити з чужим записом -- вирішує адмін, а не ми за
            // нього. Наші власні три класи сюди вже не доходять -- їх викинуто
            // вище за класнеймом.
            if (m.Kind == "antenna")
                OZ_Log.Warn("module \"" + m.ClassName + "\" has Kind \"antenna\", which no longer exists - the transponder range comes from the GPS module now, and this entry does nothing");

            if (m.Kind == "radiometer" || m.Kind == "dosimeter")
                OZ_Log.Warn("module \"" + m.ClassName + "\" has Kind \"" + m.Kind + "\", which no longer exists - radiation left the PDA on 2026-09-09, and this entry does nothing");

            if (m.RangeM < 0)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" has a negative RangeM, clamped to 0");
                m.RangeM = 0;
                warnings++;
            }

            // ДАЛЬНІСТЬ GPS БЕЗ ЧИСЛА -- це забутий ключ, а не «нікому не
            // чути»: після Copy() відсутнє поле читається нулем. Та сама
            // пастка, що в PowerFactor нижче, і те саме лікування.
            if (m.Kind == OZ_PdaConst.MOD_GPS && m.RangeM <= 0)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" has no usable RangeM for a GPS, set to " + gpsRange.ToString());
                m.RangeM = gpsRange;
                warnings++;
            }

            // ШПИГУНСЬКА ПЛАТА ДАЛЬНОСТІ НЕ МАЄ, і число в її рядку -- слід
            // мертвого виду "antenna": дальність маяка тепер бере лише GPS
            // (TransponderRange відсіює все, крім виду gps), тож для виду spy
            // це поле не читає ніхто. Мовчки лишити його не можна: у формі
            // адмінки воно виглядає робочим, і адмін крутить число, яке ні на
            // що не впливає. Обнуляємо ЯК ПОЧИНКУ, зі збільшенням warnings --
            // файл перепишеться один раз, а наступний бут пройде мовчки.
            if (m.Kind == OZ_PdaConst.MOD_SPY && m.RangeM > 0)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" is a spy plate, which has no range of its own - RangeM cleared to 0");
                m.RangeM = 0;
                warnings++;
            }

            // <= 0, а не < 0: після копії ключ, якого у файлі немає, читається
            // НУЛЕМ, а не одиницею з ініціалізатора, і множник живлення нуль
            // не означає нічого. Справжнє умовчання виставляє саме Validate.
            if (m.PowerFactor <= 0)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" has no usable PowerFactor, set to 1");
                m.PowerFactor = 1.0;
                warnings++;
            }
        }

        // Носії -- те саме правило й та сама петля: пошук бере перший, отже
        // другий мертвий.
        array<string> cnames = new array<string>();
        for (int ci = 0; ci < Carriers.Count(); ci++)
            cnames.Insert(Carriers[ci].ClassName);

        array<int> cdrop = new array<int>();
        DupsBack(cnames, cdrop);
        for (int cx = 0; cx < cdrop.Count(); cx++)
        {
            OZ_Log.Warn("carrier \"" + cnames[cdrop[cx]] + "\" is declared twice in Hardware.json - only the first entry ever worked, the later one is dropped");
            Carriers.Remove(cdrop[cx]);
            warnings++;
        }

        for (int c = 0; c < Carriers.Count(); c++)
        {
            if (!GetGame().ConfigIsExisting("CfgVehicles " + Carriers[c].ClassName))
                OZ_Log.Warn("carrier class \"" + Carriers[c].ClassName + "\" is not in CfgVehicles - is its mod loaded?");
        }
    }
}

class OZ_PdaHardware
{
    private static ref OZ_PdaHardwareConfig s_Cfg;

    // ЧИ МОЖНА ПИСАТИ У ФАЙЛ. Load() відповідає false, коли на диску лежить
    // єдиний примірник, якого він не зрозумів і не зміг винести в карантин.
    // Досі відповідь ігнорувалась, і найближче чуже оголошення переписувало
    // той файл дефолтами плюс свій модуль -- разом із усім, що адмін туди
    // вписав. Копія ядра цього правила -- OZ_Settings.Writable().
    private static bool s_Writable = true;

    // Черга чужих оголошень.
    //
    // ПОРЯДОК МОДУЛІВ CF НЕ ГАРАНТОВАНИЙ. Виміряно на стенді: мод рації
    // отримав OnMissionStart РАНІШЕ за КПК, конфіг заліза ще не був
    // завантажений, і всі його оголошення пішли в нікуди -- тихо, з
    // «modules=0» у власному ж рядку готовності.
    //
    // Тому Declare нічого не вимагає від порядку: якщо конфіга ще немає,
    // оголошення чекає в черзі, а ServerLoad його забирає.
    //
    // ЧЕРГА НЕ ЧИСТИТЬСЯ ПІСЛЯ ПЕРШОГО ЗАВАНТАЖЕННЯ, і це головне в ній.
    //
    // Раніше вона обнулялась, і це ламало гаряче застосування заліза з
    // вкладки VPP: адмін тиснув SAVE, ServerLoad перечитував файл із диска
    // -- а чужі оголошення жили ЛИШЕ в пам'яті, і після перечитування їх
    // не було. Плата рації переставала впізнаватись до найближчого
    // рестарту, і жодного рядка про це ніде.
    //
    // Тепер список -- це пам'ять про все, що оголосили чужі моди за цей
    // запуск, і ServerLoad накладає його поверх файла КОЖНОГО разу. Адмін
    // лишається головнішим: Insert не чіпає клас, який уже є у файлі.
    private static ref array<ref OZ_ModuleSpec> s_Declared;

    // ПОКОЛІННЯ КОНФІГА: росте з кожним завантаженням і з кожним чужим
    // оголошенням, що лягло. Ввімкнений прилад звіряє його у своєму OnWork і
    // перераховує витрату, коли число інше (OZ_PDA_Base.OZ_CfgFollow), --
    // інакше гаряче застосування з VPP доходило до приладу лише з його
    // наступним вмиканням.
    private static int s_Gen = 0;

    static int Gen() { return s_Gen; }

    static OZ_PdaHardwareConfig Get()      { return s_Cfg; }

    static int ModuleCount()
    {
        if (!s_Cfg)
            return 0;
        return s_Cfg.Modules.Count();
    }

    static int CarrierCount()
    {
        if (!s_Cfg)
            return 0;
        return s_Cfg.Carriers.Count();
    }

    // rewrite = false -- the read of "reload every module" (OZ_PdaFacts.Reload): neither
    // Validate's fixes nor the foreign declarations are written; both stay in memory until
    // a load that writes (the next start, the next apply from the window).
    static void ServerLoad(bool rewrite = true)
    {
        s_Cfg = new OZ_PdaHardwareConfig();
        s_Writable = OZ_ConfigLoader<OZ_PdaHardwareConfig>.Load(OZ_PdaConst.HARDWARE, "Hardware", s_Cfg, true, rewrite);

        // Чужі оголошення накладаємо ПІСЛЯ завантаження конфіга -- адмін
        // лишається головнішим -- і робимо це при КОЖНОМУ завантаженні, а не
        // лише при першому: гаряче застосування з вкладки VPP теж проходить
        // сюди.
        bool changed = false;
        for (int i = 0; s_Declared && i < s_Declared.Count(); i++)
        {
            if (Insert(s_Declared[i]))
                changed = true;
        }

        // Чужі записи -- У ФАЙЛ (D94, друга половина). Досі вони жили лише в
        // пам'яті, і редактор RAW JSON у VPP їх не показував: адмін не міг ні
        // побачити плату рації, ні перекрити її, не знаючи класнейма. Тепер
        // після кожного завантаження файл наздоганяє пам'ять; що кому
        // належить, каже Origin. Пишемо лише коли щось справді додалось.
        if (changed && s_Writable && rewrite)
        {
            OZ_ConfigLoader<OZ_PdaHardwareConfig>.Save(OZ_PdaConst.HARDWARE, "Hardware", s_Cfg);
            OZ_Log.Info("hardware: foreign module declarations written to Hardware.json");
        }
        else if (changed && !rewrite)
        {
            OZ_Log.Info("hardware: foreign module declarations are in memory; this read writes no file");
        }
        else if (changed)
        {
            OZ_Log.Warn("hardware: Hardware.json is not writable, foreign module declarations stay in memory only");
        }

        s_Gen++;
    }

    // Чуже залізо. Мод, що приносить свій модуль, оголошує його ОДНИМ рядком
    // зі свого OnMissionStart -- після ServerLoad КПК:
    //
    //     OZ_PdaHardware.Declare(spec);
    //
    // АДМІН ГОЛОВНІШИЙ. Якщо в Hardware.json уже є запис із таким класнеймом,
    // ми його не чіпаємо: власник сервера мусить мати змогу перенастроїти
    // чужий модуль, не правлячи чужий мод. Тому це «оголосити, якщо ще нема»,
    // а не «записати».
    static bool Declare(OZ_ModuleSpec spec)
    {
        if (!spec || spec.ClassName == "")
            return false;

        if (!spec.EnablesPages)
            spec.EnablesPages = new array<string>();
        if (spec.Origin == "")
            spec.Origin = "mod";

        // ЗАПАМ'ЯТОВУЄМО ЗАВЖДИ -- і коли конфіг уже є, і коли ще ні.
        //
        // Список потрібен не лише для черги: він переживає перечитування
        // конфіга (гаряче застосування з VPP) і накладається знову.
        if (!s_Declared)
            s_Declared = new array<ref OZ_ModuleSpec>();
        if (!Known(spec.ClassName))
            s_Declared.Insert(spec);

        // Конфіга ще немає -- лишаємось у черзі. Відповідаємо true:
        // оголошення ПРИЙНЯТО, і мод, який його зробив, має право так вважати.
        if (!s_Cfg)
            return true;

        // Конфіг уже є -- пишемо у файл одразу. Порядок OnMissionStart між
        // модами не гарантований (див. чергу вище): коли КПК завантажився
        // ПЕРШИМ, оголошення не проходить через ServerLoad, і без цього рядка
        // файл наздогнав би пам'ять лише наступним перечитуванням.
        bool added = Insert(spec);
        if (added)
            s_Gen++;
        if (added && s_Writable)
        {
            OZ_ConfigLoader<OZ_PdaHardwareConfig>.Save(OZ_PdaConst.HARDWARE, "Hardware", s_Cfg);
            OZ_Log.Info("hardware: a module declared after the load is written to Hardware.json: " + spec.ClassName);
        }
        return added;
    }

    private static bool Known(string cls)
    {
        for (int i = 0; s_Declared && i < s_Declared.Count(); i++)
        {
            if (s_Declared[i] && s_Declared[i].ClassName == cls)
                return true;
        }
        return false;
    }

    private static bool Insert(OZ_ModuleSpec spec)
    {
        if (!s_Cfg || !spec)
            return false;

        OZ_ModuleSpec had = ModuleFor(spec.ClassName);
        if (had)
        {
            // Запис адміна (Origin порожній) -- головніший, не чіпаємо.
            if (had.Origin == "")
                return false;

            // Запис мода -- мод і оновлює. Той самий вміст переписувати нема
            // чого; це й тримає файл у спокої між запусками.
            if (Same(had, spec))
                return false;

            int at = s_Cfg.Modules.Find(had);
            if (at >= 0)
                s_Cfg.Modules.Set(at, spec);
            OZ_Log.Dbg("module re-declared by another mod: " + spec.ClassName);
            return true;
        }

        s_Cfg.Modules.Insert(spec);
        OZ_Log.Dbg("module declared by another mod: " + spec.ClassName);
        return true;
    }

    private static bool Same(OZ_ModuleSpec a, OZ_ModuleSpec b)
    {
        if (a.DisplayName != b.DisplayName || a.Kind != b.Kind || a.Origin != b.Origin)
            return false;
        if (a.SpyMinutes != b.SpyMinutes || a.RangeM != b.RangeM || a.PowerFactor != b.PowerFactor)
            return false;
        if (!a.EnablesPages || !b.EnablesPages || a.EnablesPages.Count() != b.EnablesPages.Count())
            return false;
        for (int i = 0; i < a.EnablesPages.Count(); i++)
        {
            if (a.EnablesPages[i] != b.EnablesPages[i])
                return false;
        }
        return true;
    }

    static OZ_ModuleSpec ModuleFor(string cls)
    {
        if (!s_Cfg)
            return null;

        // null-guard: Validate вичищає порожні записи, але Declare теж пише
        // в цей масив, і одне місце, де правило порушать, не має валити
        // кожен запит статусу.
        for (int i = 0; i < s_Cfg.Modules.Count(); i++)
        {
            if (s_Cfg.Modules[i] && s_Cfg.Modules[i].ClassName == cls)
                return s_Cfg.Modules[i];
        }
        return null;
    }

    static OZ_CarrierSpec CarrierFor(string cls)
    {
        if (!s_Cfg)
            return null;

        for (int i = 0; i < s_Cfg.Carriers.Count(); i++)
        {
            if (s_Cfg.Carriers[i] && s_Cfg.Carriers[i].ClassName == cls)
                return s_Cfg.Carriers[i];
        }
        return null;
    }
}
