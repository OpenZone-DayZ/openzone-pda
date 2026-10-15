// Серверна половина КПК: реєструє свої сторінки, читає профілі пристроїв і
// підміняє ядерну заглушку доступу справжньою перевіркою.

class OZ_PdaHandlerDevice : OZ_PageHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        if (op == "status")
            return Status(sender, ok, error);

        if (op == "unlock")
            return Unlock(json, sender, ok, error);

        if (op == "logout_others")
            return LogoutOthers(sender, ok, error);

        if (op == "initiate")
            return Initiate(sender, ok, error);

        if (op == "lock")
            return Lock(sender, ok, error);

        if (op == "factory_reset")
            return FactoryReset(sender, ok, error);

        if (op == "autolock")
            return AutoLock(json, sender, ok, error);

        if (op == "power")
            return Power(json, sender, ok, error);

        if (op == "setpin")
            return SetPin(json, sender, ok, error);

        if (op == "crack")
            return Crack(sender, ok, error);

        if (op == "sealed")
            return Sealed(sender, ok, error);

        if (op == "carrier_write")
            return CarrierWrite(json, sender, ok, error);

        if (op == "carrier_read")
            return CarrierRead(sender, ok, error);

        if (op == "carrier_import" || op == "carrier_take")
        {
            // КАПСУЛА не приймає нічого нового -- ні в пам'ять пристрою,
            // ні в акаунт власника: імпорт із чипа зачинено в обидва боки.
            // Сам чип лишається живим носієм: запис, читання і чистка чипа
            // працюють -- це фізика гнізда, не пам'ять пристрою.
            if (OZ_PdaCapsule.IsFrozen(OZ_PdaLookup.HeldBy(sender)))
            {
                error = "STR_OZ_ERR_FROZEN";
                return "";
            }
        }

        if (op == "carrier_import")
            return CarrierImport(sender, ok, error);

        if (op == "carrier_take")
            return CarrierTake(json, sender, ok, error);

        if (op == "carrier_del")
            return CarrierDel(json, sender, ok, error);

        if (op == "carrier_erase")
            return CarrierErase(sender, ok, error);

        return "";
    }

    // ------------------------------------------------------------ носій
    //
    // Чип -- фізична річ для фізичного обміну: записав мітки, віддав у руки,
    // той вставив і забрав собі. Пейлоад живе НА ПРЕДМЕТІ (CF ModStorage),
    // переживає рестарти і їде з чипом у кишені, у сховку, на трупі.
    //
    // Ворота доступу безкоштовні: ці опи не входять у винятки живлення й
    // замка, тож OZ_PdaAccess вже вимагає ввімкнений і відімкнений пристрій.

    // Прилад гравця, з тією ж перевіркою, що й у OZ_CarrierOps.
    //
    // Шість місць у операціях носія брали прилад ПОВТОРНО -- рядком
    // `OZ_PdaLookup.HeldBy(sender)` без жодної перевірки -- і одразу його
    // розіменовували. Трималось воно на негласному припущенні «CarrierOf уже
    // пройшов, значить прилад є». Відколи ворота вимагають РЕЧІ (рішення
    // власника 2026-09-08 прибрало виняток для віртуального термінала),
    // припущення правдиве завжди -- але сказане тут, а не негласне: перевірка
    // на null коштує рядок, а розіменований null обриває обробник посеред
    // роботи.
    private OZ_PDA_Base DeviceOf(PlayerIdentity sender, out string error)
    {
        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
            error = "STR_OZ_ERR_NO_DEVICE";
        return pda;
    }

    private string CarrierWrite(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_CarrierWriteOp opw = new OZ_CarrierWriteOp();
        string err;
        if (!JsonFileLoader<OZ_CarrierWriteOp>.LoadData(json, opw, err) || !opw)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        // Перше справжнє застосування Writable з Hardware.json: чип, який
        // конфіг оголосив лише читаним, не перезаписується ніколи. Клас БЕЗ
        // запису в таблиці -- теж замок: клієнт такому кнопок не малює, і
        // підроблений запит не має права пройти там, де чесний не пройде.
        OZ_CarrierSpec spec = OZ_PdaHardware.CarrierFor(c.GetType());
        if (!spec || !spec.Writable)
        {
            error = "STR_OZ_ERR_CARRIER_LOCKED";
            return "";
        }

        // Гейта роду немає: секції незалежні, запис міток не чіпає записок
        // і навпаки. Стирання лишилось окремою дією для чистки ОБОХ.
        if (opw.Kind == "markers")
        {
            OZ_PDA_Base pda = DeviceOf(sender, error);
            if (!pda)
                return "";

            // РОЗБІР ОБОВ'ЯЗКОВИЙ, і саме цього тут бракувало.
            //
            // Нечитний блоб пам'яті приладу їхав на чип ДОСЛІВНО, з
            // Records = 0, і операція звітувала успіх: гравець бачив
            // «збережено», чип мовчки ніс сміття, а місткість носія рахувала
            // це в нуль записів. Нотаткова гілка поруч робила правильно.
            OZ_MarkerList pl = new OZ_MarkerList();
            string perr;
            if (!JsonFileLoader<OZ_MarkerList>.LoadData(pda.OZ_MarkersJson(), pl, perr) || !pl || !pl.Items)
            {
                // Порожня пам'ять -- законний стан, а не поломка: на чип іде
                // порожній список.
                if (pda.OZ_MarkersJson() != "")
                {
                    OZ_Log.Warn("carrier: unreadable markers on " + pda.GetType() + ", refusing to copy (" + perr + ")");
                    error = "STR_OZ_ERR_PDA_INTERNAL";
                    return "";
                }
                pl = new OZ_MarkerList();
            }

            int cnt = pl.Items.Count();

            // Місце питаємо в носія: на дискету йде стільки, скільки влазить
            // ПОРУЧ ІЗ ТИМ, ЩО НА НІЙ УЖЕ Є, -- перші зі списку, і відповідь
            // чесно каже скільки.
            int room = c.OZ_RoomFor(OZ_DataCarrier_Base.KIND_MARKS);
            int wrote = cnt;
            if (room >= 0 && cnt > room)
            {
                pl.Items.Resize(room);
                wrote = room;
            }

            // Пишемо ЗАВЖДИ розібраний і наново зібраний список, а не сирий
            // рядок приладу: те, що ліг на чип, гарантовано читається.
            string payload;
            if (!JsonFileLoader<OZ_MarkerList>.MakeData(pl, payload, perr, false))
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            // Успіх -- лише після запису: OZ_Write ВІДМОВЛЯЄ, коли місця немає
            // (він не обрізає), а безумовне ok = true казало «збережено» над
            // чипом, на який нічого не лягло.
            if (!c.OZ_WriteMarks(payload, wrote))
            {
                error = "STR_OZ_ERR_CARRIER_FULL";
                return "";
            }

            OZ_CarrierTaken wt = new OZ_CarrierTaken();
            wt.Taken = wrote;
            wt.Total = cnt;

            string wtj;
            if (!JsonFileLoader<OZ_CarrierTaken>.MakeData(wt, wtj, perr, false))
                wtj = "";

            ok = true;
            error = "";
            return wtj;
        }

        if (opw.Kind == "notes")
        {
            // Записки живуть У ПРИСТРОЇ (рішення власника 2026-08-28):
            // книжка вже в руках, міст не потрібен, відповідь синхронна.
            // Місткість класу чипа: на малий носій лягають ПЕРШІ записки,
            // і відповідь чесно каже скільки з скількох.
            OZ_PDA_Base pdaW = DeviceOf(sender, error);
            if (!pdaW)
                return "";

            OZ_NoteBook bookW = new OZ_NoteBook();
            if (pdaW.OZ_NotesJson() != "")
            {
                OZ_NoteBook parsedW = new OZ_NoteBook();
                if (JsonFileLoader<OZ_NoteBook>.LoadData(pdaW.OZ_NotesJson(), parsedW, err) && parsedW && parsedW.Notes)
                    bookW = parsedW;
            }

            int totalW = bookW.Notes.Count();
            int wroteW = totalW;
            int roomW  = c.OZ_RoomFor(OZ_DataCarrier_Base.KIND_NOTES);
            if (roomW >= 0 && totalW > roomW)
            {
                bookW.Notes.Resize(roomW);
                wroteW = roomW;
            }

            string payloadW;
            if (!JsonFileLoader<OZ_NoteBook>.MakeData(bookW, payloadW, err, false))
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            if (!c.OZ_WriteNotes(payloadW, wroteW))
            {
                error = "STR_OZ_ERR_CARRIER_FULL";
                return "";
            }

            OZ_CarrierTaken wtN = new OZ_CarrierTaken();
            wtN.Taken = wroteW;
            wtN.Total = totalW;

            string wtjN;
            if (!JsonFileLoader<OZ_CarrierTaken>.MakeData(wtN, wtjN, err, false))
                wtjN = "";

            ok = true;
            error = "";
            return wtjN;
        }

        error = "STR_OZ_ERR_PDA_INTERNAL";
        return "";
    }

    private string CarrierRead(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        if (!c.OZ_IsWritten())
        {
            error = "STR_OZ_ERR_CARRIER_BLANK";
            return "";
        }

        // Секції РОЗБИРАЄМО ТУТ і віддаємо об'єктами: рядок-значення в JSON
        // клієнт зрізав би на 1023 байтах (зміряно). Що на чипі -- те й
        // видно, і саме тому крадений КПК з чужим чипом читає чужі мітки:
        // така ціна фізичного носія, і вона навмисна.
        OZ_CarrierView v = new OZ_CarrierView();

        string serr;
        if (c.OZ_Marks() != "")
        {
            // Копія: між цими двома розборами стоїть другий розбір, а
            // серіалізується конверт іще пізніше.
            OZ_MarkerList vm = new OZ_MarkerList();
            if (JsonFileLoader<OZ_MarkerList>.LoadData(c.OZ_Marks(), vm, serr) && vm && vm.Items)
                v.Marks = vm.Copy();
        }
        if (c.OZ_Notes() != "")
        {
            OZ_NoteBook vn = new OZ_NoteBook();
            if (JsonFileLoader<OZ_NoteBook>.LoadData(c.OZ_Notes(), vn, serr) && vn && vn.Notes)
                v.Notes = vn.Copy();
        }
        if (c.OZ_Route() != "")
        {
            OZ_MarkerList vr = new OZ_MarkerList();
            if (JsonFileLoader<OZ_MarkerList>.LoadData(c.OZ_Route(), vr, serr) && vr && vr.Items)
                v.Route = vr.Copy();
        }

        OZ_CarrierSpec vspec = OZ_PdaHardware.CarrierFor(c.GetType());
        if (vspec)
        {
            v.MaxRecords  = vspec.MaxRecords;
            v.UsedRecords = c.OZ_Used();
        }

        string outJson;
        string err;
        if (!JsonFileLoader<OZ_CarrierView>.MakeData(v, outJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return outJson;
    }

    // ГУРТОВИЙ ІМПОРТ -- УСЕ АБО НІЧОГО (рішення власника 2026-09-29).
    //
    // Раніше кожна секція чипа лягала сама по собі й поки влазила: мітки до
    // стелі, маршрут окремо, записки до стелі, а бита книжка записок на чипі
    // лишала вже покладені мітки. Гравець бачив «імпортовано 17/30» і сам
    // шукав, чого не дісталось. Тепер спершу розбираємо ВСЕ -- секції чипа,
    // книжки приладу, нитку маршруту -- і рахуємо, скільки нових ячеек це
    // коштує. Не влазить -- відмова, і в приладі не міняється нічого. Влазить
    // -- усе серіалізуємо ДО першого запису й кладемо разом.
    //
    // Дублі (та сама мітка чи записка вже в приладі) ячеек не коштують і
    // вдруге не лягають, тож «взято» буває меншим за «на чипі» й при успіху:
    // клієнт тоді каже, що решта вже була в приладі.
    private string CarrierImport(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        if (!c.OZ_IsWritten())
        {
            error = "STR_OZ_ERR_CARRIER_BLANK";
            return "";
        }

        bool hasMarks = c.OZ_Marks() != "";
        bool hasNotes = c.OZ_Notes() != "";
        bool hasRoute = c.OZ_Route() != "";

        // Чип записаний, але не тим, що бере цей імпорт (скажімо, лише
        // книжкою частот рації -- її забирає своя сторінка).
        if (!hasMarks && !hasNotes && !hasRoute)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        string err;

        // ---- 1. Секції чипа: розбір, жодного запису.
        //
        // Копії ДО наступних розборів: корінь тут скриптовий, а Items і кожен
        // їхній елемент виділив серіалізатор -- і читаються вони нижче, вже
        // після розбору власних книжок приладу.
        OZ_MarkerList inMarks;
        if (hasMarks)
        {
            OZ_MarkerList pm = new OZ_MarkerList();
            if (!JsonFileLoader<OZ_MarkerList>.LoadData(c.OZ_Marks(), pm, err) || !pm || !pm.Items)
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }
            inMarks = pm.Copy();
        }

        OZ_NoteBook inNotes;
        if (hasNotes)
        {
            OZ_NoteBook pn = new OZ_NoteBook();
            if (!JsonFileLoader<OZ_NoteBook>.LoadData(c.OZ_Notes(), pn, err) || !pn || !pn.Notes)
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }
            inNotes = pn.Copy();
        }

        // Нитка -- ОДИН запис, що ЗАМІНЮЄ маршрут приладу, як і в route_take:
        // збирає її та сама дорога (OZ_PdaHandlerMap.BuildRoute), а пишемо
        // нижче, разом з усім.
        OZ_MarkerList inRoute;
        if (hasRoute)
        {
            inRoute = OZ_PdaHandlerMap.BuildRoute(pda, c, error);
            if (!inRoute)
                return "";
        }

        // ---- 2. Книжки приладу. Нечитну НЕ ПЕРЕЗАПИСУЄМО (див. OZ_PdaMarks,
        // OZ_PdaNoteBook): імпорт клав би чип поверх усього, що там лежало.
        OZ_MarkerList mine;
        if (hasMarks)
        {
            bool marksBad;
            mine = OZ_PdaMarks.Load(pda, marksBad);
            if (marksBad)
            {
                error = "STR_OZ_ERR_MARKS_CORRUPT";
                return "";
            }
        }

        OZ_NoteBook mineN;
        if (hasNotes)
        {
            bool notesBad;
            mineN = OZ_PdaNoteBook.Load(pda, notesBad);
            if (notesBad)
            {
                error = "STR_OZ_ERR_NOTES_CORRUPT";
                return "";
            }
        }

        // ---- 3. Що справді нове. Зливаємо в ЛОКАЛЬНІ копії книжок: до
        // рішення про місце прилад не бачить нічого.
        int seenMarks = 0;
        int addMarks  = 0;
        if (hasMarks)
        {
            for (int i = 0; i < inMarks.Items.Count(); i++)
            {
                OZ_MapMarker m = inMarks.Items[i];
                // Чужий чип -- чужий JSON: масив може нести null-елементи.
                if (!m)
                    continue;
                seenMarks++;

                // Той самий санітар, що й у marker_add: чуже походження --
                // не привілей, а межі в чипа ніхто не питав.
                m.Name = OZ_Text.Clip(m.Name, OZ_PdaTune.MarkerNameMax());
                m.Desc = OZ_Text.Clip(m.Desc, OZ_PdaTune.MarkerDescMax());

                // Дедуп за ВМІСТОМ: та сама назва в тій самій точці вже на
                // пристрої (чи вже відібрана з цього ж чипа) -- не дублюємо.
                // Без цього резервна копія (записав усі мітки на чип, потім
                // імпортував) плодила б другий комплект, а цикл
                // експорт->імпорт множив мітку щоразу.
                bool dup = false;
                for (int d = 0; d < mine.Items.Count(); d++)
                {
                    if (mine.Items[d].Name == m.Name && mine.Items[d].Pos == m.Pos)
                    {
                        dup = true;
                        break;
                    }
                }
                if (dup)
                    continue;

                // Id карбуємо ЗАНОВО: чужі id зіткнулись би з нашими, і
                // видалення по id зносило б не ту мітку.
                s_CarrierSeq++;
                m.Id = OZ_Time.NowUtc() + "#c" + s_CarrierSeq.ToString();
                mine.Items.Insert(m);
                addMarks++;
            }
        }

        int seenNotes = 0;
        int addNotes  = 0;
        if (hasNotes)
        {
            for (int ni = 0; ni < inNotes.Notes.Count(); ni++)
            {
                OZ_Note nn = inNotes.Notes[ni];
                if (!nn)
                    continue;
                seenNotes++;

                string tN = OZ_Text.Clip(nn.Title, OZ_PdaTune.NoteTitleMax());
                string bN = OZ_Text.Clip(nn.Body, OZ_PdaTune.NoteBodyMax());

                // Дедуп за ВМІСТОМ: цикл експорт->імпорт не плодить копій.
                bool dupN = false;
                for (int nd = 0; nd < mineN.Notes.Count(); nd++)
                {
                    if (mineN.Notes[nd].Title == tN && mineN.Notes[nd].Body == bN)
                    {
                        dupN = true;
                        break;
                    }
                }
                if (dupN)
                    continue;

                OZ_Note fresh = new OZ_Note();
                s_CarrierSeq++;
                fresh.Id        = OZ_Time.NowUtc() + "#cn" + s_CarrierSeq.ToString();
                fresh.Title     = tN;
                fresh.Body      = bN;
                fresh.CreatedAt = OZ_Time.NowUtc();
                fresh.EditedAt  = fresh.CreatedAt;
                mineN.Notes.Insert(fresh);
                addNotes++;
            }
        }

        // ---- 4. Чи влазить УСЕ. Пам'ять спільна для міток, записок,
        // маршруту й розділів чужих модулів -- OZ_Free рахує їх усі. Нитка
        // коштує ячейку, лише коли маршруту в приладі ще немає: імпорт його
        // замінює, а не додає другий.
        int routeCells = 0;
        if (hasRoute && pda.OZ_RouteJson() == "")
            routeCells = 1;

        int need = addMarks + addNotes + routeCells;
        int room = pda.OZ_Free();
        if (need > room)
        {
            string why = "carrier: import refused for " + sender.GetPlainId() + " - needs ";
            why += need.ToString() + " cell(s), " + room.ToString() + " free; nothing written";
            OZ_Log.Info(why);
            error = "STR_OZ_ERR_IMPORT_NO_ROOM";
            return "";
        }

        // ---- 5. Серіалізація ВСЬОГО до першого запису: збій на третій
        // секції не має лишити в приладі дві перші.
        string marksJson;
        if (addMarks > 0 && !JsonFileLoader<OZ_MarkerList>.MakeData(mine, marksJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string notesJson;
        if (addNotes > 0 && !JsonFileLoader<OZ_NoteBook>.MakeData(mineN, notesJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string routeJson;
        if (hasRoute && !JsonFileLoader<OZ_MarkerList>.MakeData(inRoute, routeJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        // ---- 6. Запис -- разом, і лише те, що змінилось.
        if (addMarks > 0)
            pda.OZ_SetMarkersJson(marksJson);
        if (addNotes > 0)
            pda.OZ_SetNotesJson(notesJson);
        if (hasRoute)
            pda.OZ_SetRouteJson(routeJson);

        // ---- 7. Підсумок: одна op, одна цифра на всі секції. Маршрут у
        // підсумку -- один запис, як і в пам'яті.
        int taken = addMarks + addNotes;
        int total = seenMarks + seenNotes;
        if (hasRoute)
        {
            taken += 1;
            total += 1;
        }

        string said = "carrier: imported " + addMarks.ToString() + " marker(s), " + addNotes.ToString() + " note(s)";
        if (hasRoute)
            said += " and a route of " + inRoute.Items.Count().ToString() + " point(s)";
        said += ", " + taken.ToString() + "/" + total.ToString() + " for " + sender.GetPlainId();
        OZ_Log.Info(said);

        OZ_CarrierTaken t = new OZ_CarrierTaken();
        t.Taken = taken;
        t.Total = total;

        string tj;
        if (!JsonFileLoader<OZ_CarrierTaken>.MakeData(t, tj, err, false))
            tj = "";

        ok = true;
        error = "";
        return tj;
    }

    // Забрати ОДИН запис із чипа: гравець дивиться превʼю і бере лише те,
    // що йому треба. І мітка, і записка лягають одразу в пам'ять пристрою.
    private string CarrierTake(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_CarrierItemRef r = new OZ_CarrierItemRef();
        string err;
        if (!JsonFileLoader<OZ_CarrierItemRef>.LoadData(json, r, err) || !r || r.Index < 0)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        if (r.Kind == "mark")
        {
            OZ_MarkerList src = new OZ_MarkerList();
            if (!JsonFileLoader<OZ_MarkerList>.LoadData(c.OZ_Marks(), src, err) || !src || !src.Items || r.Index >= src.Items.Count())
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            // Потрібну мітку знімаємо ДО другого розбору й одразу копіюємо:
            // Items і кожен його елемент виділив серіалізатор, а Name, Pos
            // і Desc читаються нижче, вже після розбору власного списку
            // приладу. Копіюємо саме ЕЛЕМЕНТ, а не весь список: src.Copy()
            // пропускає null-елементи, і після нього r.Index указував би на
            // сусідню мітку.
            OZ_MapMarker m = src.Items[r.Index];
            if (!m)
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }
            m = m.Copy();

            OZ_PDA_Base pda = DeviceOf(sender, error);
            if (!pda)
                return "";

            // Нечитну пам'ять не перезаписуємо (див. OZ_PdaMarks).
            bool marksBad;
            OZ_MarkerList mine = OZ_PdaMarks.Load(pda, marksBad);
            if (marksBad)
            {
                error = "STR_OZ_ERR_MARKS_CORRUPT";
                return "";
            }

            // Стеля -- ПАМ'ЯТЬ ПРИЛАДУ, спільна з нотатками, маршрутом і
            // розділами чужих модулів: вільні ячейки плюс ті, що вже зайняли
            // власні мітки (їх імпорт не додає, а доповнює).
            int limit = pda.OZ_Free() + mine.Items.Count();

            if (limit <= 0 || mine.Items.Count() >= limit)
            {
                error = "STR_OZ_ERR_MARKERS_FULL";
                return "";
            }

            m.Name = OZ_Text.Clip(m.Name, OZ_PdaTune.MarkerNameMax());
            m.Desc = OZ_Text.Clip(m.Desc, OZ_PdaTune.MarkerDescMax());

            // Дедуп за ВМІСТОМ -- та сама причина, що в гуртового імпорту:
            // цикл експорт->імпорт не має плодити копії.
            for (int d = 0; d < mine.Items.Count(); d++)
            {
                if (mine.Items[d].Name == m.Name && mine.Items[d].Pos == m.Pos)
                {
                    error = "STR_OZ_ERR_CARRIER_DUP";
                    return "";
                }
            }

            s_CarrierSeq++;
            m.Id = OZ_Time.NowUtc() + "#c" + s_CarrierSeq.ToString();
            mine.Items.Insert(m);

            string outJson;
            if (!JsonFileLoader<OZ_MarkerList>.MakeData(mine, outJson, err, false))
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            pda.OZ_SetMarkersJson(outJson);
            ok = true;
            error = "";
            return "";
        }

        if (r.Kind == "note")
        {
            OZ_NoteBook book = new OZ_NoteBook();
            if (!JsonFileLoader<OZ_NoteBook>.LoadData(c.OZ_Notes(), book, err) || !book || !book.Notes || r.Index >= book.Notes.Count())
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            // Потрібну записку знімаємо ДО другого розбору й одразу
            // копіюємо: Notes і кожен його елемент виділив серіалізатор, а
            // Title і Body читаються нижче, вже після розбору власної
            // книжки приладу. Копіюємо саме ЕЛЕМЕНТ, а не всю книжку:
            // book.Copy() пропускає null-елементи, і після нього r.Index
            // указував би на сусідню записку.
            OZ_Note taken = book.Notes[r.Index];
            if (!taken)
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }
            taken = taken.Copy();

            // Записки -- пам'ять ПРИСТРОЮ: забрати з чипа означає
            // дописати в книжку приладу, без моста. Межі й дедап ті
            // самі, що в міток: цикл експорт->імпорт не плодить копій.
            OZ_PDA_Base pdaN = DeviceOf(sender, error);
            if (!pdaN)
                return "";

            // Нечитну книжку не перезаписуємо (див. OZ_PdaNoteBook).
            bool notesBad;
            OZ_NoteBook mineN = OZ_PdaNoteBook.Load(pdaN, notesBad);
            if (notesBad)
            {
                error = "STR_OZ_ERR_NOTES_CORRUPT";
                return "";
            }

            // Та сама спільна пам'ять -- див. вище про мітки.
            //
            // ПІДСТАВНОГО ПОТОЛКА ТУТ БІЛЬШЕ НЕМАЄ. Стояло
            // `if (limitN <= 0) limitN = OZ_PdaTune.NotesMax();` -- тобто
            // прилад, у якого ВЖЕ НЕМАЄ вільних ячейок, отримував дозвіл
            // дописати ще стільки записок, скільки каже налаштування. Нуль
            // тут означає «повний», а не «невідомо».
            int limitN = pdaN.OZ_Free() + mineN.Notes.Count();

            if (mineN.Notes.Count() >= limitN)
            {
                error = "STR_OZ_ERR_NOTES_FULL";
                return "";
            }

            string tN = OZ_Text.Clip(taken.Title, OZ_PdaTune.NoteTitleMax());
            string bN = OZ_Text.Clip(taken.Body, OZ_PdaTune.NoteBodyMax());

            for (int dn = 0; dn < mineN.Notes.Count(); dn++)
            {
                if (mineN.Notes[dn].Title == tN && mineN.Notes[dn].Body == bN)
                {
                    error = "STR_OZ_ERR_CARRIER_DUP";
                    return "";
                }
            }

            OZ_Note freshN = new OZ_Note();
            s_CarrierSeq++;
            freshN.Id        = OZ_Time.NowUtc() + "#cn" + s_CarrierSeq.ToString();
            freshN.Title     = tN;
            freshN.Body      = bN;
            freshN.CreatedAt = OZ_Time.NowUtc();
            freshN.EditedAt  = freshN.CreatedAt;
            mineN.Notes.Insert(freshN);

            string outN;
            if (!JsonFileLoader<OZ_NoteBook>.MakeData(mineN, outN, err, false))
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            pdaN.OZ_SetNotesJson(outN);
            ok = true;
            error = "";
            return "";
        }

        error = "STR_OZ_ERR_PDA_INTERNAL";
        return "";
    }

    // Стерти ОДИН запис із чипа. Писабельність та сама, що в будь-якого
    // запису: замкнений клас не редагується поштучно так само, як і цілком.
    private string CarrierDel(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_CarrierItemRef r = new OZ_CarrierItemRef();
        string err;
        if (!JsonFileLoader<OZ_CarrierItemRef>.LoadData(json, r, err) || !r || r.Index < 0)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_DataCarrier_Base c = OZ_CarrierOps.ResolveWritable(sender, error);
        if (!c)
            return "";

        if (r.Kind == "mark")
        {
            OZ_MarkerList ml = new OZ_MarkerList();
            if (!JsonFileLoader<OZ_MarkerList>.LoadData(c.OZ_Marks(), ml, err) || !ml || !ml.Items || r.Index >= ml.Items.Count())
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            ml.Items.RemoveOrdered(r.Index);

            string mj = "";
            if (ml.Items.Count() > 0)
            {
                if (!JsonFileLoader<OZ_MarkerList>.MakeData(ml, mj, err, false))
                {
                    error = "STR_OZ_ERR_PDA_INTERNAL";
                    return "";
                }
            }

            if (!c.OZ_WriteMarks(mj, ml.Items.Count()))
            {
                error = "STR_OZ_ERR_CARRIER_FULL";
                return "";
            }
            ok = true;
            error = "";
            return "";
        }

        if (r.Kind == "note")
        {
            OZ_NoteBook nb = new OZ_NoteBook();
            if (!JsonFileLoader<OZ_NoteBook>.LoadData(c.OZ_Notes(), nb, err) || !nb || !nb.Notes || r.Index >= nb.Notes.Count())
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            nb.Notes.RemoveOrdered(r.Index);

            string nj = "";
            if (nb.Notes.Count() > 0)
            {
                if (!JsonFileLoader<OZ_NoteBook>.MakeData(nb, nj, err, false))
                {
                    error = "STR_OZ_ERR_PDA_INTERNAL";
                    return "";
                }
            }

            if (!c.OZ_WriteNotes(nj, nb.Notes.Count()))
            {
                error = "STR_OZ_ERR_CARRIER_FULL";
                return "";
            }
            ok = true;
            error = "";
            return "";
        }

        error = "STR_OZ_ERR_PDA_INTERNAL";
        return "";
    }

    private string CarrierErase(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        OZ_CarrierSpec spec = OZ_PdaHardware.CarrierFor(c.GetType());
        if (!spec || !spec.Writable)
        {
            error = "STR_OZ_ERR_CARRIER_LOCKED";
            return "";
        }

        // Симетрія з імпортом і читанням: порожній чип стирати нема чого,
        // і «зроблено» на ніщо було б звітом про неіснуючу роботу.
        if (!c.OZ_IsWritten())
        {
            error = "STR_OZ_ERR_CARRIER_BLANK";
            return "";
        }

        c.OZ_Erase();
        ok = true;
        error = "";
        return "";
    }

    // Лічильник записів, які кладе в прилад НОСІЙ. Id -- «мить + префікс +
    // число», і в кожного карбувальника свій префікс: мітки носія "#c",
    // нотатки носія "#cn", свої мітки карти "#" (OZ_PdaMap), маршрут "#r",
    // свої нотатки "#n" (OZ_PdaNotes). Нотатки носія карбувались "#n" ЦИМ
    // лічильником, тобто в ту саму секунду давали той самий id, що й нотатка,
    // щойно записана вручну, -- а правка й видалення шукають саме за id.
    private static int s_CarrierSeq = 0;

    // ТУТ ЖИЛИ VirtualOpen І VirtualStatus -- «КПК без предмета» (D132):
    // операція, що відчиняла екран без речі, і вигаданий стан, який його
    // наповнював (повний заряд, нульові відсіки, сторінки з
    // VirtualDevice.Pages). Рішення власника 2026-09-08: приладу немає --
    // екрана немає, і вигадувати нема чого.

    // СКІЛЬКИ ХВИЛИН РОБОТИ ЛИШИЛОСЬ У ПЛАТІ (ТЗ-5 R-B2.9), або -1, якщо
    // ресурсу в неї немає взагалі.
    //
    // Ресурс живе В ПЛАТІ, не в конфізі: конфіг -- бюджет, предмет --
    // лічильник (R-B2.5). Свіжа плата віддає -1 із власного лічильника, і
    // це «повний бак», а не «невідомо»: скільки в ній хвилин, знає спека.
    //
    // Поки ресурс має рівно один рід плат -- шпигунська антена. Коли
    // з'явиться другий (R-B2.3 обіцяє ResourceMinutes/ResourceMode усім),
    // питати треба буде поведінку модуля, а не клас; єдиний рядок, який це
    // зачепить, -- цей.
    private int ResourceLeftMin(ItemBase item, OZ_ModuleSpec spec)
    {
        if (!spec)
            return -1;
        if (spec.SpyMinutes <= 0)
            return -1;

        OZ_Module_SpyAntenna plate = OZ_Module_SpyAntenna.Cast(item);
        if (!plate)
            return -1;

        float leftS = plate.OZ_SpyLeftS();
        if (leftS < 0)
            leftS = spec.SpyMinutes * 60;
        if (leftS < 0)
            leftS = 0;

        int mins = Math.Round(leftS / 60.0);
        return mins;
    }

    // ЧИ МОЖНА ПОКАЗАТИ АКАУНТНУ ВКЛАДКУ на цьому приладі. Одне правило на
    // сторінки профілю й на ті, що приносить залізо (EnablesPages): досі
    // друге обходило його зовсім.
    //
    // Вимкненому -- лише пристрій. Нічийному -- теж. КАПСУЛІ -- читальня:
    // карта, розмови, записки, контакти (зрізом до заморозки). ЖИВИЙ говорить
    // за власника сесії повним набором, хто б його не тримав.
    private bool OwnerPageAllowed(string page, bool powered, bool ownedAtAll, bool devFrozen)
    {
        if (!powered)
            return false;
        if (!ownedAtAll)
            return false;
        if (!devFrozen)
            return true;
        if (page == OZ_PdaConst.PAGE_MAP || page == OZ_PdaConst.PAGE_CHAT)
            return true;
        if (page == OZ_PdaConst.PAGE_NOTES || page == OZ_PdaConst.PAGE_CONTACTS)
            return true;
        return false;
    }

    private string Status(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            // Порожні руки сюди більше не доходять узагалі: ворота вимагають
            // речі (рішення власника 2026-09-08). Тут стояла гілка
            // віртуального стану -- її немає; лишається чесна відмова.
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(pda.GetType());
        if (!prof)
        {
            // Пристрій є, а профілю під нього немає: адмін прибрав його з
            // Profiles.json або переплутав класнейм. Кажемо про це прямо.
            OZ_Log.Warn("no device profile for class " + pda.GetType());
            error = "STR_OZ_ERR_NO_PROFILE";
            return "";
        }

        PlayerBase player = OZ_PdaLookup.PlayerOf(sender);
        OZ_PlayerData pd = OZ_PlayerStore.Load(sender.GetPlainId());

        // Замок тут НЕ рахуємо: OZ_PdaAccess.Check уже покликав
        // OZ_EvaluateLock мікросекундами раніше, на цьому ж самому запиті
        // (OZ_PdaAccess.c:42) -- і робить це для КОЖНОЇ сторінки, а не лише
        // для цієї. Другий виклик тут був мертвою роботою, і він же створював
        // хибне враження, ніби автозамок тримається на секундному опитуванні
        // сторінки «Пристрій». Не тримається: він у воротах.

        // Ліниво, як і замок: рахунок дешифратора добігає саме тоді, коли на
        // пристрій дивляться.
        pda.OZ_EvaluateCrack();

        // Прив'язки «поглядом» більше немає: власність дає лише явна
        // ІНІЦІАЦІЯ (op initiate). Пристрій без сесії -- чесно нічий.

        OZ_PdaDeviceStatus st = new OZ_PdaDeviceStatus();

        st.ClassName   = pda.GetType();
        st.ProfileId   = prof.Id;
        st.DisplayName = prof.DisplayName;
        st.ModuleSlots = prof.ModuleSlots;
        st.LockAfterMinutes = prof.LockAfterMinutes;
        // Довжина коду, що СТОЇТЬ (його набирають), і довжина НОВОГО -- два
        // різні числа, див. OZ_PdaDeviceStatus.PinLength.
        st.PinLength    = pda.OZ_UnlockPinLength();
        st.NewPinLength = pda.OZ_PinLength();

        // Готове значення -- у поле; складену умову рахуємо окремо
        // (вимір 2026-09-01, див. OZR_Page.Book).
        bool inHands = false;
        if (player)
            inHands = (player.GetItemInHands() == pda);
        st.InHands = inHands;

        // Чужому чи неініційованому пристрою -- лише ЙОГО власні вкладки:
        // пристрій і карта. Акаунтні сторінки все одно відіб'є гейт, а
        // мертві вкладки в стрічці лише брехали б.
        bool ownedAtAll = pda.OZ_HasAnySession();
        bool devFrozen  = OZ_PdaCapsule.IsFrozen(pda);

        // ЧИЙ це пристрій -- саме за цим uid питаємо видимість вкладок.
        // Порожньо в нічийного: у нього все одно лишиться сама лише
        // сторінка «Пристрій», і питати про решту немає в кого.
        string pageUid = pda.OZ_SessionUid();

        // ВИМКНЕНИЙ ПРИЛАД ПОКАЗУЄ ЛИШЕ СЕБЕ. status пропускається на
        // вимкненому (з нього малюється кнопка живлення), і він обіцяв
        // віддавати тоді лише те, що видно ззовні, -- а віддавав увесь набір
        // вкладок ВЛАСНИКА: вкладка фракції на чужому мертвому приладі
        // казала, що його власник в угрупованні. Решта вкладок однаково
        // відмовить POWERED_DOWN.
        bool powered = pda.OZ_IsOn();

        for (int i = 0; i < prof.Pages.Count(); i++)
        {
            // На клієнт їдуть лише ті сторінки, які СПРАВДІ зареєстровані
            // І ВИДИМІ ЦЬОМУ ГРАВЦЕВІ: намалювати вкладку, за якою нікого
            // немає, гірше, ніж не намалювати її зовсім.
            //
            // Питає власника сторінки, а не вирішує сам: правило видимості --
            // справа того, хто сторінку приніс. Так вкладка «Фракція» зникає
            // в того, хто не в угрупованні (ТЗ-1 R4.3), а КПК про угруповання
            // так і не дізнається.
            //
            // uid тут -- ВЛАСНИКА СЕСІЇ: пристрій говорить за господаря.
            if (!OZ_PageRegistry.VisibleFor(prof.Pages[i], pageUid))
                continue;

            if (prof.Pages[i] != OZ_PdaConst.PAGE_DEVICE)
            {
                if (!OwnerPageAllowed(prof.Pages[i], powered, ownedAtAll, devFrozen))
                    continue;
            }

            st.Pages.Insert(prof.Pages[i]);
        }

        // І сторінки, які приносить ЗАЛІЗО.
        //
        // Профіль описує ПРИСТРІЙ, а не те, що в нього вставили, тож без
        // цього договір EnablesPages лишався б обіцянкою, якої КПК не
        // виконує. Саме так і сталося з рацією: гейт операцій уже питав
        // модулі (OZ_PdaAccess.ModuleEnables), а перелік вкладок -- ні, і
        // вставлена плата працювала б, якби до неї було як дійти.
        for (int m = 0; m < OZ_PdaConst.MODULE_SLOTS_MAX; m++)
        {
            string mcls = pda.OZ_ModuleClass(m);
            if (mcls == "")
                continue;

            OZ_ModuleSpec mspec = OZ_PdaHardware.ModuleFor(mcls);
            if (!mspec || !mspec.EnablesPages)
                continue;

            for (int e = 0; e < mspec.EnablesPages.Count(); e++)
            {
                string extra = mspec.EnablesPages[e];
                if (!OZ_PageRegistry.VisibleFor(extra, pageUid))
                    continue;
                if (st.Pages.Find(extra) != -1)
                    continue;
                // ТОЙ САМИЙ ФІЛЬТР, що й у сторінок профілю: вкладка рації на
                // нічийному приладі чи капсулі відмовляла б кожною операцією
                // -- «мертва вкладка», яку комент вище обіцяв не малювати.
                if (extra != OZ_PdaConst.PAGE_DEVICE && !OwnerPageAllowed(extra, powered, ownedAtAll, devFrozen))
                    continue;
                st.Pages.Insert(extra);
            }
        }

        st.Powered    = pda.OZ_IsOn();
        st.HasBattery = pda.OZ_HasBattery();
        st.Charge01   = pda.OZ_Charge01();

        for (int b = 0; b < OZ_PdaConst.MODULE_SLOTS_MAX; b++)
        {
            OZ_BayInfo bay = new OZ_BayInfo();
            bay.Index   = b;
            bay.Visible = (b < prof.ModuleSlots);

            // ГНІЗДО, А НЕ РОБОЧУ ПЛАТУ: OZ_ModuleClass не віддає ВИГОРІЛУ
            // (мертва електроніка), і саме тому екран показував її гніздо
            // порожнім. ТЗ-5 R-B2.9 вимагає протилежного: вигоріла підписана
            // прямо, а не відсутністю рядка.
            //
            // Але через ТІ САМІ двері (OZ_ModuleSeat), а не повз них: сирий
            // OZ_Attached обходив «схований відсік -- вимкнений відсік»
            // (ТЗ-4 R-F2.1), і вміст схованого гнізда їхав клієнту -- рядок
            // на екрані сховано, а по дроті назва, вид і залишок їхали.
            EntityAI seat = pda.OZ_ModuleSeat(b);
            ItemBase seatItem = ItemBase.Cast(seat);
            if (seatItem)
            {
                bay.ClassName = seatItem.GetType();
                bay.Burnt     = seatItem.IsRuined();

                OZ_ModuleSpec spec = OZ_PdaHardware.ModuleFor(bay.ClassName);
                if (spec)
                {
                    bay.Display = spec.DisplayName;
                    bay.Kind    = spec.Kind;
                    bay.LeftMin = ResourceLeftMin(seatItem, spec);
                }
            }
            st.Bays.Insert(bay);
        }

        st.CarrierClass = pda.OZ_CarrierClass();
        if (st.CarrierClass != "")
        {
            OZ_CarrierSpec cs = OZ_PdaHardware.CarrierFor(st.CarrierClass);
            if (cs)
            {
                st.CarrierWritable = cs.Writable;
                st.CarrierMaxRecords = cs.MaxRecords;
            }

            // ВМІСТ чипа -- лише на УВІМКНЕНОМУ пристрої. Наявність носія
            // видно фізично (клас вище), а от що на ньому записано і скільки
            // -- це вже читання, і мертвий КПК його не робить. Інакше
            // знайдений вимкнений прилад видавав би вміст чужого чипа тим
            // самим статусом, у якому carrier_read чесно відмовляє.
            //
            // Носій дістаємо ОДИН раз: два послідовні пошуки того самого
            // вкладення стояли поруч, і другий питав про те, що вже знайшов
            // перший.
            OZ_DataCarrier_Base carrier = OZ_DataCarrier_Base.Cast(pda.OZ_AttachedId(OZ_PdaSlots.Carrier()));
            if (carrier && st.Powered)
            {
                st.CarrierWritten = carrier.OZ_IsWritten();
                st.CarrierMarks   = carrier.OZ_MarkCount();
                st.CarrierNotes   = carrier.OZ_NoteCount();
                st.CarrierRoute   = carrier.OZ_Records(OZ_DataCarrier_Base.KIND_ROUTE);
            }
        }

        st.HasPin    = pda.OZ_HasPin();
        st.Unlocked  = pda.OZ_IsUnlocked();
        st.AutoLock  = pda.OZ_AutoLock();
        st.ForceAutoLock = prof.ForceAutoLock;
        st.LockedOut = pda.OZ_IsLockedOut(sender.GetPlainId());
        st.LockWaitS = pda.OZ_LockWaitSec(sender.GetPlainId());

        st.Sealed       = pda.OZ_IsSealed();
        st.HasDecryptor = pda.OZ_HasDecryptor();
        st.Cracking     = pda.OZ_IsCracking();
        st.CrackLeftSec = pda.OZ_CrackLeftSec();

        // Сесія й прив'язка -- теж читання, і теж лише на увімкненому.
        // Хто востаннє тримав пристрій живим і чи прив'язаний його акаунт --
        // не те, що видно ззовні з мертвого приладу.
        if (st.Powered)
        {
            // Онлайн міряється епохою ВЛАСНИКА сесії, не глядача: вкрадений
            // КПК живого власника має чесно казати «онлайн у нього», а
            // капсула -- лишатись капсулою в будь-чиїх руках.
            string ownUid = pda.OZ_SessionUid();
            int ownEpoch = 0;
            OZ_PlayerData ownPd = null;
            if (ownUid != "")
            {
                // Peek: власник сесії може бути офлайн -- це і є капсула.
                ownPd = OZ_PlayerStore.Peek(ownUid);
                if (ownPd)
                {
                    ownEpoch = ownPd.SessionEpoch;
                    st.OwnerName = ownPd.Name;
                }
            }

            st.Owned       = pda.OZ_HasAnySession();
            st.Online      = pda.OZ_IsOnline(ownEpoch);
            st.SessionMine = pda.OZ_HasSession(sender.GetPlainId(), pd.SessionEpoch);

            if (st.Online && ownPd)
            {
                // Живий пристрій наповнює свою майбутню капсулу даними
                // ВЛАСНИКА СЕСІЇ -- пристрій говорить за нього, хто б не
                // тримав. Штамп цього запису -- заразом МИТЬ ЗАМОРОЗКИ:
                // коли епоха власника піде вперед, зріз історії ріжеться
                // саме по ньому.
                //
                // ШТАМП -- ЩОРАЗУ, ТІЛО -- КОЛИ ЗМІНИЛОСЬ. Знімок перезбирався
                // й серіалізувався на КОЖЕН статус, тобто раз на п'ять секунд
                // на кожному живому приладі, разом із проходом по всіх друзях
                // власника й читанням файлу кожного з них. Складники знімка --
                // ім'я, база, угруповання й склад записника -- міняються раз
                // на години; підпис із них коштує склейку кількох рядків.
                string sig = ownPd.Name;
                sig += "|" + OZ_Identity.Get().BaseOf(ownUid);
                sig += "|" + OZ_Identity.Get().OrgOf(ownUid);
                sig += "|" + ownPd.Friends.Count().ToString();
                if (ownPd.Friends.Count() > 0)
                    sig += "|" + ownPd.Friends[ownPd.Friends.Count() - 1];

                if (sig == pda.OZ_SnapshotSig() && pda.OZ_Snapshot() != "")
                {
                    pda.OZ_TouchSnapshot(ownEpoch);
                }
                else
                {
                    OZ_PdaSnapshot snap = new OZ_PdaSnapshot();
                    snap.Owner   = ownPd.Name;
                    snap.Base    = OZ_Identity.Get().FactionName(OZ_Identity.Get().BaseOf(ownUid));
                    snap.Org     = OZ_Identity.Get().FactionName(OZ_Identity.Get().OrgOf(ownUid));
                    for (int sf = 0; sf < ownPd.Friends.Count(); sf++)
                    {
                        // Ім'я з ТОГО САМОГО персонажа, а не з акаунта: у
                        // капсулі мусить лишитись той, кого власник знав.
                        OZ_PlayerData fr = OZ_PlayerStore.ByKey(ownPd.Friends[sf]);
                        if (fr && fr.Name != "")
                            snap.Contacts.Insert(fr.Name);
                    }

                    string snapJson;
                    string snapErr;
                    if (JsonFileLoader<OZ_PdaSnapshot>.MakeData(snap, snapJson, snapErr, false))
                        pda.OZ_RefreshSnapshot(ownEpoch, snapJson, sig);
                }
            }

            if (!st.Online)
            {
                st.SnapshotAt = pda.OZ_SnapshotAt();

                // Об'єктом, а не рядком (див. OZ_PdaDeviceStatus.Snap): на
                // диску предмета знімок лишається JSON-рядком, а на дріт іде
                // розібраним. Копія -- до наступних виділень.
                if (pda.OZ_Snapshot() != "")
                {
                    OZ_PdaSnapshot snapOut = new OZ_PdaSnapshot();
                    string snapReadErr;
                    if (JsonFileLoader<OZ_PdaSnapshot>.LoadData(pda.OZ_Snapshot(), snapOut, snapReadErr) && snapOut)
                        st.Snap = snapOut.Copy();
                }
            }

            st.DiscordLinked = (pd.DiscordId != "");
        }

        // ТУТ ПИТАЛИ РАДІАЦІЮ, і більше не питають (рішення власника
        // 2026-09-09): радіометра й дозиметра в моді немає, а без них питати
        // нема кому й нема чим. Договір OZ_PdaRadiation пішов слідом -- він
        // існував рівно заради цих двох рядків.

        string outJson;
        string err;
        if (!JsonFileLoader<OZ_PdaDeviceStatus>.MakeData(st, outJson, err, false))
        {
            OZ_Log.Error("device status serialise failed: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return outJson;
    }

    private string Unlock(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        OZ_PdaPinAttempt att = new OZ_PdaPinAttempt();
        string err;
        if (!JsonFileLoader<OZ_PdaPinAttempt>.LoadData(json, att, err) || !att)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        // Причину відмови кажемо ТУ, яка є. «Хибний код» на запечатаному
        // приладі (код якого не знає ніхто) і під час блокування (коли
        // хибним виявляється і правильний код) -- неправда, через яку
        // гравець підбирав далі, не знаючи, що його не слухають.
        string refused = PinRefusal(pda, sender);
        if (refused != "")
        {
            error = refused;
            return LockoutBody(pda, sender);
        }

        if (!pda.OZ_TryUnlock(sender.GetPlainId(), att.Pin))
        {
            // Скільки спроб лишилось -- НЕ кажемо. Це підказка тому, хто
            // підбирає, і жодної користі власнику.
            error = "STR_OZ_ERR_BAD_PIN";

            // А от що спроби СКІНЧИЛИСЬ -- кажемо: ця відмова щойно стала
            // блокуванням, і наступну цифру слухати не будуть.
            if (pda.OZ_IsLockedOut(sender.GetPlainId()))
            {
                error = "STR_OZ_LOCK_TOO_MANY";
                return LockoutBody(pda, sender);
            }
            return "";
        }

        // Знати пін -- НЕ означає володіти: прив'язка лишається за
        // ініціатором (рішення власника 2026-08-28), і сесію дає лише явна
        // ініціація. Тут стояло «бесхазяйний пристрій відкриває сесію тому,
        // хто його відімкнув -- та сама умова, що й у setpin»; setpin сесій
        // не відкриває (див. його), і правило розійшлось із самим собою:
        // відімкнувши неініційований прилад із піном, гравець мовчки ставав
        // його власником.

        ok = true;
        error = "";
        return "";
    }

    // Чому код НЕ ПЕРЕВІРЯТИМУТЬ зовсім, або порожньо, якщо перевірять.
    private string PinRefusal(OZ_PDA_Base pda, PlayerIdentity sender)
    {
        if (pda.OZ_IsSealed())
            return "STR_OZ_ERR_SEALED";
        if (pda.OZ_IsLockedOut(sender.GetPlainId()))
            return "STR_OZ_LOCK_TOO_MANY";
        return "";
    }

    // Тіло відмови з відліком блокування: пад малює «ще N с» з нього.
    // Відповідь на status (де відлік жив досі) замкнений прилад не отримує
    // ніколи -- ворота відмовляють раніше, -- тож і відліку ніхто не бачив.
    private string LockoutBody(OZ_PDA_Base pda, PlayerIdentity sender)
    {
        OZ_PdaDeviceStatus lo = new OZ_PdaDeviceStatus();
        lo.LockedOut = pda.OZ_IsLockedOut(sender.GetPlainId());
        lo.LockWaitS = pda.OZ_LockWaitSec(sender.GetPlainId());

        string outJson;
        string err;
        if (!JsonFileLoader<OZ_PdaDeviceStatus>.MakeData(lo, outJson, err, false))
            return "";
        return outJson;
    }

    // Що можна сказати про ЗАМКНЕНИЙ пристрій, не відмикаючи його.
    //
    // Рівно чотири речі, і жодна з них нічого не видає: чи він запечатаний,
    // чи є чим його зламати, чи вже ламають і скільки лишилось. Усе решта --
    // ім'я, профіль, вміст -- лишається за замком, бо саме за цим замок і є.
    private string Sealed(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        pda.OZ_EvaluateCrack();

        OZ_PdaDeviceStatus st = new OZ_PdaDeviceStatus();
        st.Sealed       = pda.OZ_IsSealed();
        st.HasDecryptor = pda.OZ_HasDecryptor();
        st.Cracking     = pda.OZ_IsCracking();
        st.CrackLeftSec = pda.OZ_CrackLeftSec();
        // П'ЯТА річ, і вона теж нічого не видає: скільки цифр у коді
        // (ТЗ-5 R-B3.3). Без неї пад на ЗАМКНЕНОМУ приладі -- а це і є
        // єдиний екран, де код набирають, -- малював би чотири крапки
        // моделі, яка просить шість.
        st.PinLength    = pda.OZ_UnlockPinLength();
        st.NewPinLength = pda.OZ_PinLength();

        // І ШОСТА -- чи слухають зараз коди взагалі, і скільки ще чекати.
        // Це екран ЗАМКНЕНОГО приладу, тобто єдиний, де відлік потрібен.
        st.LockedOut = pda.OZ_IsLockedOut(sender.GetPlainId());
        st.LockWaitS = pda.OZ_LockWaitSec(sender.GetPlainId());

        string outJson;
        string err;
        if (!JsonFileLoader<OZ_PdaDeviceStatus>.MakeData(st, outJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return outJson;
    }

    // Почати злам запечатаного пристрою.
    private string Crack(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(pda.GetType());
        if (!prof)
        {
            error = "STR_OZ_ERR_NO_PROFILE";
            return "";
        }

        string why = pda.OZ_StartCrack(prof.CrackSeconds);
        if (why != "")
        {
            error = why;
            return "";
        }

        ok = true;
        error = "";
        return "";
    }

    // Зміна коду. Щоб змінити пін, його треба ЗНАТИ -- пристрій не питає,
    // хто ти, він питає старий код. Порожній новий код означає «зняти пін».
    private string SetPin(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        OZ_PdaPinChange ch = new OZ_PdaPinChange();
        string err;
        if (!JsonFileLoader<OZ_PdaPinChange>.LoadData(json, ch, err) || !ch)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        // Та сама пара чесних причин, що й у Unlock.
        string refused = PinRefusal(pda, sender);
        if (refused != "")
        {
            error = refused;
            return LockoutBody(pda, sender);
        }

        if (!pda.OZ_SetPin(sender.GetPlainId(), ch.OldPin, ch.NewPin))
        {
            // Скільки спроб лишилось -- НЕ кажемо, з тієї ж причини, що й при
            // відмиканні: це підказка тому, хто підбирає.
            error = "STR_OZ_ERR_BAD_PIN";
            if (pda.OZ_IsLockedOut(sender.GetPlainId()))
            {
                error = "STR_OZ_LOCK_TOO_MANY";
                return LockoutBody(pda, sender);
            }
            return "";
        }

        // Пін -- це ЗАМОК, не власність: сесію дає лише явна ініціація.

        ok = true;
        error = "";
        return "";
    }

    // ІНІЦІАЦІЯ: явна церемонія власності. Пристрій прив'язується до
    // ПОТОЧНОЇ епохи гравця і стає ще одним його живим терміналом --
    // попередні НЕ гаснуть (рішення власника 2026-08-28). Гасить їх лише
    // явний LOG OUT OTHER DEVICES.
    private string Initiate(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        if (pda.OZ_HasAnySession())
        {
            error = "STR_OZ_ERR_OWNED";
            return "";
        }

        string uid = sender.GetPlainId();
        OZ_PlayerData pd = OZ_PlayerStore.Load(uid);

        pda.OZ_OpenSession(uid, pd.SessionEpoch);

        OZ_Log.Info("pda: " + uid + " initiated " + pda.GetType() + ", epoch " + pd.SessionEpoch.ToString());

        ok = true;
        error = "";
        return "";
    }

    // «Розлогінитись на інших»: епоха +1 і перевідкриття СВОЄЇ сесії тут
    // же -- всі інші пристрої власника замерзають, цей лишається єдиним
    // живим. Вимагає живої сесії саме на цьому пристрої.
    private string LogoutOthers(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        string uid = sender.GetPlainId();
        OZ_PlayerData pd = OZ_PlayerStore.Load(uid);

        if (!pda.OZ_HasSession(uid, pd.SessionEpoch))
        {
            error = "STR_OZ_ERR_PDA_NOT_YOURS";
            return "";
        }

        // ОДИН РАЗ НА КІЛЬКА СЕКУНД. Кожен виклик пише файл гравця
        // синхронно (Flush), а ворота його пропускають щоразу: операція сама
        // перевідкриває сесію з новою епохою. Змінений клієнт, що слав її
        // сотні разів на секунду, робив із сервера молотарку диска. Ядро
        // тримає таку саму стелю на привіті (OZ_Module, HELLO_GAP_MS).
        int now = GetGame().GetTime();
        int lastAt;
        if (m_LogoutAt.Find(uid, lastAt) && now - lastAt < LOGOUT_GAP_MS)
        {
            error = "STR_OZ_ERR_BUSY";
            return "";
        }
        m_LogoutAt.Set(uid, now);

        pd.SessionEpoch = pd.SessionEpoch + 1;
        OZ_PlayerStore.Flush(uid);
        pda.OZ_OpenSession(uid, pd.SessionEpoch);

        OZ_Log.Info("pda: " + uid + " logged out other devices, epoch " + pd.SessionEpoch.ToString());

        ok = true;
        error = "";
        return "";
    }

    // Коли кожен гравець востаннє скидав інші сесії. Росте на ті uid, що
    // це робили, -- одиниці за аптайм, чистити нема чого.
    private ref map<string, int> m_LogoutAt = new map<string, int>();
    private static const int LOGOUT_GAP_MS = 5000;

    // Ручний замок: власник іде від пристрою -- пристрій мовчить одразу,
    // а не за таймером автолока.
    private string Lock(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        if (!pda.OZ_HasPin())
        {
            error = "STR_OZ_ERR_NO_PIN";
            return "";
        }

        pda.OZ_Lock();
        ok = true;
        error = "";
        return "";
    }

    // «До заводських» без піна -- переініціалізація знайденого пристрою.
    // Дані попереднього власника згорають чесно й повністю; Sealed
    // відмовляє всередині предмета.
    private string FactoryReset(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        string why = pda.OZ_FactoryReset();
        if (why != "")
        {
            error = why;
            return "";
        }

        OZ_Log.Info("pda: factory reset by " + sender.GetPlainId());
        ok = true;
        error = "";
        return "";
    }

    // Живлення. Той самий важіль, що й ванільна дія з рук -- але через сервер:
    // клієнт просить, сервер вирішує й називає причину відмови.
    private string Power(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        OZ_PdaFlagOp flag = new OZ_PdaFlagOp();
        string err;
        if (!JsonFileLoader<OZ_PdaFlagOp>.LoadData(json, flag, err) || !flag)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string why = pda.OZ_SetPower(flag.Value);
        if (why != "")
        {
            error = why;
            return "";
        }

        ok = true;
        error = "";
        return "";
    }

    private string AutoLock(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = DeviceOf(sender, error);
        if (!pda)
            return "";

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(pda.GetType());
        if (!prof)
        {
            error = "STR_OZ_ERR_NO_PROFILE";
            return "";
        }

        OZ_PdaFlagOp flag = new OZ_PdaFlagOp();
        string err;
        if (!JsonFileLoader<OZ_PdaFlagOp>.LoadData(json, flag, err) || !flag)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        if (!pda.OZ_SetAutoLock(flag.Value, prof.ForceAutoLock))
        {
            error = "STR_OZ_ERR_REFUSED";
            return "";
        }

        ok = true;
        error = "";
        return "";
    }
}

class OZ_PdaHandlerQuests : OZ_PageHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        if (op == "journal")
        {
            // Порожній журнал і ВІДСУТНІЙ журнал -- різні повідомлення для
            // гравця. HasProvider розрізняє «завдань немає» і «на цьому
            // сервері квестового мода взагалі немає».
            OZ_QuestJournal j = OZ_PdaQuests.Collect(sender);

            string outJson;
            string err;
            if (JsonFileLoader<OZ_QuestJournal>.MakeData(j, outJson, err, false))
            {
                ok = true;
                error = "";
                return outJson;
            }

            OZ_Log.Error("quest journal serialise failed: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
        }

        return "";
    }
}

[CF_RegisterModule(OZ_PdaModule)]
class OZ_PdaModule : CF_ModuleWorld
{
    private ref Timer m_BeaconTimer;

    // Один живий модуль на сервер: аплаєр тюнінгу мусить дотягтися до його
    // таймера, а тримати другий шлях до нього -- це другий спосіб помилитись.
    private static OZ_PdaModule s_Inst;

    void BeaconTick()
    {
        OZ_PdaHandlerMap.PushBeacons();
    }

    // ПЕРІОД ПОСИЛОК ЧИТАВСЯ РІВНО ОДИН РАЗ, на старті місії. Адмін міняв
    // BeaconPushSeconds у вкладці VPP, бачив «застосовано» -- і нічого не
    // відбувалось до рестарту сервера. Аплаєр кличе це після ServerLoad.
    static void RearmBeacons()
    {
        if (!s_Inst || !s_Inst.m_BeaconTimer)
            return;

        s_Inst.m_BeaconTimer.Stop();
        s_Inst.m_BeaconTimer.Run(OZ_PdaTune.BeaconPushSeconds(), s_Inst, "BeaconTick", NULL, true);
        OZ_Log.Info("pda: beacon push re-armed at " + OZ_PdaTune.BeaconPushSeconds().ToString() + "s");
    }

    override void OnInit()
    {
        super.OnInit();
        EnableMissionStart();
        EnableClientDisconnect();
    }

    // Гравець вийшов -- забуваємо, що йому востаннє посилали.
    //
    // Мапа підписів маячків (OZ_PdaHandlerMap.m_BeaconSig) ключується
    // Steam64 і не чистилась ніколи: за довгий аптайм у ній осідав рядок на
    // кожного, хто хоч раз зайшов. Гірше за пам'ять було інше -- гравець, що
    // вийшов із порожнім списком і повернувся, мав у мапі "" й не отримував
    // ПЕРШОГО порожнього пуша, тобто клієнт не діставав команди стерти
    // маячки, які встиг намалювати за минулий сеанс.
    //
    // ПОДІЯ -- OnClientDisconnect, а не OnInvokeDisconnect. Другу CF кличе з
    // голими CF_EventPlayerArgs, і приведення до CF_EventPlayerDisconnectedArgs
    // тут завжди давало null: обробник виходив першим же рядком на КОЖНОМУ
    // дисконекті від першого дня. UID у тих аргументах -- ХЕШОВАНИЙ id рушія,
    // а мапи тут ключуються Steam64; Steam64 дістає ядро
    // (OZ_Players.PlainOfLeaving).
    override void OnClientDisconnect(Class sender, CF_EventArgs args)
    {
        super.OnClientDisconnect(sender, args);

        if (!GetGame().IsServer())
            return;

        string uid = OZ_Players.PlainOfLeaving(args);
        if (uid == "")
            return;

        OZ_PdaHandlerMap.ForgetBeacons(uid);
    }

    // Що КПК докладає до пакета синхронізації ядра (D87).
    //
    // Тост і крок ведення маршрутом їхали лише в посилці маячків, а її
    // отримує той, у кого є антена: решта жила на умовчаннях із config.cpp,
    // хоч адмін виставив інше в OZ_PDA_Tuning.json. Пакет ядра їде кожному
    // на вході й знову на кожну зміну -- саме те місце для двох чисел, які
    // потрібні всім і не змінюються від маячка до маячка.
    void OZ_PdaSyncFill(OZ_SyncPayload p)
    {
        OZ_SyncExtras.Put(p, OZ_PdaConst.SYNC_TOAST_S, OZ_PdaTune.ToastSeconds().ToString());
        OZ_SyncExtras.Put(p, OZ_PdaConst.SYNC_ROUTE_M, OZ_PdaTune.RouteAdvanceM().ToString());
        OZ_SyncExtras.Put(p, OZ_PdaConst.SYNC_MSG_MAX, OZ_PdaTune.ChatMsgMax().ToString());
        // Скільки відсіків видно на кожній моделі -- інвентар питає це
        // клієнта, а профілі живуть тут (OZ_PdaProfiles.ClientSlots).
        OZ_SyncExtras.Put(p, OZ_PdaConst.SYNC_SLOTS, OZ_PdaProfiles.PackSlots());
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        if (!GetGame().IsServer())
            return;

        OZ_SyncExtras.OnFill().Insert(OZ_PdaSyncFill);

        // Дерево каталогів профілю -- ПЕРШИМ рядком, до будь-якого читання
        // чи запису. Ядро будує його у своєму OnMissionStart, але порядок
        // CF-модулів не гарантований, і на цьому ж стенді він уже підводив:
        // рація відпрацювала раніше за КПК. EnsureTree ідемпотентна, тож
        // зайвий виклик коштує нічого, а відсутній коштує конфігів.
        OZ_Json.EnsureTree();

        // Спочатку СТОРІНКИ, потім профілі: Validate() профілів звіряє свій
        // список Pages з реєстром, і на порожньому реєстрі виплюнув би
        // попередження на кожен рядок.
        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_DEVICE,
                                 "#STR_OZ_PAGE_DEVICE",
                                 "set:oz_pda image:device",
                                 new OZ_PdaHandlerDevice());

        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_QUESTS,
                                 "#STR_OZ_PAGE_QUESTS",
                                 "set:oz_pda image:quests",
                                 new OZ_PdaHandlerQuests());

        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_CONTACTS,
                                 "#STR_OZ_PAGE_CONTACTS",
                                 "set:oz_pda image:contacts",
                                 new OZ_PdaHandlerContacts());

        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_NOTES,
                                 "#STR_OZ_PAGE_NOTES",
                                 "set:oz_pda image:notes",
                                 new OZ_PdaHandlerNotes());

        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_MAP,
                                 "#STR_OZ_PAGE_MAP",
                                 "set:oz_pda image:map",
                                 new OZ_PdaHandlerMap());

        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_CHAT,
                                 "#STR_OZ_PAGE_CHAT",
                                 "set:oz_pda image:chat",
                                 new OZ_PdaHandlerChat());


        OZ_PageRegistry.Register(OZ_PdaConst.PAGE_NEWS,
                                 "#STR_OZ_PAGE_NEWS",
                                 "set:oz_pda image:news",
                                 new OZ_PdaHandlerNews());

        // Розмови живуть у Discord, тож на диску їм каталогу не треба -- а
        // ось вухо для вхідних рядків треба. Підписка не залежить від того,
        // чи вже стартував міст: порядок модулів CF не гарантований, а мапа
        // приймачів однаково питається на кожну пачку.
        OZ_BridgeClient.Subscribe("chat", new OZ_ChatSink());
        OZ_BridgeClient.Subscribe("news", new OZ_NewsSink());
        // Штовхання ролей у КПК підписує СКЛЕЙКА (OZFP_Module): рядок називав
        // одразу два класи мода фракцій, і через нього КПК не компілювався
        // без нього взагалі.
        OZ_PdaModules.Register(new OZ_SpyAntennaBehaviour());
        OZ_BridgeClient.RegisterUidProvider(new OZ_PdaUidProvider());

        // ПЕРМАДЕС: стираємо СВОЄ, і більше нічиє. Шість полів файла гравця,
        // які пише лише КПК, до 2026-09-08 чистив мод фракцій -- через що
        // сервер core+PDA без нього не мав вайпу зовсім. Ядро тримає конвеєр,
        // ми лише кажемо, що в ньому наше.
        OZ_Wipe.Register("pda", new OZ_PdaWiper());

        OZ_PdaProfiles.ServerLoad();
        OZ_PdaHardware.ServerLoad();
        OZ_PdaTuning.ServerLoad();

        // Конфіги КПК стають редагованими з адмінської консолі ядра.
        OZ_AdminCfg.Register("Tuning",   OZ_Const.PROFILE_DIR + "\\OZ_PDA_Tuning.json",   new OZ_PdaTuningApplier(), "pda");
        OZ_AdminCfg.Register("Profiles", OZ_PdaConst.PROFILES, new OZ_PdaProfilesApplier(), "pda");
        OZ_AdminCfg.Register("Hardware", OZ_PdaConst.HARDWARE, new OZ_PdaHardwareApplier(), "pda");

        // Маячки транспондера РОЗСИЛАЄ сервер -- раз на кілька секунд тим,
        // у кого антена справді працює. Клієнт більше нічого не опитує:
        // на сорока гравцях це мінус десять запитів на секунду.
        s_Inst = this;
        m_BeaconTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_BeaconTimer.Run(OZ_PdaTune.BeaconPushSeconds(), this, "BeaconTick", NULL, true);

        // Ядро пускало всі сторінки, бо пристроїв не має. Тепер вирішує той,
        // хто їх приносить.
        OZ_PageAccess.Bind(new OZ_PdaAccess());

        CheckSlots();

        string summary = "pda loaded: profiles=" + OZ_PdaProfiles.Count().ToString();
        summary += " pages=" + OZ_PageRegistry.Count().ToString();
        summary += " modules=" + OZ_PdaHardware.ModuleCount().ToString();
        summary += " carriers=" + OZ_PdaHardware.CarrierCount().ToString();
        OZ_Log.Info(summary);
    }

    // Дзеркало OnMissionStart: інвокер ядра статичний і переживе місію, а
    // модуль, що в ньому підписаний, -- ні (рація й дослідження роблять те
    // саме). Сторінки й доступ із реєстрів ядра прибирає саме ядро, після
    // всіх модулів (OZ_MissionServer.OnMissionFinish).
    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        super.OnMissionFinish(sender, args);

        if (!GetGame().IsServer())
            return;

        if (m_BeaconTimer)
            m_BeaconTimer.Stop();

        OZ_SyncExtras.OnFill().Remove(OZ_PdaSyncFill);
        s_Inst = NULL;
    }

    // Слот з друкарською помилкою в імені -- класична мовчазна поломка: конфіг
    // парситься, предмет спавниться, а вкласти в нього нічого не можна, і в
    // лозі про це ані слова. Ловимо на буті, а не в грі.
    private void CheckSlots()
    {
        CheckSlot(OZ_PdaConst.SLOT_BATTERY);
        CheckSlot(OZ_PdaConst.SLOT_CARRIER);
        CheckSlot(OZ_PdaConst.SLOT_WEAR);
        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
            CheckSlot(OZ_PdaConst.ModuleSlot(i));
    }

    private void CheckSlot(string name)
    {
        int id = InventorySlots.GetSlotIdFromString(name);
        if (id == -1)
        {
            OZ_Log.Warn("slot \"" + name + "\" does not resolve - check CfgSlots and attachments[]");
            return;
        }
        OZ_Log.Dbg("slot " + name + " -> id " + id.ToString());
    }
}
