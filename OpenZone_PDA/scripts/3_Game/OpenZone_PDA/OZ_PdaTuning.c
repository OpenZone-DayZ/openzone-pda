// Tuning.json -- ігрові ходові ручки КПК одним файлом.
//
// Все, що адмін хоче підкрутити МІЖ рестартами, живе тут, а не в
// константах: межі текстів, лічильники спроб, радіуси, секунди. Поставочні
// числа -- ЦЕ САМІ ПОЛЯ цього класу й більш ніщо: другий комплект у
// OZ_PdaConst знято 2026-09-06, бо ніщо не звіряло його з першим.
//
// Частина значень потрібна КЛІЄНТОВІ (тривалість тоста, радіус автопроходу
// маршруту) -- їх він отримує не звідси, а в конверті beacon-пуша:
// сервер раз на тік докладає два числа, і окремий канал не потрібен.

// ДОМІВКА МАПИ -- ОДИН РОЗБІРНИК НА ОБИДВІ СТОРОНИ.
//
// Ключ MapHome читають двоє: Validate на сервері (щоб покласти у файл
// справжню точку замість порожнечі) і сторінка карти на клієнті (щоб
// поставити туди мапу приладу без GPS). Розбір і запасний центр мусять бути
// в них ОДНІ; два однакові шматки коду розійшлися б від першої ж правки.
class OZ_PdaHome
{
    // Центр світу. Розмір питаємо в рушія -- тим самим GetWorldSize, яким
    // ваніль міряє межі мапи (mapnavigationbehaviour.c), -- бо мод обіцяє
    // працювати на будь-якій карті, а не на самій Чорнарусі.
    static vector Centre()
    {
        int size = 0;
        World w = GetGame().GetWorld();
        if (w)
            size = w.GetWorldSize();

        // Рушій ще не назвав розміру: беремо Чорнарусь -- 15360 м у
        // квадраті. Це запасний варіант, а не поставочне число карти.
        if (size <= 0)
            size = 15360;

        float half = size * 0.5;
        return Vector(half, 0, half);
    }

    // «x z» (або «x y z») -> точка. Порожній, нерозбірний чи вилізлий за межі
    // світу рядок -- центр карти.
    static vector Point(string s)
    {
        array<string> parts = new array<string>();
        if (s != "")
            s.Split(" ", parts);

        string sx = "";
        string sz = "";
        if (parts.Count() == 2)
        {
            sx = parts[0];
            sz = parts[1];
        }
        else if (parts.Count() == 3)
        {
            sx = parts[0];
            sz = parts[2];
        }

        if (sx == "" || sz == "")
            return Centre();

        float x = sx.ToFloat();
        float z = sz.ToFloat();

        int size = 0;
        World w = GetGame().GetWorld();
        if (w)
            size = w.GetWorldSize();

        // Межі перевіряємо лише тоді, коли рушій сказав розмір: інакше
        // порівнювати нема з чим, а відкидати адмінську точку через мовчання
        // рушія -- гірше, ніж її прийняти.
        if (size > 0)
        {
            if (x < 0 || x > size)
                return Centre();
            if (z < 0 || z > size)
                return Centre();
        }

        return Vector(x, 0, z);
    }

    // Канонічний запис для файла: два числа, «x z». Метра досить -- це
    // місце, куди дивиться мапа, а не координати схованки.
    static string Clean(string s)
    {
        vector p = Point(s);
        int x = Math.Round(p[0]);
        int z = Math.Round(p[2]);
        return x.ToString() + " " + z.ToString();
    }
}

class OZ_PdaTuning : OZ_ConfigBase
{
    // --- PIN ---
    // Скільки невдалих спроб підряд замикає пристрій для цієї особи.
    int PinMaxFails = 5;
    // На скільки секунд. 0 -- до рестарту сервера (стара поведінка).
    int PinLockoutSeconds = 300;

    // --- chat ---
    int ChatMsgMaxBytes   = 1000;
    int ChatTitleMaxBytes = 32;
    int ChatDescMaxBytes  = 96;
    // Скільки останніх рядків віддає міст при відкритті розмови...
    int ChatHistoryOpen = 20;
    // ...і скільки довантажує LOAD OLDER за один крок.
    int ChatHistoryPage = 20;
    // Скільки живе запрошення до групи (ТЗ-4 R-D3.3): своє поле, своє
    // число -- фракційне (Faction.InviteTtlSeconds, 120 с) для групи
    // замале: пропозиція має пережити ніч. Міст прибирає протухлі сам.
    int GroupInviteTtlSeconds = 86400;
    // Стеля складу групи. 0 -- без межі. Перевіряє МІСТ при запрошенні.
    int ChatGroupMax = 16;

    // --- нотатки ---
    // Запасна стеля кількості, коли профіль пристрою не каже своєї.
    int NoteTitleMaxBytes = 64;
    // Не більше OZ_Const.NEWS_BODY_MAX: JsonFileLoader ріже рядки на 1023
    // байтах, і міст відмовляє на тій самій межі. Validate не дасть
    // поставити більше. Ця ж стеля міряє тіло новини на сторінці новин, тож
    // опустити її -- значить зробити КПК суворішим за консоль, а підняти --
    // не можна взагалі.
    int NoteBodyMaxBytes = 1000;

    // --- мітки карти ---
    int MarkerNameMaxBytes = 32;
    int MarkerDescMaxBytes = 160;

    // Куди дивиться мапа приладу БЕЗ GPS, світовими координатами «x z».
    // Прилад не знає, де він, отже відкрити карту на гравцеві означало б
    // сказати йому те, чого прилад не знає; замість цього -- одне й те саме
    // місце, яке обрав адмін. Порожньо -- центр карти; Validate підставляє
    // туди справжні числа, щоб адмін бачив у файлі точку, а не порожнечу.
    string MapHome = "";

    // --- контакти ---
    // З якої відстані інший гравець вважається «поруч» для обміну.
    int FriendReachMeters = 12;
    // Скільки живе пропозиція обміну контактами.
    int SwapOfferTtlSeconds = 60;

    // --- HUD (їде клієнтові в beacon-пуші) ---
    int ToastSeconds = 8;
    int RouteAdvanceMeters = 30;
    // Як часто сервер розсилає маячки власникам працюючих антен.
    int BeaconPushSeconds = 5;

    private static ref OZ_PdaTuning s_Inst;

    // НІКОЛИ НЕ null, і саме тому кожен читач нижче -- один рядок.
    //
    // Було: Get() віддавав null до ServerLoad, і сімнадцять читачів OZ_PdaTune
    // мали по три рядки на цей випадок, а поставочні числа лежали ДРУГИМ
    // комплектом у OZ_PdaConst -- два джерела однієї правди, які ніщо не
    // звіряло. Тепер джерело одне: поля цього класу. Клієнт, який ServerLoad
    // не кличе ніколи, дістає рівно ті самі поставочні значення.
    static OZ_PdaTuning Get()
    {
        if (!s_Inst)
        {
            s_Inst = new OZ_PdaTuning();
            s_Inst.LoadDefaults();
        }
        return s_Inst;
    }

    // v2: блокування піна стало в СЕКУНДАХ (PinLockoutSeconds замість хвилин).
    // Старе поле не переноситься: схема прожила лічені години, і значення за
    // замовчуванням ті самі 5 хвилин -- тобто Migrate тут рівно те, що робить
    // OZ_ConfigBase за замовчуванням, і власного перевизначення не потребує.
    override int LatestVersion()
    {
        return 2;
    }

    override void LoadDefaults()
    {
        Version = LatestVersion();

        PinMaxFails       = 5;
        PinLockoutSeconds = 300;

        ChatMsgMaxBytes   = 1000;
        ChatTitleMaxBytes = 32;
        ChatDescMaxBytes  = 96;
        ChatHistoryOpen   = 20;
        ChatHistoryPage   = 20;
        ChatGroupMax      = 16;
        GroupInviteTtlSeconds = 86400;

        NoteTitleMaxBytes = 64;
        NoteBodyMaxBytes  = 1000;

        MarkerNameMaxBytes = 32;
        MarkerDescMaxBytes = 160;

        // Порожньо, а не число: центр карти залежить від СВІТУ, а
        // LoadDefaults кличуть і там, де рушій ще не сказав його розміру
        // (клієнт бере поставочні значення тим самим шляхом). Справжню
        // точку підставляє Validate, і лише на сервері.
        MapHome = "";

        FriendReachMeters   = 12;
        SwapOfferTtlSeconds = 60;

        ToastSeconds       = 8;
        RouteAdvanceMeters = 30;
        BeaconPushSeconds  = 5;
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        warnings += ClampMin("PinMaxFails", PinMaxFails, 1);
        warnings += ClampMin("PinLockoutSeconds", PinLockoutSeconds, 0);

        // Вище 1000 байтів не можна НІДЕ, де текст їздить через
        // JsonFileLoader чи міст: різ на 1023 псує UTF-8 посеред знака.
        //
        // ЧИСЛОМ ЯДРА, А НЕ СВОЇМ (розбіжність 96). Ця ж стеля міряє тіло
        // новини (OZ_PdaNews, гілка "post"), а те саме тіло з консолі VPP
        // міряє OZ_Const.NEWS_BODY_MAX -- дві однакові тисячі, записані
        // окремо, розійшлися рівно на один байт і на одне порівняння. Тепер
        // адмін не може підняти тутешню вище за ядрову: клямп бере її саму.
        warnings += ClampMax("NoteBodyMaxBytes", NoteBodyMaxBytes, OZ_Const.NEWS_BODY_MAX);
        warnings += ClampMax("ChatMsgMaxBytes", ChatMsgMaxBytes, 1000);

        // СТЕЛІ ТЕКСТУ МАЮТЬ І ПІДЛОГУ, а не саму лише стелю.
        //
        // Нуль чи від'ємне в будь-якій із п'яти означало Substring(0, <=0) на
        // кожному імені й кожному тілі: назви міток порожніли мовчки, а
        // від'ємна довжина -- це вже питання до рушія. Чотири байти -- це
        // одна кирилична пара; менше не є назвою ні для чого.
        warnings += ClampMin("ChatMsgMaxBytes", ChatMsgMaxBytes, 16);
        warnings += ClampMin("ChatTitleMaxBytes", ChatTitleMaxBytes, 4);
        warnings += ClampMin("ChatDescMaxBytes", ChatDescMaxBytes, 4);
        warnings += ClampMin("NoteTitleMaxBytes", NoteTitleMaxBytes, 4);
        warnings += ClampMin("NoteBodyMaxBytes", NoteBodyMaxBytes, 8);
        warnings += ClampMin("MarkerNameMaxBytes", MarkerNameMaxBytes, 4);
        warnings += ClampMin("MarkerDescMaxBytes", MarkerDescMaxBytes, 4);

        // Ці п'ять їдуть у SanitizeString ПЕРЕД OZ_Text.Clip, а vanilla
        // SanitizeString сама ріже на 512 байтах наосліп, посеред UTF-8
        // символу (finding 152). Без стелі тут адмін, що підняв поле вище
        // 512, повертає той самий баг для цього поля -- тому стеля та сама,
        // 512.
        warnings += ClampMax("ChatTitleMaxBytes", ChatTitleMaxBytes, 512);
        warnings += ClampMax("ChatDescMaxBytes", ChatDescMaxBytes, 512);
        warnings += ClampMax("NoteTitleMaxBytes", NoteTitleMaxBytes, 512);
        warnings += ClampMax("MarkerNameMaxBytes", MarkerNameMaxBytes, 512);
        warnings += ClampMax("MarkerDescMaxBytes", MarkerDescMaxBytes, 512);

        warnings += ClampMin("ChatHistoryOpen", ChatHistoryOpen, 1);
        warnings += ClampMin("ChatHistoryPage", ChatHistoryPage, 1);
        warnings += ClampMin("ChatGroupMax", ChatGroupMax, 0);
        warnings += ClampMin("GroupInviteTtlSeconds", GroupInviteTtlSeconds, 60);

        warnings += ClampMin("FriendReachMeters", FriendReachMeters, 1);
        warnings += ClampMin("SwapOfferTtlSeconds", SwapOfferTtlSeconds, 5);

        warnings += ClampMin("ToastSeconds", ToastSeconds, 2);
        warnings += ClampMin("RouteAdvanceMeters", RouteAdvanceMeters, 5);
        warnings += ClampMin("BeaconPushSeconds", BeaconPushSeconds, 2);

        // ДОМІВКА МАПИ: підставляємо центр світу, коли адмін не написав
        // точки або написав щось, з чого точки не виходить. Це ПОЧИНКА --
        // файл після неї перепишеться раз і більше не турбуватиме.
        string home = OZ_PdaHome.Clean(MapHome);
        if (home != MapHome)
        {
            OZ_Log.Warn("Tuning: MapHome \"" + MapHome + "\" is not a world point, set to " + home);
            MapHome = home;
            warnings++;
        }
    }

    // КЛАМП ЖИВЕ ТУТ, а не в викликача. Enforce ПЕРЕДАЄ int за посиланням --
    // ключове слово inout, -- тож коментар «сам кламп робить викликач» був
    // неправдою, яка коштувала чотирнадцяти зайвих рядків Math.Max поруч, і
    // кожен із них міг розійтися зі своїм попередженням непомітно.
    private int ClampMin(string name, inout int val, int floor)
    {
        if (val >= floor)
            return 0;
        OZ_Log.Warn("Tuning: " + name + " under " + floor.ToString() + ", clamped");
        val = floor;
        return 1;
    }

    private int ClampMax(string name, inout int val, int ceiling)
    {
        if (val <= ceiling)
            return 0;
        OZ_Log.Warn("Tuning: " + name + " over " + ceiling.ToString() + " breaks storage strings and the bridge clip, clamped");
        val = ceiling;
        return 1;
    }

    // rewrite = false -- the read of "reload every module" (OZ_PdaFacts.Reload): what
    // Validate fixes stays in memory, and the file stays as the admin left it. A missing
    // or older file is still written, with its backup, as at the start.
    static void ServerLoad(bool rewrite = true)
    {
        s_Inst = new OZ_PdaTuning();
        OZ_ConfigLoader<OZ_PdaTuning>.Load(OZ_Const.PROFILE_DIR + "\\OZ_PDA_Tuning.json", "Tuning", s_Inst, true, rewrite);
    }
}

// Читачі одним іменем. Запасних значень тут БІЛЬШЕ НЕМАЄ: Get() ніколи не
// віддає null, а поставочні числа живуть у полях самого OZ_PdaTuning --
// один комплект замість двох, що розходились би мовчки.
class OZ_PdaTune
{
    static int PinMaxFails()      { return OZ_PdaTuning.Get().PinMaxFails; }
    static int PinLockoutMs()     { return OZ_PdaTuning.Get().PinLockoutSeconds * 1000; }

    static int ChatMsgMax()       { return OZ_PdaTuning.Get().ChatMsgMaxBytes; }
    static int ChatTitleMax()     { return OZ_PdaTuning.Get().ChatTitleMaxBytes; }
    static int ChatDescMax()      { return OZ_PdaTuning.Get().ChatDescMaxBytes; }
    static int ChatHistoryOpen()  { return OZ_PdaTuning.Get().ChatHistoryOpen; }
    static int ChatHistoryPage()  { return OZ_PdaTuning.Get().ChatHistoryPage; }
    static int GroupInviteTtlS()  { return OZ_PdaTuning.Get().GroupInviteTtlSeconds; }
    static int ChatGroupMax()     { return OZ_PdaTuning.Get().ChatGroupMax; }

    static int NoteTitleMax()     { return OZ_PdaTuning.Get().NoteTitleMaxBytes; }
    static int NoteBodyMax()      { return OZ_PdaTuning.Get().NoteBodyMaxBytes; }

    static int MarkerNameMax()    { return OZ_PdaTuning.Get().MarkerNameMaxBytes; }
    static int MarkerDescMax()    { return OZ_PdaTuning.Get().MarkerDescMaxBytes; }
    static string MapHome()       { return OZ_PdaTuning.Get().MapHome; }

    static int FriendReachM()     { return OZ_PdaTuning.Get().FriendReachMeters; }
    static int SwapOfferTtlMs()   { return OZ_PdaTuning.Get().SwapOfferTtlSeconds * 1000; }

    static int ToastSeconds()     { return OZ_PdaTuning.Get().ToastSeconds; }
    static int RouteAdvanceM()    { return OZ_PdaTuning.Get().RouteAdvanceMeters; }
    static float BeaconPushSeconds() { return OZ_PdaTuning.Get().BeaconPushSeconds; }
}
