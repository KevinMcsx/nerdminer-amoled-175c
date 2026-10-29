# NerdMiner – Waveshare ESP32-S3-Touch-AMOLED-1.75C

Külön PlatformIO projekt a BitMaker NerdMiner_v2 alapján. Az eredeti README és licenc változatlanul megtalálható a projektben.

## Hardverprofil

- CO5300 QSPI, 466×466, kör alakú panelhez igazított szövegelhelyezés.
- CST9217 érintés: érintésenként egy lapváltás.
- 240 MHz CPU, 8 MB OPI PSRAM, 32 MB flash profil.
- 2 × 6 MB alkalmazáspartíció és SPIFFS. Az upstream webflasher fix flash-című konfigurációolvasása ezen a profilon kikapcsolva.
- Fekete háttér, közepes fényerő, másodpercenkénti adatfrissítés, teljes képernyős framebuffer nélkül.
- Az upstream hardveres SHA és szoftveres bányászfeladatai megmaradtak. Sebességnövekedés nincs hardveres méréssel igazolva.

## Használat

Nyisd meg ezt a mappát VS Code / PlatformIO alatt. Az alapértelmezett környezet `waveshare-175c`.

```powershell
pio run -e waveshare-175c
pio run -e waveshare-175c -t upload
pio device monitor -b 115200
```

Első indítás: csatlakozz a `NerdMinerAP` Wi-Fi hálózathoz, jelszó: `MineYourCoins`. A böngészőben nyisd meg a `192.168.4.1` címet, és állítsd be a saját Wi-Fi kapcsolatodat, Bitcoin-címedet, valamint a pool címét és portját.

Érintés vagy rövid BOOT gombnyomás: következő képernyő. BOOT dupla kattintás: 180° forgatás. BOOT nyomva tartása 5 másodpercig: a NerdMiner konfigurációjának törlése. A PWR gombot a panel energiavezérlője kezeli.

A kijelző kikapcsolható a BOOT többszörös kattintásával az upstream gombkezelés szerint; érintésre felébred. Bányászat közben a kijelző kikapcsolása nem állítja le a bányászfeladatokat.

## Ellenőrzés a készüléken

Feltöltés után ellenőrizendő: kép pozíciója, érintésenként egy lapváltás, Wi-Fi beállítás, pool kapcsolat, hashrate és hosszabb üzem stabilitása. Az AXP2101 feszültség- és töltésbeállításait ez a port nem módosítja; a gyártói kijelzőpélda inicializálását követi. Akkumulátorszint-kijelzés és új töltésvezérlés nincs hozzáadva.

Források:
- https://github.com/bitmaker-hub/nerdminer_v2
- https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C
- https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C

Upstream alap commit: `e3a04b7d9f0b6e60ca6e02f7433c94d78d9dded4`.

## Fordítási ellenőrzés

2026-10-28: `pio run -e waveshare-175c` sikeres. Statikus RAM: 54 000 bájt; alkalmazás: 1 164 853 bájt. Ezek fordítási méretek, a futás közbeni heap-fogyasztást nem mérik. Fizikai készüléken még nem tesztelt.

Hardveres indul?s alapj?n a SPIFFS part?ci? 2 MB-ra cs?kkentve; a nagyobb part?ci?t az Arduino alap?rtelmezett SPIFFS konfigur?ci?ja nem t?mogatta. A pool-jelsz? mez? ?rv?nytelen HTML-azonos?t?ja jav?tva.
